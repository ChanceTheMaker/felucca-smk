/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>
/* Multi-sector A/B objects. No hardware driver or
 * format-on-boot path is provided here. A caller must validate ownership and
 * migration before granting write permission. The preset integration grants it only for blank or CRC-valid owned pairs. */
typedef struct {
    uint32_t a,b,sectors,kind;
} smk37_object_t;
typedef struct {
    uint32_t magic,version,kind,sequence,length,crc,layout,header_crc;
} smk37_object_header_t;
#define SMK37_OBJECT_LAYOUT 0x20261023u
static const smk37_object_t smk37_object_presets={0xb1000,0xb4000,3,1};
static const smk37_object_t smk37_object_fm6={0xb7000,0xb9000,2,2};
static int smk37_object_known(const smk37_object_t *o){
    return (o->a==0xb1000 && o->b==0xb4000 && o->sectors==3 && o->kind==1) ||
           (o->a==0xb7000 && o->b==0xb9000 && o->sectors==2 && o->kind==2);
}
static uint32_t smk37_object_crc_step(uint32_t c,const void *data,uint32_t n){
    const uint8_t *p=data;
    while(n--){c^=*p++;for(unsigned b=0;b<8;++b)c=(c>>1)^((0u-(c&1u))&0xedb88320u);}return c;
}
static uint32_t smk37_object_crc(const void *p,uint32_t n){return ~smk37_object_crc_step(~0u,p,n);}
/* Leave every sector's final 256 bytes erased for the vendor boot scanner. */
static uint32_t smk37_object_payload(uint32_t base,uint32_t pos){
    pos+=256u;return base+(pos/3840u)*4096u+pos%3840u;
}
static int smk37_object_check(const smk37_store_io_t *io,const smk37_object_t *o,unsigned copy,uint32_t length,smk37_object_header_t *h){
    if(!smk37_object_known(o)||copy>1||!length||length>o->sectors*3840u-256u)return -1;
    uint32_t base=copy?o->b:o->a;
    if(io->read(base,h,sizeof *h)||h->magic!=0x324f4d53u||h->version!=1||h->kind!=o->kind||h->length!=length||h->layout!=SMK37_OBJECT_LAYOUT||
       h->header_crc!=smk37_object_crc(h,28))return -1;
    uint8_t block[256];uint32_t crc=~0u;
    for(uint32_t pos=0;pos<length;){uint32_t n=length-pos;if(n>256)n=256;
        if(io->read(smk37_object_payload(base,pos),block,n))return -1;crc=smk37_object_crc_step(crc,block,n);pos+=n;}
    return h->crc==~crc?0:-1;
}
static int smk37_object_current(const smk37_store_io_t *io,const smk37_object_t *o,uint32_t length,uint32_t *sequence){
    smk37_object_header_t a,b;int va=!smk37_object_check(io,o,0,length,&a),vb=!smk37_object_check(io,o,1,length,&b);
    if(vb&&(!va||(a.sequence!=b.sequence&&b.sequence-a.sequence<0x80000000u))){if(sequence)*sequence=b.sequence;return 1;}
    if(va){if(sequence)*sequence=a.sequence;return 0;}return -1;
}
static int smk37_object_load(const smk37_store_io_t *io,const smk37_object_t *o,void *payload,uint32_t length){
    int c=smk37_object_current(io,o,length,0);if(c<0)return -1;
    uint8_t *p=payload;uint32_t base=c?o->b:o->a;
    for(uint32_t pos=0;pos<length;){uint32_t n=length-pos;if(n>256)n=256;
        if(io->read(smk37_object_payload(base,pos),p+pos,n))return -1;pos+=n;}
    return c;
}
static int smk37_object_save(const smk37_store_io_t *io,const smk37_object_t *o,const void *payload,uint32_t length,int owned){
    if(!owned||!smk37_object_known(o)||!payload||!length||length>o->sectors*3840u-256u)return -1;
    uint32_t sequence=0;int current=smk37_object_current(io,o,length,&sequence);unsigned copy=current==0?1:0;
    uint32_t base=copy?o->b:o->a;const uint8_t *p=payload;uint8_t verify[256];
    for(unsigned sector=0;sector<o->sectors;++sector)if(io->erase(base+sector*4096u))return -1;
    for(uint32_t pos=0;pos<length;){uint32_t n=length-pos;if(n>256)n=256;
        if(io->program(smk37_object_payload(base,pos),p+pos,n)||io->read(smk37_object_payload(base,pos),verify,n)||memcmp(p+pos,verify,n))return -1;pos+=n;}
    smk37_object_header_t h={0x324f4d53u,1,o->kind,sequence+1,length,smk37_object_crc(payload,length),SMK37_OBJECT_LAYOUT,0};
    h.header_crc=smk37_object_crc(&h,28);
    if(io->program(base,&h,sizeof h))return -1;
    return smk37_object_check(io,o,copy,length,&h);
}

/* Read-only ownership check. Unknown or interrupted initial data is not erased. */
static int smk37_object_owned(const smk37_store_io_t *io,const smk37_object_t *o,uint32_t length){
    if(!smk37_object_known(o))return 0;
    if(smk37_object_current(io,o,length,0)>=0)return 1;
    uint8_t block[256];
    for(unsigned copy=0;copy<2;++copy)for(uint32_t pos=0;pos<o->sectors*4096u;pos+=256){
        if(io->read((copy?o->b:o->a)+pos,block,256))return 0;
        for(unsigned i=0;i<256;++i)if(block[i]!=255)return 0;
    }return 1;
}
