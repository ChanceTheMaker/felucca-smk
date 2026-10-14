/* SPDX-License-Identifier: GPL-3.0-only
 * Passive 240x240 LCD copy, 4 bits/pixel (28,800 bytes), enabled only by a
 * negotiated browser lease. Geometry comes from the actual LCD blits. Colors
 * are quantized to the device's 16 UI tokens; no second UI or audio engine. */
#if SMK37_REMOTE_PANEL
static uint8_t smk37_remote_pixels[240*120] __attribute__((section(".pool")));
static uint16_t smk37_remote_palette[16],smk37_remote_cache_color[256];
static uint8_t smk37_remote_cache_index[256],smk37_remote_cache_valid[256];
static uint32_t smk37_remote_palette_gen=~0u;
/* Nine MIDI-safe bytes cover the sixty four-row strips. No extra framebuffer. */
static unsigned smk37_remote_page=~0u;
static uint8_t smk37_remote_cursor;
static uint8_t smk37_remote_dirty[9],smk37_remote_left[60],smk37_remote_right[60];
static void smk37_remote_dirty_all(void){memset(smk37_remote_dirty,127,8);smk37_remote_dirty[8]=15;memset(smk37_remote_left,0,60);memset(smk37_remote_right,240,60);}
static void smk37_remote_blit(unsigned x,unsigned y,unsigned w,unsigned h,const uint16_t *p){
    if(!smk37_remote_live||fm1_ms-smk37_remote_at>2000||x+w>240||y+h>240)return;
    if(smk37_remote_page!=smk37_page){smk37_remote_page=smk37_page;smk37_remote_cursor=0;}
    if(smk37_remote_palette_gen!=ux.gen){
        smk37_remote_cursor=0;
        const uint16_t colors[16]={T_BG,T_SURF,T_TEXT,T_THEME,T_ACCENT,T_MID,T_DIM,T_LINE,T_SEL,T_TINT,T_INK,T_REC,T_RAISE,T_KEY,T_LANE,T_GRID};
        memcpy(smk37_remote_palette,colors,sizeof colors);memset(smk37_remote_cache_valid,0,sizeof smk37_remote_cache_valid);
        smk37_remote_palette_gen=ux.gen;
        smk37_remote_dirty_all();
    }
    for(unsigned j=0;j<h;j++)for(unsigned i=0;i<w;i++){
        uint16_t c=swap16(p[j*w+i]);unsigned hash=(c^(c>>8))&255,index=0;
        if(smk37_remote_cache_valid[hash]&&smk37_remote_cache_color[hash]==c)index=smk37_remote_cache_index[hash];
        else{
            unsigned best=~0u;
            for(unsigned k=0;k<16;k++){
                uint16_t t=smk37_remote_palette[k];int r=(c>>11)-(t>>11),g=((c>>5)&63)-((t>>5)&63),b=(c&31)-(t&31);
                unsigned d=2*r*r+g*g+2*b*b;if(d<best){best=d;index=k;}if(!d)break;
            }
            smk37_remote_cache_valid[hash]=1;smk37_remote_cache_color[hash]=c;smk37_remote_cache_index[hash]=index;
        }
        unsigned pos=(y+j)*240+x+i;uint8_t *dst=&smk37_remote_pixels[pos>>1];
        uint8_t next=(pos&1)?(*dst&15)|(index<<4):(*dst&240)|index;
        if(next!=*dst){
            *dst=next;unsigned tile=(y+j)/4,bit=1u<<(tile%7),column=x+i;
            if(!(smk37_remote_dirty[tile/7]&bit)){smk37_remote_left[tile]=column;smk37_remote_right[tile]=column+1;}
            else{if(column<smk37_remote_left[tile])smk37_remote_left[tile]=column;if(column>=smk37_remote_right[tile])smk37_remote_right[tile]=column+1;}
            smk37_remote_dirty[tile/7]|=bit;
        }
    }
}
static void smk37_remote_uint(uint32_t v,unsigned n){while(n--){smk37_ed_byte(v);v>>=7;}}
static void smk37_remote_state(void){
        uint32_t lo=0,hi=0;unsigned pads=0;
        for(unsigned i=0;i<37;i++)if(smk37_surface_keys[0][i]||smk37_surface_keys[1][i]){if(i<32)lo|=1u<<i;else hi|=1u<<(i-32);}
        for(unsigned i=0;i<16;i++)if(smk37_surface_pads[0][i]||smk37_surface_pads[1][i])pads|=1u<<i;
        smk37_remote_uint(smk37_remote_led_buttons,3);smk37_remote_uint(lo,5);smk37_remote_uint(hi,1);
        smk37_remote_uint(pads,3);smk37_remote_uint(smk37_remote_led_pads,3);
        smk37_ed_byte(smk37_remote_led_color);smk37_remote_uint(smk37_remote_led_level,2);
        smk37_ed_byte(smk37_page);smk37_ed_byte(song.sel);smk37_ed_byte(song.playing);smk37_ed_byte(song.rec);
        smk37_ed_byte(smk37_knob_bank);smk37_ed_byte(smk37_fader_bank);smk37_ed_value(song.octave);
        smk37_ed_byte(smk37_pad_bank);smk37_ed_value(song.master_q12);
        for(unsigned i=0;i<4;i++)smk37_ed_value((smk37_fader_bank?TSEL->p[P_ATK+i]:trk[i].p[P_LEVEL])*1023/127);
        smk37_ed_value(smk37_local_bend);smk37_ed_byte(smk37_local_mod);smk37_ed_byte(smk37_pedal_down);
}
static unsigned smk37_remote_region_runs(const uint8_t *p,unsigned x,unsigned width,unsigned emit){
    unsigned bytes=0,run=0,last=0;
    for(unsigned row=0;row<4;row++)for(unsigned col=x;col<x+width;col++){
        unsigned pos=row*240+col,color=(p[pos>>1]>>((pos&1)*4))&15;
        if(run&&(color!=last||run==127)){if(emit){smk37_ed_byte(run);smk37_ed_byte(last);}bytes+=2;run=0;}
        last=color;++run;
    }
    if(run){if(emit){smk37_ed_byte(run);smk37_ed_byte(last);}bytes+=2;}
    return bytes;
}
static unsigned smk37_remote_runs(const uint8_t *p,unsigned emit){return smk37_remote_region_runs(p,0,240,emit);}
static unsigned smk37_remote_region_byte(const uint8_t *p,unsigned x,unsigned width,unsigned i){return width==240?p[i]:p[(i/(width/2))*120+x/2+i%(width/2)];}
static void smk37_remote_region_raw(const uint8_t *p,unsigned x,unsigned width){
    unsigned total=width*2;
    for(unsigned i=0;i<total;i+=7){unsigned len=total-i<7?total-i:7,mask=0;for(unsigned k=0;k<len;k++)mask|=(smk37_remote_region_byte(p,x,width,i+k)>>7)<<k;smk37_ed_byte(mask);for(unsigned k=0;k<len;k++)smk37_ed_byte(smk37_remote_region_byte(p,x,width,i+k));}
}
static void smk37_remote_raw(const uint8_t *p){smk37_remote_region_raw(p,0,240);}
static unsigned smk37_remote_command(const uint8_t *f,unsigned n){
    if(n<9||f[0]!=125||f[1]!=70||f[2]!=76||f[3]!=126||f[4]!=83||f[5]!=77||f[6]!=1)return 0;
    for(unsigned i=0;i<n;i++)if(f[i]&128)return 0;
    unsigned op=f[7];
    if(((op==0||op==3)&&n!=9)||(op==1&&n!=13)||((op==2||op==4)&&(n!=10||f[9]>=60))||(op==5&&(n!=18||f[17]>15))||(op==6&&(n!=19||f[17]>15||f[18]>1))||op>6)return 0;
    smk37_ed_begin(126);smk37_ed_byte(83);smk37_ed_byte(77);smk37_ed_byte(1);smk37_ed_byte(op);smk37_ed_byte(f[8]);
    fm1_irq_off();if(!smk37_remote_live){smk37_remote_refresh=1;smk37_remote_dirty_all();}smk37_remote_live=1;smk37_remote_at=fm1_ms;
    if(op==1){unsigned rc=smk37_remote_enqueue(f[9],f[10],smk37_ed_read(f+11));fm1_irq_on();smk37_ed_byte(rc);return smk37_ed_finish();}
    if(op==0){smk37_remote_state();fm1_irq_on();return smk37_ed_finish();}
    if(op==3){for(unsigned i=0;i<9;i++)smk37_ed_byte(smk37_remote_dirty[i]);fm1_irq_on();return smk37_ed_finish();}
    if(op==5||op==6){
        /* One state snapshot + palette + as many strips as fit. Main-loop LCD
           capture and encoding cannot interleave; audio/USB interrupts stay on. */
        uint8_t pending[9];for(unsigned i=0;i<9;i++)pending[i]=f[9+i]|smk37_remote_dirty[i];
        smk37_remote_state();fm1_irq_on();
        for(unsigned i=0;i<16;i++)smk37_remote_uint(smk37_remote_palette[i],3);
        unsigned map_at=smk37_ed_n;for(unsigned i=0;i<9;i++)smk37_ed_byte(0);
        unsigned count_at=smk37_ed_n,count=0;smk37_ed_byte(0);
        for(unsigned scanned=0;scanned<60&&count<(op==6?60u:16u);scanned++){
            unsigned tile=smk37_remote_cursor;smk37_remote_cursor=(smk37_remote_cursor+1)%60;
            if(!(pending[tile/7]&(1u<<(tile%7))))continue;
            const uint8_t *p=smk37_remote_pixels+tile*480;
            unsigned x=0,width=240;
            if(op==6&&!f[18]&&(smk37_remote_dirty[tile/7]&(1u<<(tile%7)))){
                x=smk37_remote_left[tile]&~1u;width=((smk37_remote_right[tile]+1u)&~1u)-x;
            }
            unsigned raw=width*2+(width*2+6)/7,runs=smk37_remote_region_runs(p,x,width,0),rle=runs<raw,len=rle?runs:raw;
            if(smk37_ed_n+(op==6?6:4)+len+1>1536){smk37_remote_cursor=tile;break;}
            smk37_remote_dirty[tile/7]&=~(1u<<(tile%7));pending[tile/7]&=~(1u<<(tile%7));
            smk37_ed_byte(tile);if(op==6){smk37_ed_byte(x/2);smk37_ed_byte(width/2);}
            smk37_ed_byte(rle);smk37_remote_uint(len,2);
            if(rle)smk37_remote_region_runs(p,x,width,1);else smk37_remote_region_raw(p,x,width);
            ++count;
        }
        for(unsigned i=0;i<9;i++)smk37_ed_out[map_at+i]=pending[i]|smk37_remote_dirty[i];
        smk37_ed_out[count_at]=count;return smk37_ed_finish();
    }
    /* Clear before encoding so a later redraw remains marked for retransmit. */
    smk37_remote_dirty[f[9]/7]&=~(1u<<(f[9]%7));
    fm1_irq_on();smk37_ed_byte(f[9]);
    const uint8_t *p=smk37_remote_pixels+f[9]*480;
    unsigned rle=0;
    if(op==4){rle=smk37_remote_runs(p,0)<549;smk37_ed_byte(rle);}
    for(unsigned i=0;i<16;i++)smk37_remote_uint(smk37_remote_palette[i],3);
    if(rle)smk37_remote_runs(p,1);else smk37_remote_raw(p);
    return smk37_ed_finish();
}
#endif
