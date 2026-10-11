/* SPDX-License-Identifier: GPL-3.0-only
 * SMK-only raw encrypted application update. No SPL records or tail writes.
 * Runs entirely from RAM. Disconnects/timeouts never trigger a reset.
 * Power loss during programming requires the hardware recovery adapter.
 * Transport hooks: smku_read(host offset), smku_flash_read/erase/prog, smku_poll.
 */
#include "smk37_update_format.h"
#include "smk37_public_profile.h"
static uint8_t smku_header[SMKU_HEADER],smku_sector[4096],smku_current[4096];
static uint32_t smku_flash_crc(uint32_t off,uint32_t n,int *ok){
    uint32_t crc=~0u;
    while(n){unsigned count=n>4096?4096:n;if(smku_flash_read(off,smku_current,count)){*ok=0;return 0;}
        crc=smku_crc(smku_current,count,crc);off+=count;n-=count;smku_poll();}
    return crc^~0u;
}
static int smku_fetch(unsigned sector){
    for(unsigned k=0;k<4096;k+=512)if(smku_read(SMKU_HEADER+sector*4096+k,smku_sector+k,512))return -1;
    return (smku_crc(smku_sector,4096,~0u)^~0u)==smku_rd(smku_header+128+sector*4)?0:-2;
}
static __attribute__((noinline)) int smku_session(void){
    if(smku_flash_id()!=0x856014u)return -14;
    for(unsigned k=0;k<SMKU_HEADER;k+=512)if(smku_read(k,smku_header+k,512))return -1;
    int ok=1;
    uint32_t head_crc=SMK37_UNIT_HEAD_CRC,trim_crc=SMK37_UNIT_TRIM_CRC;
#if SMK37_PUBLIC_BUILD
    head_crc=SMK37_PUBLIC_HEAD_CRC;
    if(smku_flash_read(0xc1000,smku_current,556) || !smk37_trim_keys_valid(smku_current,556))return -3;
    trim_crc=smku_crc(smku_current,556,~0u)^~0u;
#endif
    if(!smku_header_check(smku_header,head_crc,trim_crc,SMK37_PUBLIC_BUILD))return -2;
    if(smku_flash_crc(0,0x4000,&ok)!=head_crc || !ok ||
       smku_flash_crc(0xc1000,556,&ok)!=trim_crc || !ok)return -3;
    /* Validate the complete package before the first erase. Revalidate each
       sector on the second pass so changed host data cannot bypass preflight. */
    uint32_t crc=~0u;
    for(unsigned s=0;s<SMKU_SECTORS;++s){if(smku_fetch(s))return -4;crc=smku_crc(smku_sector,4096,crc);}
    if((crc^~0u)!=smku_rd(smku_header+40))return -5;
    for(unsigned s=0;s<SMKU_SECTORS;++s){
        uint32_t off=SMKU_LO+s*4096;
        if(smku_fetch(s))return -6;
        if(smku_flash_read(off,smku_current,4096))return -7;
        unsigned different=0;for(unsigned i=0;i<4096;++i)different|=smku_sector[i]^smku_current[i];
        if(!different)continue;
        if(smku_flash_erase(off))return -8;
        for(unsigned k=0;k<4096;k+=256)if(smku_flash_prog(off+k,smku_sector+k,256))return -9;
        if(smku_flash_read(off,smku_current,4096))return -10;
        for(unsigned i=0;i<4096;++i)if(smku_sector[i]!=smku_current[i])return -11;
        smku_poll();
    }
    if(smku_flash_crc(SMKU_LO,SMKU_BYTES,&ok)!=smku_rd(smku_header+40) || !ok)return -12;
    if(smku_flash_crc(0,0x4000,&ok)!=head_crc || !ok ||
       smku_flash_crc(0xc1000,556,&ok)!=trim_crc || !ok)return -15;
    /* Final handshake is sent only after whole-application readback. */
    if(smku_read(0xf0000000u,smku_sector,8))return -13;
    return 0;
}
