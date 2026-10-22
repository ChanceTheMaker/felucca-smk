/* SPDX-License-Identifier: GPL-3.0-only */
#define SMK37_REMOTE_PANEL 1
#define SMK37_ENABLE_USB_UPDATE 1
#define SMK37_PUBLIC_BUILD 1
#define SMK37_UPDATE_VERSION "Felucca SMK 0.9.27 Beta"
#define main baseline_main
#include "smk37_completion_test.c"
#undef main
#include "../firmware/src/smk37_remote_input.c"
static unsigned smk37_analog_failed,smk37_light_failed,smk37_last_velocity=100;
static uint16_t actual[240*240];
static void smk37_remote_blit(unsigned,unsigned,unsigned,unsigned,const uint16_t *);
static void lcd_sync(void){}
static void lcd_blit(uint32_t x,uint32_t y,uint32_t w,uint32_t h,const uint16_t *p){
    assert(!locked && x+w<=240 && y+h<=240);
    for(unsigned r=0;r<h;r++)memcpy(actual+(y+r)*240+x,p+r*w,w*2);
    smk37_remote_blit(x,y,w,h,p);
}
#include "../firmware/src/gfx.c"
#include "../firmware/src/smk37_remote_screen.c"
#include "../firmware/src/smk37_draw.c"
static unsigned query(unsigned op,unsigned arg){
    uint8_t f[]={125,70,76,126,83,77,1,op,19,arg};
    return smk37_remote_command(f,(op==2||op==4)?10:9);
}
static unsigned batch(const uint8_t *mask){
    uint8_t f[18]={125,70,76,126,83,77,1,5,21};memcpy(f+9,mask,9);
    return smk37_remote_command(f,sizeof f);
}
static unsigned rectangles(const uint8_t *mask,unsigned force){
    uint8_t f[19]={125,70,76,126,83,77,1,6,22};memcpy(f+9,mask,9);f[18]=force;
    return smk37_remote_command(f,sizeof f);
}
static void event(unsigned kind,unsigned id,int v){
    unsigned u=v+8192;uint8_t f[]={125,70,76,126,83,77,1,1,20,kind,id,u&127,u>>7};
    assert(smk37_remote_command(f,sizeof f)==12 && smk37_ed_out[10]==0);
    while(smk37_remote_r!=smk37_remote_w)smk37_remote_poll();
}
static void output(FILE *f){
    fputc('[',f);for(unsigned i=5;i<smk37_ed_n-1;i++)fprintf(f,"%s%u",i==5?"":",",smk37_ed_out[i]);fputs("]",f);
}
int main(void){
    smk37_synth_init();memset(flash,255,sizeof flash);
    assert(query(0,0)==52 && smk37_remote_live && smk37_remote_refresh);
    smk37_page=SMK37_TOOLS_PAGE;smk37_tool=TL_N;event(0,3,1);assert(seq_fill);event(7,0,0);assert(!seq_fill && !smk37_fill_down);
    event(0,3,1);smk37_surface_button(0,3,1);event(7,0,0);assert(seq_fill);smk37_surface_button(0,3,0);assert(!seq_fill);
    smk37_page=0;
    event(2,36,100);assert(kb_chn[36]);
    smk37_surface_key(0,36,90);event(2,36,0);assert(kb_chn[36]);
    smk37_surface_key(0,36,0);assert(!kb_chn[36]);
    event(3,0,110);assert(kb_chn[37] && (smk37_pad_seen&1));
    smk37_surface_pad(0,0,90);event(7,0,0);assert(kb_chn[37]);
    smk37_surface_pad(0,0,0);assert(!kb_chn[37]);
    unsigned bank=smk37_knob_bank;event(0,0,1);event(0,0,0);assert(smk37_knob_bank!=bank);
    smk37_knob_bank=0;int master=song.master_q12;event(1,0,-2);assert(song.master_q12==master-64);
    event(4,0,1023);assert(trk[0].p[P_LEVEL]==127);
    event(5,0,4096);event(5,1,93);assert(smk37_local_bend==4096&&smk37_local_mod==93);
    event(6,0,1);assert(smk37_remote_pedal);event(7,0,0);assert(!smk37_remote_pedal);
    /* Expiry releases only the browser; a held button must not become a tap. */
    smk37_surface_key(0,4,100);event(2,5,100);event(0,12,1);
    unsigned page=smk37_page;fm1_ms+=2001;smk37_remote_poll();
    assert(kb_chn[4]&&!kb_chn[5]&&!smk37_remote_live&&smk37_page==page&&!smk37_button_down);
    smk37_surface_key(0,4,0);
    assert(!query(2,60));
    assert(smk37_remote_enqueue(3,16,100)==1 && smk37_remote_enqueue(5,0,8192)==1);
    for(unsigned i=0;i<32;i++)assert(!smk37_remote_enqueue(2,1,0));
    assert(smk37_remote_enqueue(2,1,0)==2);smk37_remote_r=smk37_remote_w;
    assert(query(0,0));palette_set(0);font_set(0);smk37_page=3;
    cv_begin(240,120,T_BG);cv_blit(0,0);cv_blit(0,120);smk37_sound_status();
    /* Every transmitted pixel is the nearest UI palette color to the real LCD,
       not a separately reconstructed page. */
    for(unsigned i=0;i<240*240;i++){
        uint16_t c=swap16(actual[i]);unsigned nearest=0,best=~0u;
        for(unsigned k=0;k<16;k++){unsigned t=smk37_remote_palette[k];int r=(c>>11)-(t>>11),g=((c>>5)&63)-((t>>5)&63),b=(c&31)-(t&31);unsigned d=2*r*r+g*g+2*b*b;if(d<best){best=d;nearest=k;}}
        assert(((smk37_remote_pixels[i>>1]>>((i&1)*4))&15)==nearest);
    }
    /* Browser fixture generated from the real firmware's screen and wire replies. */
    FILE *f=fopen("build/smk37-tests/remote-wire.json","w");assert(f);fputs("{\"info\":",f);
    const uint8_t info[]={125,70,76,1};assert(smk37_editor_handle(info,sizeof info));output(f);fputs(",\"palette\":",f);assert(req(80,0,0));output(f);fputs(",\"menus\":[",f);
    for(uint8_t i=0;i<SMK37_MENU_COUNT;i++){if(i)fputc(',',f);assert(req(72,&i,1));output(f);}fputs("],\"state\":",f);
    smk37_surface_key(0,0,100);smk37_surface_key(0,36,100);smk37_surface_pad(0,2,100);
    smk37_remote_led_pads=4;smk37_remote_led_buttons=1u<<12;smk37_remote_led_color=2;smk37_remote_led_level=8;
    query(0,0);output(f);fputs(",\"tiles\":[",f);
    for(unsigned t=0;t<60;t++){if(t)fputc(',',f);assert(query(2,t)==609);output(f);}
    fputs("],\"compactTiles\":[",f);unsigned wirebytes=0;
    for(unsigned t=0;t<60;t++){if(t)fputc(',',f);unsigned n=query(4,t);assert(n<=610);wirebytes+=n;output(f);}
    fputs("],\"dirty\":",f);assert(query(3,0)==20);output(f);
    fputs(",\"batches\":[",f);
    uint8_t mask[9];memset(mask,127,8);mask[8]=15;unsigned batches=0,batchbytes=0,tiles=0;
    while(tiles<60){
        unsigned n=batch(mask);assert(n>=110&&n<=1536&&smk37_ed_out[108]);
        if(batches)fputc(',',f);output(f);batchbytes+=n;tiles+=smk37_ed_out[108];
        memcpy(mask,smk37_ed_out+99,9);assert(++batches<=30);
    }
    assert(tiles==60);for(unsigned i=0;i<9;i++)assert(!mask[i]);
    fputs("],\"rectBatches\":[",f);
    memset(mask,127,8);mask[8]=15;unsigned recttiles=0,rectcount=0;
    while(recttiles<60){
        assert(rectangles(mask,1)<=1536&&smk37_ed_out[108]);
        if(rectcount++)fputc(',',f);output(f);recttiles+=smk37_ed_out[108];
        memcpy(mask,smk37_ed_out+99,9);assert(rectcount<=30);
    }
    assert(recttiles==60);
    /* A moving sequencer cursor must transmit narrow columns and preserve
       all surrounding pixels. Exercise both RLE and packed raw regions. */
    uint16_t stripe[4*104];for(unsigned i=0;i<4*104;i++)stripe[i]=swap16(T_TEXT);
    lcd_blit(41,80,4,104,stripe);
    fputs("],\"rectChanges\":[",f);assert(rectangles(mask,0)<=1536);output(f);
    assert(smk37_ed_out[108]==26);unsigned narrowbytes=smk37_ed_n;
    for(unsigned i=0;i<9;i++)assert(!smk37_remote_dirty[i]);
    for(unsigned i=0;i<4*104;i++)stripe[i]=swap16(i&1?T_BG:T_TEXT);
    lcd_blit(41,80,4,104,stripe);assert(rectangles(mask,0)<=1536);fputc(',',f);output(f);
    fputs("],\"afterRectTiles\":[",f);
    for(unsigned t=0;t<60;t++){if(t)fputc(',',f);assert(query(2,t));output(f);}
    fputs("]}\n",f);fclose(f);
    printf("SMK moving-column update: %u bytes for 26 strips in one reply\n",narrowbytes);
    /* A page change restarts the scan from the top, not the previous cursor. */
    smk37_remote_cursor=35;smk37_page++;
    uint16_t same=actual[0];lcd_blit(0,0,1,1,&same);assert(!smk37_remote_cursor);
    memset(mask,127,8);mask[8]=15;assert(rectangles(mask,1)<=1536&&smk37_ed_out[109]==0);
    memset(mask,0,sizeof mask);while(smk37_ed_out[108])assert(rectangles(mask,0)<=1536);
    assert(batch(mask)==110&&smk37_ed_out[108]==0);
    for(unsigned i=0;i<9;i++)assert(!smk37_remote_dirty[i]);
    /* Repainting identical pixels sends nothing; a one-pixel change marks only
       its strip, including the far edge. Reading the map never clears it. */
    uint16_t pixel=swap16(T_TEXT);lcd_blit(239,239,1,1,&pixel);
    assert(smk37_remote_dirty[8]==8);assert(query(3,0)==20&&smk37_remote_dirty[8]==8);
    assert(query(4,59));for(unsigned i=0;i<9;i++)assert(!smk37_remote_dirty[i]);
    lcd_blit(239,239,1,1,&pixel);for(unsigned i=0;i<9;i++)assert(!smk37_remote_dirty[i]);
    pixel=swap16(T_BG);lcd_blit(239,239,1,1,&pixel);assert(smk37_remote_dirty[8]==8);
    /* Worst-case alternating pixels use bounded raw encoding; solid strips RLE. */
    memset(smk37_remote_pixels,0x10,480);assert(query(4,0)==610&&smk37_ed_out[11]==0);
    memset(smk37_remote_pixels,0,480);assert(query(4,0)==77&&smk37_ed_out[11]==1);
    palette_set(1);lcd_blit(0,0,1,1,&pixel);
    for(unsigned i=0;i<8;i++)assert(smk37_remote_dirty[i]==127);assert(smk37_remote_dirty[8]==15);
    assert(!query(4,60));
    printf("SMK screen fixture: %u compressed bytes vs %u legacy bytes\n",wirebytes,60*609);
    printf("SMK batched fixture: %u replies, %u bytes (including state snapshots)\n",batches,batchbytes);
    memset(smk37_remote_pixels,0x10,sizeof smk37_remote_pixels);smk37_remote_dirty_all();
    assert(batch(mask)<=1536&&smk37_ed_out[108]==2);
    smk37_page=23;song.playing=1;TSEL->seq_idx=0;smk37_sound_status();
    memset(mask,0,9);do{assert(rectangles(mask,0)<=1536);}while(smk37_ed_out[108]);
    TSEL->seq_idx=1;smk37_sound_status();
    uint8_t saved_dirty[9],saved_left[60],saved_right[60];
    memcpy(saved_dirty,smk37_remote_dirty,9);memcpy(saved_left,smk37_remote_left,60);memcpy(saved_right,smk37_remote_right,60);
    unsigned fullbytes=0,fullreplies=0,rectbytes=0,rectreplies=0;
    do{unsigned n=batch(mask);if(smk37_ed_out[108]){fullbytes+=n;fullreplies++;}}while(smk37_ed_out[108]);
    memcpy(smk37_remote_dirty,saved_dirty,9);memcpy(smk37_remote_left,saved_left,60);memcpy(smk37_remote_right,saved_right,60);
    do{unsigned n=rectangles(mask,0);if(smk37_ed_out[108]){rectbytes+=n;rectreplies++;}}while(smk37_ed_out[108]);
    assert(rectbytes&&rectbytes<fullbytes);
    printf("SMK actual sequencer step: %u bytes / %u replies vs full strips %u bytes / %u replies\n",rectbytes,rectreplies,fullbytes,fullreplies);
    assert(!locked);puts("SMK remote: ownership, lease, physical handlers, limits and LCD/wire fixture passed");
}
