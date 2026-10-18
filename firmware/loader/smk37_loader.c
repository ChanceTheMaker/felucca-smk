/* SPDX-License-Identifier: GPL-3.0-only
 * RAM-only SMK loader, entered from updater-enabled Salt. Reuses the original
 * Felucca USB transport and packet codec; no FM-1 staging or storage policy. */
#include <stdint.h>
#include "smk37_unit.h"
#define FELUCCA_LOADER 1
#define FELUCCA_CDC 0
#define FELUCCA_OTA 1
#define FELUCCA_USB_PID 0x0002
#define FELUCCA_ID "ota-" SMK37_UPDATE_PRODUCT
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
#include "fm1_time.h"
#include "fm1_sys.h"
#define FL_RANGE_OK(off,n) ((uint32_t)(off)>=0x4000u && (uint32_t)(off)<0x9d000u && (uint32_t)(n)>0 && (uint32_t)(n)<=0x9d000u-(uint32_t)(off))
#include "fm1_flash.h"
#include "smk37_flash_profile.h"
#include "../src/libc.c"
#include "../src/usb.c"
static uint32_t ota_now_ms(void){
    static uint32_t last,acc,ms;uint32_t now=fm1_ticks();acc+=now-last;last=now;
    ms+=acc/24000u;acc%=24000u;return ms;
}
static void ota_idle(void){fm1_wdt_feed();usb_poll();}
/* Unused stage-one hooks; never linked into this loader. */
static int ota_erase(uint32_t a){(void)a;return -1;}
static int ota_prog(uint32_t a,const void*p,uint32_t n){(void)a;(void)p;(void)n;return -1;}
static int ota_fread(uint32_t a,void*p,uint32_t n){return fl_read_ram(a,p,n);}
static void ota_show(uint32_t s,int32_t c){(void)s;(void)c;}
static void ota_commit(const uint8_t*p){(void)p;}
#include "../src/ota.c"
static int smku_flash_supported(uint32_t id){return smk37_flash_supported_id(id);}
static uint32_t smku_flash_id(void){return fl_jedec_ram();}
static int smku_read(uint32_t a,uint8_t*p,uint32_t n){return ota_read(a,p,n);}
static int smku_flash_read(uint32_t a,void*p,uint32_t n){return fl_read_ram(a,p,n);}
static int smku_flash_erase(uint32_t a){uint32_t t;return fl_erase4k_ram(a,&t);}
static int smku_flash_prog(uint32_t a,const void*p,uint32_t n){uint32_t t;return fl_prog_ram(a,p,n,&t);}
static void smku_poll(void){ota_idle();}
#include "smk37_update_core.c"
/* Versioned SMKU diagnostic inside the existing checksummed OTA envelope.
 * Distinct from read/identity commands; older hosts ignore it. */
static void smku_report(int rc){
    uint8_t body[12]={'S','M','K','U',1,(uint8_t)(-rc),(uint8_t)smku_write_started,0};
    ota_wr32(body+8,smku_observed_id);ota_send_msg(0x7e,body,sizeof body);
}
extern uint32_t _bss_start[],_bss_end[];
void ldr_cstart(void){
    for(uint32_t*p=_bss_start;p<_bss_end;++p)*p=0;
    fm1_wdt_arm(0x0d);fm1_time_init();usb_start();
    for(;;){
        ota_idle();usb_retry(ota_now_ms());ota_service();
        if(usb.ota_req){usb.ota_req=0;
            int rc=smku_session();if(rc)smku_report(rc);
            if(rc==0){fm1_delay_ms(200);usb_detach();fm1_delay_ms(30);fm1_core_reset();}
            /* On failure keep polling USB in RAM, allowing explicit retry. */
        }
        if(usb.uboot_req){usb_detach();fm1_delay_ms(30);fm1_enter_uboot();}
    }
}
