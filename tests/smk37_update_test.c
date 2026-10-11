/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "smk37_update_format.h"
static uint8_t flash[0x100000],original[0x100000],package[SMKU_HEADER+SMKU_BYTES];
static uint32_t headcrc,trimcrc;static unsigned erases,programs,reads,fetches;static int mode;
#ifdef TEST_PUBLIC
#define SMK37_PUBLIC_BUILD 1
#include "smk37_public_profile.h"
/* Synthetic model header for host tests only. Production keeps its pinned CRC. */
#undef SMK37_PUBLIC_HEAD_CRC
#define SMK37_PUBLIC_HEAD_CRC headcrc
#endif
#define SMK37_UNIT_HEAD_CRC headcrc
#define SMK37_UNIT_TRIM_CRC trimcrc
static int smku_read(uint32_t off,uint8_t*p,uint32_t n){
    if(off==0xf0000000){assert(n==8);memcpy(p,"success",8);return 0;}
    assert(off<=sizeof package && n<=sizeof package-off);++reads;
    if(off==SMKU_HEADER)++fetches;
    if(mode==3 && fetches==2)return -1;
    memcpy(p,package+off,n);
    if(mode==4 && fetches==2 && off==SMKU_HEADER)p[0]^=1;
    return 0;
}
static int smku_flash_read(uint32_t off,void*p,uint32_t n){assert(off<=sizeof flash && n<=sizeof flash-off);memcpy(p,flash+off,n);return 0;}
static int smku_flash_erase(uint32_t off){assert(off>=SMKU_LO && off<=SMKU_HI-4096 && !(off%4096));++erases;memset(flash+off,255,4096);return 0;}
static int smku_flash_prog(uint32_t off,const void*p,uint32_t n){assert(off>=SMKU_LO && off<SMKU_HI && n<=SMKU_HI-off);++programs;if(mode==5)return -1;memcpy(flash+off,p,n);return 0;}
static void smku_poll(void){}
static uint32_t smku_flash_id(void){return mode==6?0:0x856014;}
#include "../firmware/loader/smk37_update_core.c"
static void put(unsigned off,uint32_t x){for(unsigned i=0;i<4;++i)package[off+i]=(uint8_t)(x>>(8*i));}
static void seal(void){put(44,0);put(44,smku_crc(package,SMKU_HEADER,~0u)^~0u);}
static void setup(void){
    mode=0;erases=programs=reads=fetches=0;
    for(unsigned i=0;i<sizeof flash;++i)flash[i]=(uint8_t)(i*7+i/251);
#ifdef TEST_PUBLIC
    memcpy(flash+0xc1000+548,"trimdat",8);
    for(unsigned k=0;k<37;++k)for(unsigned j=0;j<5;++j){
        unsigned a=0xc1000+168+k*10+j*2,v=5+j*200;
        flash[a]=v;flash[a+1]=v>>8;
    }
#endif
    memcpy(original,flash,sizeof flash);memset(package,0,SMKU_HEADER);
    memcpy(package,"SMK37-SALT-USB1",15);put(16,1);put(20,SMKU_LO);put(24,SMKU_HI);put(28,SMKU_BYTES);
    headcrc=smku_crc(flash,0x4000,~0u)^~0u;trimcrc=smku_crc(flash+0xc1000,556,~0u)^~0u;
    put(32,headcrc);put(36,trimcrc);memcpy(package+48,"SMK-37_911",10);memcpy(package+80,"SMK37 TEST11 USB",15);
    memcpy(package+SMKU_HEADER,flash+SMKU_LO,SMKU_BYTES);package[SMKU_HEADER+100]^=1;
    put(40,smku_crc(package+SMKU_HEADER,SMKU_BYTES,~0u)^~0u);
    for(unsigned i=0;i<SMKU_SECTORS;++i)put(128+i*4,smku_crc(package+SMKU_HEADER+i*4096,4096,~0u)^~0u);
    seal();
}
static void protected_ok(void){assert(!memcmp(flash,original,SMKU_LO));assert(!memcmp(flash+SMKU_HI,original+SMKU_HI,sizeof flash-SMKU_HI));}
int main(int argc,char **argv){
    if(argc==3){
        FILE *f=fopen(argv[1],"rb");assert(f);assert(fread(flash,1,sizeof flash,f)==sizeof flash);assert(fgetc(f)==EOF);fclose(f);
        f=fopen(argv[2],"rb");assert(f);assert(fread(package,1,sizeof package,f)==sizeof package);assert(fgetc(f)==EOF);fclose(f);
        memcpy(original,flash,sizeof flash);
        headcrc=smku_crc(flash,0x4000,~0u)^~0u;trimcrc=smku_crc(flash+0xc1000,556,~0u)^~0u;
        unsigned changes=0;for(unsigned i=0;i<SMKU_SECTORS;++i)if(memcmp(flash+SMKU_LO+i*4096,package+SMKU_HEADER+i*4096,4096))++changes;
        assert(changes>0);assert(smku_session()==0);assert(erases==changes);protected_ok();
        assert(!memcmp(flash+SMKU_LO,package+SMKU_HEADER,SMKU_BYTES));
        printf("Real candidate format/loader integration passed: %u changed sectors; saved tail preserved\n",changes);return 0;
    }

    setup();assert(smku_session()==0);assert(erases==1 && programs==16);assert(!memcmp(flash+SMKU_LO,package+SMKU_HEADER,SMKU_BYTES));protected_ok();
    erases=programs=fetches=0;assert(smku_session()==0 && !erases && !programs);
    setup();mode=6;assert(smku_session()<0 && !erases);
    setup();package[0]^=1;assert(smku_session()<0 && !erases);
    setup();put(24,0xf3000);seal();assert(smku_session()<0 && !erases);
    setup();package[36]^=1;seal();assert(smku_session()<0 && !erases);
    setup();flash[0xc1000]^=1;assert(smku_session()<0 && !erases);
    setup();package[SMKU_HEADER+SMKU_BYTES-1]^=1;assert(smku_session()<0 && !erases);
    setup();mode=3;assert(smku_session()<0 && !erases);
    setup();mode=4;assert(smku_session()<0 && !erases);
    setup();mode=5;assert(smku_session()<0 && erases==1);protected_ok();
    mode=0;fetches=0;assert(smku_session()==0);protected_ok();
#ifdef TEST_PUBLIC
    /* The same portable package accepts each instrument's valid calibration. */
    for(unsigned unit=0;unit<3;++unit){
        setup();put(16,2);put(36,0);seal();
        flash[0xc1000+168]+=unit;
        memcpy(original,flash,sizeof flash);
        assert(smku_session()==0);protected_ok();
    }
    setup();put(16,2);put(36,0);seal();flash[0xc1000+548]=0;
    assert(smku_session()<0&&!erases);
    setup();put(16,2);put(36,0);seal();flash[100]^=1;
    assert(smku_session()<0&&!erases);
    setup();put(16,2);put(36,1);seal();assert(smku_session()<0&&!erases);
#else
    setup();put(16,2);put(36,0);seal();assert(smku_session()<0&&!erases);
#endif
    puts("SMK USB updater: success, no-op, foreign/corrupt headers, full preflight, changed host data, failure/retry and protected ranges passed");
}
