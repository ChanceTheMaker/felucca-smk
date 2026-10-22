/* SPDX-License-Identifier: GPL-3.0-only
 * Felucca SMK browser peripherals. Native SMK sound/UI/editor code, no USB,
 * GPIO, radio or physical flash driver. Storage below is WASM memory only. */
#include <stdint.h>
#define SMK37_SYNTH_HOST_TEST 1
#define SMK37_ENABLE_SAVES 1
#define SMK37_ENABLE_PRESETS 1
#define SMK37_EXPANDED_PRESETS 1
#define SMK37_ENABLE_SAMPLES 1
#include "../../firmware/src/smk37_synth.c"
static void midi_out_event(uint32_t p){(void)p;}
static void fm1_irq_off(void){}
static void fm1_irq_on(void){}
static uint8_t browser_flash[0x100000];
#include "../../firmware/src/smk37_session.c"
static int smk37_storage_read(uint32_t a,void *p,uint32_t n){if(a>sizeof browser_flash||n>sizeof browser_flash-a)return -1;memcpy(p,browser_flash+a,n);return 0;}
static int smk37_storage_erase(uint32_t a){if(a&4095u||a>sizeof browser_flash-4096)return -1;memset(browser_flash+a,255,4096);return 0;}
static int smk37_storage_program(uint32_t a,const void *p,uint32_t n){if(a>sizeof browser_flash||n>sizeof browser_flash-a)return -1;for(unsigned i=0;i<n;i++)browser_flash[a+i]&=((const uint8_t *)p)[i];return 0;}
static const uint8_t *smk37_sample_xip(unsigned k){return k<3?browser_flash+smk37_sample_address(k):smk37_empty_sample;}
static void smk37_sample_invalidate(unsigned k){(void)k;}
static int smk37_sample_format(void){smk37_samples_owned=1;return 0;}
#include "../../firmware/src/smk37_editor.c"
static unsigned smk37_analog_failed,smk37_light_failed,smk37_last_velocity=100;
static uint16_t screen[240*240];
static void lcd_sync(void){}
static void lcd_blit(uint32_t x,uint32_t y,uint32_t w,uint32_t h,const uint16_t *p){if(x+w>240||y+h>240)return;for(unsigned j=0;j<h;j++)memcpy(screen+(y+j)*240+x,p+j*w,w*2);}
#include "../../firmware/src/gfx.c"
#include "../../firmware/src/smk37_draw.c"
#define API __attribute__((visibility("default")))
static int32_t output[CTL*2];
static uint64_t frames;
static uint8_t request[2048];
static uint32_t status[16];
static uint16_t analog[6]={0,0,0,0,528,411};
static uint64_t held_keys;
static uint16_t held_pads;
API void synth_init(void){
    memset(browser_flash,255,sizeof browser_flash);smk37_synth_init();
    smk37_storage_ready=smk37_storage_writable=smk37_samples_owned=1;
    smk37_up_boot();for(unsigned k=0;k<3;k++)smp_user_scan(k);
    smk37_palette_apply();font_set(smk37_bold);smk37_analog_playback(analog,15);
}
static void service(void){
    smk37_hold_poll();smk37_performance_poll();
    if(smk37_prefs_request&&smk37_up_idle()){smk37_prefs_request=0;smk37_preferences_save();}
    if(smk37_rename_request){smk37_rename_request=0;smk37_preset_rename();}
    if(smk37_song_request){smk37_song_request=0;smk37_chain_prepare();}
    if(smk37_user_request){unsigned r=smk37_user_request;smk37_user_request=0;int rc=(r&3)==1?smk37_up_load(r>>2):smk37_up_store(r>>2,0);smk37_notice=rc?"STOP / RELEASE KEYS FIRST":(r&3)==1?"USER PRESET LOADED":"BROWSER PRESET SAVED";smk37_notice_at=fm1_ms;}
    if(smk37_project_request){unsigned r=smk37_project_request;smk37_project_request=0;int rc=(r&3)==1?smk37_session_load(r>>2):smk37_session_save(r>>2);smk37_notice=rc?"STOP / RELEASE KEYS FIRST":(r&3)==1?"PROJECT LOADED":"BROWSER PROJECT SAVED";smk37_notice_at=fm1_ms;}
    smk37_slice_service();if(smk37_samples_owned)slc_store_poll();
}
API int32_t *synth_render(void){fm1_ms=(uint32_t)(frames*1000/FS);service();fm6_poll();mix_block(output,CTL);frames+=CTL;return output;}
API void synth_midi(unsigned st,unsigned a,unsigned b){
    if(st>=0x80&&st<0xf0&&a<128&&b<128)midi_event(st&0xf0,st&15,a,b);
}
API void fw_event(unsigned kind,unsigned id,int value){
    if(kind==0&&id<18)smk37_button_input(id,!!value);
    if(kind==1&&id<8){int d=value<0?-1:1;unsigned n=(unsigned)(value<0?-value:value);if(n>32)n=32;while(n--)smk37_turn(id,d);}
    if(kind==2&&id<37){value=clamp(value,0,127);if(value)held_keys|=(uint64_t)1<<id;else held_keys&=~((uint64_t)1<<id);smk37_play_key_event(id,value);}
    if(kind==3&&id<16){value=clamp(value,0,127);if(value)held_pads|=1u<<id;else held_pads&=~(1u<<id);smk37_pad_event(id,value);}
    if(kind==4&&id<4){smk37_analog_playback(analog,15);analog[id]=clamp(value,0,1023);smk37_analog_playback(analog,15);}
    if(kind==5){if(!id)smk37_local_bend=clamp(value,-8192,8191);else smk37_local_mod=clamp(value,0,127);unsigned targets=1u<<song.sel;for(unsigned k=0;k<FELUCCA_KEY_COUNT;k++)if(kb_chn[k])targets|=1u<<kb_trk[k];smk37_wheels(smk37_local_bend,smk37_local_mod,targets);}
    if(kind==6)smk37_sustain(!!value);
    service();
}
API void fw_release(void){
    smk37_sustain(0);for(unsigned k=0;k<37;k++)smk37_play_key_event(k,0);for(unsigned k=0;k<16;k++)smk37_pad_event(k,0);
    held_keys=held_pads=smk37_button_down=smk37_button_held=0;seq_fill=smk37_fill_down=0;
    for(unsigned ch=0;ch<16;ch++){midi_control(ch,64,0);midi_control(ch,120,0);}
    smk37_local_bend=0;smk37_wheels(0,smk37_local_mod,15);
}
API uint16_t *fw_screen(void){smk37_palette_apply();font_set(smk37_bold);smk37_sound_status();return screen;}
API uint32_t *fw_status(void){status[0]=smk37_indicator_status();status[1]=(uint32_t)held_keys;status[2]=(uint32_t)(held_keys>>32);status[3]=held_pads;status[4]=smk37_page;status[5]=song.sel;status[6]=song.playing;status[7]=smk37_knob_bank;status[8]=smk37_fader_bank;status[9]=song.octave+3;status[10]=smk37_pad_bank;status[11]=song.master_q12;for(unsigned i=0;i<4;i++)status[12+i]=(smk37_fader_bank?TSEL->p[P_ATK+i]:trk[i].p[P_LEVEL])*1023u/127u;return status;}
API uint8_t *fw_request(void){return request;}
API uint8_t *fw_reply(void){return smk37_ed_out;}
API unsigned fw_command(unsigned n){
    if(n<4||n>sizeof request)return 0;
    unsigned rc;
    if(request[0]!=125||request[1]!=70||request[2]!=76)return 0;
    if(request[3]==1&&n==4){rc=smk37_editor_handle(request,n);/* Device-compatible capabilities, honest browser identity. */
        unsigned end=5;while(end<rc&&smk37_ed_out[end])end++;
        static const char name[]="FELUCCA SMK37 BROWSER";unsigned len=sizeof name-1,old=end-5;
        if(len<=old){for(unsigned i=end+1;i<rc;i++)smk37_ed_out[i+len-old]=smk37_ed_out[i];memcpy(smk37_ed_out+5,name,len);smk37_ed_out[5+len]=0;rc-=old-len;}return rc;
    }
    if(request[3]==9&&n==6&&request[4]<=2&&request[5]<4){unsigned op=request[4],k=request[5];int err=op==0?smk37_session_load(k):op==1?smk37_session_save(k):0;if(err)return 0;smk37_ed_begin(9);smk37_ed_byte(op);smk37_ed_byte(k);smk37_ed_byte(smk37_session_used(k));return smk37_ed_finish();}
    rc=smk37_editor_main_command(request,n);if(!rc)rc=smk37_editor_handle(request,n);service();return rc;
}
API unsigned fw_changed(void){return smk37_editor_changed();}
/* Browser-private state: native session + virtual flash. Never a firmware file. */
static uint8_t snapshot[sizeof(smk37_session_t)+sizeof browser_flash];
API uint8_t *fw_snapshot(void){smk37_session_capture();memcpy(snapshot,&smk37_session,sizeof smk37_session);memcpy(snapshot+sizeof smk37_session,browser_flash,sizeof browser_flash);return snapshot;}
API unsigned fw_snapshot_size(void){return sizeof snapshot;}
API int fw_restore(void){
    memcpy(&smk37_session,snapshot,sizeof smk37_session);if(!smk37_session_valid())return -1;
    fw_release();memcpy(browser_flash,snapshot+sizeof smk37_session,sizeof browser_flash);
    smk37_up_boot();smk37_prefs_restore();for(unsigned k=0;k<3;k++){smp_user_scan(k);slc_user_scan(k,0);}return smk37_session_restore();
}
