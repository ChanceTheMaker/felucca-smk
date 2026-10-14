/* SPDX-License-Identifier: GPL-3.0-only
 * Device-master panel extension. Inputs are consumed on the audio/control path,
 * alongside physical controls. A lost browser lease releases only its owners. */
#ifndef SMK37_REMOTE_PANEL
#define SMK37_REMOTE_PANEL 0
#endif
#if SMK37_REMOTE_PANEL
static uint8_t smk37_surface_keys[2][37],smk37_surface_pads[2][16],smk37_surface_buttons[2][18];
static uint8_t smk37_remote_pedal;
static uint32_t smk37_remote_at;
static uint8_t smk37_remote_live,smk37_remote_refresh;
static uint16_t smk37_remote_led_pads;
static uint32_t smk37_remote_led_buttons;
static uint8_t smk37_remote_led_color,smk37_remote_led_level;
static void smk37_surface_key(unsigned source,unsigned id,unsigned v){
    if(source>1||id>=37||v>127)return;
    unsigned was=smk37_surface_keys[0][id]||smk37_surface_keys[1][id];
    smk37_surface_keys[source][id]=!!v;
    unsigned now=smk37_surface_keys[0][id]||smk37_surface_keys[1][id];
    if(was!=now)smk37_play_key_event(id,now?v:0);
}
static void smk37_surface_pad(unsigned source,unsigned id,unsigned v){
    if(source>1||id>=16||v>127)return;
    unsigned was=smk37_surface_pads[0][id]||smk37_surface_pads[1][id];
    smk37_surface_pads[source][id]=!!v;
    unsigned now=smk37_surface_pads[0][id]||smk37_surface_pads[1][id];
    if(was!=now)smk37_pad_event(id,now?v:0);
}
static void smk37_surface_button(unsigned source,unsigned id,unsigned v){
    if(source>1||id>=18||v>1)return;
    unsigned was=smk37_surface_buttons[0][id]||smk37_surface_buttons[1][id];
    smk37_surface_buttons[source][id]=v;
    unsigned now=smk37_surface_buttons[0][id]||smk37_surface_buttons[1][id];
    if(was!=now)smk37_button_input(id,now);
}
static void smk37_remote_release(void){
    for(unsigned i=0;i<37;i++)smk37_surface_key(1,i,0);
    for(unsigned i=0;i<16;i++)smk37_surface_pad(1,i,0);
    /* A disconnected hold must not become a tap (which can invoke a menu action). */
    for(unsigned i=0;i<18;i++)if(smk37_surface_buttons[1][i]){
        smk37_surface_buttons[1][i]=0;
        if(!smk37_surface_buttons[0][i]){smk37_button_down&=~(1u<<i);smk37_button_held&=~(1u<<i);}
    }
    smk37_remote_pedal=0;
    smk37_local_bend=(smk37_analog_ready&16)?smk37_pitch_value(smk37_analog_anchor[4]):0;
    smk37_local_mod=(smk37_analog_ready&32)?smk37_mod_value(smk37_analog_anchor[5]):0;
    smk37_wheels(smk37_local_bend,smk37_local_mod,15);
}
typedef struct {uint8_t kind,id;int16_t value;} smk37_remote_event_t;
static smk37_remote_event_t smk37_remote_queue[32];
static volatile unsigned smk37_remote_w,smk37_remote_r;
static unsigned smk37_remote_valid(unsigned kind,unsigned id,int v){
    switch(kind){
    case 0:return id<18&&(v==0||v==1);
    case 1:return id<8&&v>=-32&&v<=32;
    case 2:return id<37&&v>=0&&v<=127;
    case 3:return id<16&&v>=0&&v<=127;
    case 4:return id<4&&v>=0&&v<=1023;
    case 5:return id<2&&(id?(v>=0&&v<=127):(v>=-8192&&v<=8191));
    case 6:return id==0&&(v==0||v==1);
    case 7:return id==0&&v==0;
    default:return 0;
    }
}
static unsigned smk37_remote_enqueue(unsigned kind,unsigned id,int v){
    if(!smk37_remote_valid(kind,id,v))return 1;
    if(smk37_remote_w-smk37_remote_r>=32)return 2;
    smk37_remote_queue[smk37_remote_w&31]=(smk37_remote_event_t){kind,id,v};
    ++smk37_remote_w;return 0;
}
static void smk37_remote_poll(void){
    if(smk37_remote_live && fm1_ms-smk37_remote_at>2000){
        smk37_remote_live=0;smk37_remote_r=smk37_remote_w;smk37_remote_release();return;
    }
    unsigned budget=8;
    while(budget-- && smk37_remote_r!=smk37_remote_w){
        smk37_remote_event_t e=smk37_remote_queue[smk37_remote_r&31];
        /* Preset/engine loads are expensive: at most one encoder step per audio half. */
        if(e.kind==1 && (e.value>1||e.value<-1))smk37_remote_queue[smk37_remote_r&31].value+=e.value>0?-1:1;
        else ++smk37_remote_r;
        unsigned id=e.id;int v=e.value;
        switch(e.kind){
        case 0:smk37_surface_button(1,id,v);break;
        case 1:if(v)smk37_turn(id,v<0?-1:1);budget=0;break;
        case 2:smk37_surface_key(1,id,v);break;
        case 3:smk37_surface_pad(1,id,v);break;
        case 4:smk37_input_at=fm1_ms;smk37_sound_take(smk37_fader_bank?TSEL:&trk[id],0x800+id);
            if(smk37_fader_bank)TSEL->p[P_ATK+id]=smk37_fader_value(v);else trk[id].p[P_LEVEL]=smk37_fader_value(v);break;
        case 5:if(id)smk37_local_mod=v;else smk37_local_bend=v;
            smk37_wheels(smk37_local_bend,smk37_local_mod,15);break;
        case 6:smk37_remote_pedal=!!v;break;
        case 7:smk37_remote_release();break;
        }
    }
}
#endif
