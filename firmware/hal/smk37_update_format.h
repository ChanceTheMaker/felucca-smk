/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>
#define SMKU_HEADER 1024u
#define SMKU_LO 0x4000u
#define SMKU_HI 0x9d000u
#define SMKU_BYTES (SMKU_HI-SMKU_LO)
#define SMKU_SECTORS (SMKU_BYTES/4096u)
static uint32_t smku_rd(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint32_t smku_crc(const uint8_t *p,uint32_t n,uint32_t crc){
    while(n--){crc^=*p++;for(unsigned b=0;b<8;++b)crc=(crc>>1)^((0u-(crc&1u))&0xedb88320u);}return crc;
}
static int smku_header_check(const uint8_t *h,uint32_t head_crc,uint32_t trim_crc,int portable){
    const uint8_t magic[16]="SMK37-SALT-USB1";
    for(unsigned i=0;i<16;++i)if(h[i]!=magic[i])return 0;
    unsigned version=smku_rd(h+16);
    if((version!=1 && !(portable && version==2)) || smku_rd(h+20)!=SMKU_LO || smku_rd(h+24)!=SMKU_HI ||
       smku_rd(h+28)!=SMKU_BYTES || smku_rd(h+32)!=head_crc || smku_rd(h+36)!=(version==2?0:trim_crc))return 0;
    uint32_t crc=~0u;for(unsigned i=0;i<SMKU_HEADER;++i){uint8_t b=(i>=44 && i<48)?0:h[i];crc=smku_crc(&b,1,crc);}
    if((crc^~0u)!=smku_rd(h+44))return 0;
    const char model[]="SMK-37_";for(unsigned i=0;i<7;++i)if(h[48+i]!=(uint8_t)model[i])return 0;
    for(unsigned i=55;i<58;++i)if(h[i]<'0'||h[i]>'9')return 0;
    if(h[58])return 0;
    for(unsigned i=59;i<80;++i)if(h[i])return 0;
    if(!h[80] || h[111])return 0;
    for(unsigned i=112;i<128;++i)if(h[i])return 0;
    for(unsigned i=128+SMKU_SECTORS*4;i<SMKU_HEADER;++i)if(h[i])return 0;
    return 1;
}
static int smku_header_ok(const uint8_t *h,uint32_t head_crc,uint32_t trim_crc){return smku_header_check(h,head_crc,trim_crc,0);}
