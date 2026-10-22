/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Chance Roth (@ChanceTheMaker)
 * FM6 field and slice editing semantics from upstream ui_fm6op.c/ui_slice.c,
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hugelton Instruments. */
#define FELUCCA_PALETTE_ADAPTER 1
#include "favorites.c"
#undef FELUCCA_PALETTE_ADAPTER
#include "ui_palette_types.h"
#include "ui_palettes.h"
static uint16_t smk37_custom_preview[5];
static uint8_t smk37_preview_palette;
static uint32_t smk37_palette_generation;
static uint32_t palette_n(void){return UI_NPALETTES+!!(ui_cpal_flags&CPAL_STORED);}
static uint32_t palette_cur(void){return cpal_on()?UI_NPALETTES:smk37_palette%UI_NPALETTES;}
static const char *palette_name(uint32_t v){return v==UI_NPALETTES?"CUSTOM":UI_PALETTES[v%UI_NPALETTES].name;}

#define smk37_browser_fav favorites.filter
static uint8_t smk37_global_prefs;
static uint8_t smk37_restore_last=1,smk37_style,smk37_large;
static uint8_t smk37_animation=1,smk37_pref_row,smk37_nav_group,smk37_nav_row;
static uint8_t smk37_browser_engine,smk37_browser_index,smk37_op,smk37_op_stage,smk37_slice_sel;
static uint8_t smk37_name_pos,smk37_name_slot;
static uint32_t smk37_storage_confirm_at;
static uint8_t smk37_storage_confirm;
static char smk37_name[13];
static volatile uint8_t smk37_prefs_request,smk37_rename_request;
static uint32_t smk37_user_used;
static char smk37_user_names[SMK37_UP_SLOTS][13];
static void smk37_user_name(unsigned k,char *out){str_cpy(out,k<smk37_user_capacity && (smk37_user_used&(1u<<k))?smk37_user_names[k]:"",24);}
static const char *const smk37_groups[]={"SOUND","MODULATION","EFFECTS","SEQUENCER","PERFORMANCE","SAVE / PRESETS","SYSTEM","DIAGNOSTICS"};
static const uint8_t smk37_group_pages[8][13]={
 {0,1,2,SMK37_OP_PAGE,SMK37_OPENV_PAGE,SMK37_OP2_PAGE,SMK37_SLICE_PAGE,3,11,12,255},
 {4,5,6,29,19,20,21,22,255},
 {7,8,9,10,27,28,17,255},
 {23,SMK37_STEP_PAGE,SMK37_DETAIL_PAGE,37,SMK37_AUTOMATION_PAGE,SMK37_SONG_PAGE,SMK37_TOOLS_PAGE,SMK37_PHRASE_PAGE,SMK37_EUCLID_PAGE,30,31,255},
 {13,14,15,16,SMK37_PERFORM_PAGE,SMK37_CLICK_PAGE,255},
 {SMK37_BROWSER_PAGE,SMK37_PRESET_PAGE,SMK37_NAME_PAGE,SMK37_PROJECT_PAGE,SMK37_UNDO_PAGE,255},
 {18,SMK37_SETTINGS_PAGE,33,SMK37_MIDI_CHANNEL_PAGE,SMK37_LEARN_PAGE,SMK37_DISPLAY_PAGE,SMK37_PREF_PAGE,SMK37_STORAGE_PAGE,255},
 {SMK37_ANALOG_PAGE,SMK37_BUTTON_PAGE,SMK37_LIGHT_PAGE,SMK37_PEDAL_PAGE,255}
};
static int smk37_page_ok(unsigned p){
    if(p>=SMK37_OP_PAGE && p<=SMK37_OP2_PAGE)return TSEL->eng_req==ENGI_FM6;
    if(p==SMK37_SLICE_PAGE)return ENGINES[TSEL->eng_req]==&ENG_SLICE;
    return 1;
}
static void smk37_name_begin(void);
static unsigned smk37_next_page(unsigned p,int d){
    if(p>=SMK37_SOUND_PAGES)return smk37_wrap(p,d,SMK37_PAGE_COUNT);
    for(unsigned n=0;n<SMK37_PAGE_COUNT;++n){p=smk37_wrap(p,d,SMK37_PAGE_COUNT);if(smk37_page_ok(p)){if(p==SMK37_NAME_PAGE)smk37_name_begin();return p;}}
    return 0;
}
static unsigned smk37_edit_next(unsigned p){
    static const uint8_t list[]={1,2,SMK37_OP_PAGE,SMK37_OPENV_PAGE,SMK37_OP2_PAGE,SMK37_SLICE_PAGE,3,4,5,6,29,11,12,19,20,21,22};
    unsigned start=0;for(unsigned i=0;i<sizeof list;++i)if(list[i]==p){start=(i+1)%sizeof list;break;}
    for(unsigned n=0;n<sizeof list;++n){unsigned q=list[(start+n)%sizeof list];if(smk37_page_ok(q))return q;}return 1;
}
static unsigned smk37_nav_count(void){unsigned n=0;while(n<13 && smk37_group_pages[smk37_nav_group][n]!=255)++n;return n;}
static void smk37_open(unsigned p){if(eu.on && p!=SMK37_EUCLID_PAGE)eu_close(TSEL,1);smk37_storage_confirm=0;smk37_page=p;smk37_patch_mode=p==0;smk37_input_at=fm1_ms;}
/* One sound undo per gesture. Runtime voice pointers and live note ownership
 * are never copied. Pattern/automation undo remains independent. */
typedef struct {int16_t p[P_COUNT];uint8_t fm6[FP_SIZE+1],engine,preset,user;} smk37_sound_state_t;
static struct {smk37_sound_state_t state;uint32_t at,gesture;uint8_t track,valid;} smk37_sound_undo;
static void smk37_sound_capture(track_t *t,smk37_sound_state_t *s){
    for(unsigned i=0;i<P_COUNT;++i)s->p[i]=motion_base_value(t,i);
    memcpy(s->fm6,fm6_patch[trk_index(t)],sizeof s->fm6);s->engine=t->eng_req;s->preset=t->preset;s->user=t->user;
}
static void smk37_sound_take(track_t *t,unsigned gesture){
    if(smk37_undo_loading)return;
    if(gesture!=0xffff && smk37_sound_undo.valid && smk37_sound_undo.track==trk_index(t) && smk37_sound_undo.gesture==gesture && fm1_ms-smk37_sound_undo.at<800u){smk37_sound_undo.at=fm1_ms;return;}
    smk37_sound_capture(t,&smk37_sound_undo.state);smk37_sound_undo.track=trk_index(t);smk37_sound_undo.valid=1;
    smk37_sound_undo.at=fm1_ms;smk37_sound_undo.gesture=gesture;
}
static void smk37_sound_swap(void){
    if(!smk37_sound_undo.valid){ui_message("NO SOUND UNDO");return;}
    if(chain.running || chain.armed){ui_message("STOP SONG BEFORE UNDO");return;}
    unsigned k=smk37_sound_undo.track;track_t *t=&trk[k];smk37_sound_state_t now;smk37_sound_capture(t,&now);
    smk37_sound_state_t *s=&smk37_sound_undo.state;motion_restore(t);panic_req|=1u<<k;
    memcpy(t->p,s->p,sizeof t->p);t->eng_req=s->engine;t->preset=s->preset;t->user=s->user;
    fm6_set_patch(k,s->fm6);fm6_adopt(k);smk37_sound_undo.state=now;smk37_sound_undo.gesture=0xfffe;
    ui_message("SOUND UNDO / REDO");
}
static const char *const smk37_pref_names[]={"PALETTE","BOLD FONT","MIDI MONITOR","KNOB ACCEL","BPM LOCK","HOLD TIME","HOME VIEW","SCREEN OFF","HELP HINTS","LED LEVEL","ANIMATION","FX LATCH","SCOPE","USB LEVEL","CLICK","CLICK LEVEL","COUNT-IN","RESTORE LAST","STYLE","LARGE","STEP PREVIEW","SPEAKER EQ","TRS OUT"};
#define SMK37_PREF_COUNT 23u
static unsigned smk37_pref_get(unsigned r){switch(r){
 case 0:return palette_cur();case 1:return smk37_bold;case 2:return monitor_mode;case 3:return smk37_accel;case 4:return smk37_bpm_lock;
 case 5:return smk37_hold;case 6:return smk37_home;case 7:return smk37_screen_off;case 8:return smk37_help;case 9:return smk37_led_level;case 10:return smk37_animation;case 11:return perf_latch_on;case 12:return scope_mix;case 13:return fx_usb_fixed;case 14:return click_mode;case 15:return click_lvl;case 16:return cin_bars;case 17:return smk37_restore_last;case 18:return smk37_style;case 19:return smk37_large;case 20:return smk37_step_preview;case 21:return fx_lowcut;case 22:return smk37_trs_enabled;default:return 0;}}
static void smk37_pref_set(unsigned r,unsigned v){switch(r){
 case 0:if(v==UI_NPALETTES && (ui_cpal_flags&CPAL_STORED))ui_cpal_flags|=CPAL_ACTIVE;else {smk37_palette=v%UI_NPALETTES;ui_cpal_flags&=~CPAL_ACTIVE;}smk37_preview_palette=0;++smk37_palette_generation;break;case 1:smk37_bold=!!v;break;case 2:monitor_mode=v%3;break;case 3:smk37_accel=!!v;break;case 4:smk37_bpm_lock=!!v;break;
 case 5:smk37_hold=v%4;break;case 6:smk37_home=v%2;break;case 7:smk37_screen_off=v%5;break;case 8:smk37_help=!!v;break;case 9:smk37_led_level=v%9;break;case 10:smk37_animation=!!v;break;case 11:perf_latch_on=!!v;if(!v)perf_latched=0;break;case 12:scope_mix=!!v;break;case 13:fx_usb_fixed=!!v;break;case 14:click_mode=v%3;break;case 15:click_lvl=v%3;break;case 16:cin_bars=v%3;break;case 17:smk37_restore_last=!!v;break;case 18:smk37_style=!!v;break;case 19:smk37_large=!!v;break;case 20:smk37_step_preview=!!v;if(!v)aud_req=0;break;case 21:fx_lowcut=v%3;break;case 22:smk37_trs_enabled=SMK37_ENABLE_TRS&&!!v;break;}}
static unsigned smk37_pref_max(unsigned r){return r==0?palette_n()-1:r==2 || r==21 || (r>=14 && r<=16)?2:r==5?3:r==7?4:r==9?8:1;}
static void smk37_pref_format(unsigned r,char *s){unsigned v=smk37_pref_get(r);
    if(r==0){str_cpy(s,palette_name(v),24);return;}
    if(r==2){str_cpy(s,(const char *const[]){"OFF","EVENTS","NOTES"}[v],24);return;}
    if(r==5){fmt_int(s,300+100*v);str_cpy(s+str_len(s)," ms",8);return;}
    if(r==6){str_cpy(s,v?"SCOPE":"TRACKS",24);return;}
    if(r==7){str_cpy(s,(const char *const[]){"NEVER","5 MIN","15 MIN","30 MIN","60 MIN"}[v],24);return;}
    if(r==12){str_cpy(s,v?"MIX":"OUT",24);return;}
    if(r==13){str_cpy(s,v?"FIXED":"MASTER",24);return;}
    if(r==14){str_cpy(s,(const char *const[]){"OFF","REC","ON"}[v],24);return;}
    if(r==15){str_cpy(s,(const char *const[]){"LOW","MID","HIGH"}[v],24);return;}
    if(r==16){str_cpy(s,(const char *const[]){"OFF","1 BAR","2 BARS"}[v],24);return;}
    if(r==18){str_cpy(s,v?"LINE":"FLAT",24);return;}
    if(r==21){str_cpy(s,(const char *const[]){"FLAT","LOWCUT","BASS+"}[v],24);return;}
    if(r==9){fmt_int(s,v);return;}str_cpy(s,v?"ON":"OFF",24);
}
static int smk37_browser_valid(unsigned e,unsigned p){
    if(e==NENGINES){if(p>=smk37_user_capacity || !(smk37_user_used&(1u<<p)))return 0;}
    else {if(e>=NENGINES || e==ENGI_DIGITAL || p>=ENGINES[e]->npresets)return 0;
        if(ENGINES[e]->presets==SMP_PRESET_TABLE && p<SMP_NSETS && SMP_SET_ORIG[p]!=p)return 0;}
    return !smk37_browser_fav || favorite_has(e,p);
}
static unsigned smk37_browser_count(void){unsigned n=0;for(unsigned p=0;p<256;++p)n+=smk37_browser_valid(smk37_browser_engine,p);return n;}
static void smk37_browser_move(int d){unsigned p=smk37_browser_index;
    for(unsigned n=0;n<256;++n){p=smk37_wrap(p,d,256);if(smk37_browser_valid(smk37_browser_engine,p)){smk37_browser_index=p;return;}}}
static void smk37_browser_fix(void){if(!smk37_browser_valid(smk37_browser_engine,smk37_browser_index))smk37_browser_move(1);}
static void smk37_browser_name(unsigned p,char *s){
    if(!smk37_browser_valid(smk37_browser_engine,p)){str_cpy(s,"NO MATCHING PRESETS",24);return;}
    if(smk37_browser_engine==NENGINES)smk37_user_name(p,s);
    else str_cpy(s,ENGINES[smk37_browser_engine]->presets[p].name,24);
}
static void smk37_name_begin(void){smk37_name_slot=smk37_user_slot;memset(smk37_name,' ',12);smk37_name[12]=0;
    char name[24];smk37_user_name(smk37_user_slot,name);for(unsigned i=0;i<12 && name[i];++i)smk37_name[i]=name[i];smk37_name_pos=0;}
/* A held navigation key selects a layer without firing its tap action. */
static uint8_t smk37_fill_down;
static uint32_t smk37_button_down,smk37_button_held,smk37_button_at[18];
static void smk37_hold_poll(void){for(unsigned b=10;b<=13;++b)if((smk37_button_down&(1u<<b)) && !(smk37_button_held&(1u<<b)) && fm1_ms-smk37_button_at[b]>=300u+100u*smk37_hold){
    smk37_button_held|=1u<<b;
    if(b==10)smk37_open(SMK37_NAV_PAGE);if(b==11)smk37_open(SMK37_PREF_PAGE);
    if(b==12){smk37_browser_engine=TSEL->eng_req;smk37_browser_index=TSEL->preset;smk37_browser_fix();smk37_open(SMK37_BROWSER_PAGE);}
    if(b==13)smk37_open(SMK37_UNDO_PAGE);
}}
static void smk37_button_input(unsigned b,unsigned down){
    if(b>=18)return;smk37_input_at=fm1_ms;
    /* Hold > on SEQ TOOLS for momentary FILL; release always clears it. */
    if(b==3 && ((!down && smk37_fill_down) || (down && smk37_page==SMK37_TOOLS_PAGE && smk37_tool==TL_N))){seq_fill=!!down;smk37_fill_down=!!down;return;}
    if(down){smk37_button_down|=1u<<b;smk37_button_at[b]=fm1_ms;if(b<10 || b>13)smk37_button_event(b);}
    else {smk37_hold_poll();smk37_button_down&=~(1u<<b);if(b>=10 && b<=13 && !(smk37_button_held&(1u<<b)))smk37_button_event(b);smk37_button_held&=~(1u<<b);}
}
/* Deep FM6: operator order, byte bounds and OWN adoption match upstream. */
static unsigned smk37_op_byte(unsigned col){
    unsigned off=0;
    if(smk37_page==SMK37_OP_PAGE)off=(unsigned[]){0,FP_FC,FP_FF,FP_OL}[col];
    if(smk37_page==SMK37_OP2_PAGE)off=(unsigned[]){0,FP_MODE,FP_DET,FP_KVS}[col];
    if(smk37_page==SMK37_OPENV_PAGE)off=(col==2?FP_R1:FP_L1)+smk37_op_stage;
    return (5u-smk37_op)*FP_OP+off;
}
static void smk37_op_turn(unsigned col,int d){
    if(TSEL->eng_req!=ENGI_FM6)return;
    if(!col){smk37_op=clamp(smk37_op+d,0,5);return;}
    if(smk37_page==SMK37_OPENV_PAGE && col==1){smk37_op_stage=clamp(smk37_op_stage+d,0,3);return;}
    unsigned b=smk37_op_byte(col);uint8_t p[FP_SIZE+1];memcpy(p,fm6_patch[song.sel],sizeof p);
    int v=clamp(p[b]+d,0,fm6_max(b));if(v==p[b])return;smk37_sound_take(TSEL,0x1000+b);p[b]=v;
    fm6_set_patch(song.sel,p);fm6_own_ok&=~(1u<<song.sel);fm6_slot[song.sel]=FM6_OWN;TSEL->p[P_E7]=FM6_OWN;
}
/* Built-in samples stay read-only, as upstream. User material is required for
 * manual tables; sample uploads require separately initialized SMK storage. */
static const slc_src_t *smk37_slice_source(void){return ENGINES[TSEL->eng_req]==&ENG_SLICE?slc_get(slc_src_of(TSEL->p)):0;}
static unsigned smk37_slice_count(void){const slc_src_t *s=smk37_slice_source();return s?slc_count(s,clamp(TSEL->p[P_E1],0,SLC_DIV_MAN)):0;}
static unsigned smk37_slice_mark(unsigned j){const slc_src_t *s=smk37_slice_source();unsigned n=smk37_slice_count(),a,b,st;if(!n)return 0;slc_bounds(s,TSEL->p[P_E1],j<n?j:n-1,&a,&b,&st);return j<n?a:b;}
/* Control IRQ publishes a small request. Decoder checkpoint rebuilding can be
 * long, so the main loop performs it with the audio interrupts running. */
static volatile uint8_t smk37_slice_request;
static uint8_t smk37_slice_track,smk37_slice_src,smk37_slice_div,smk37_slice_marker;
static int8_t smk37_slice_direction;
static void smk37_slice_edit(unsigned action,int direction){
    const slc_src_t *s=smk37_slice_source();unsigned src=slc_src_of(TSEL->p);
    if(!s){ui_message("USER SAMPLE EMPTY");return;}
    if(!src || src==SLC_SRC_PIANO){ui_message("BUILT-IN: READ ONLY");return;}
    if(smk37_slice_request){ui_message("SLICE EDIT BUSY");return;}
    smk37_slice_track=song.sel;smk37_slice_src=src;smk37_slice_div=TSEL->p[P_E1];
    smk37_slice_marker=smk37_slice_sel;smk37_slice_direction=direction>0?1:-1;
    smk37_slice_request=action;
}
static int smk37_workflow_turn(unsigned e,int d){
    if(e<4)return 0;unsigned c=e-4;
    if(smk37_page==33 && c>=2){if(c==2){smk37_trs_enabled=SMK37_ENABLE_TRS&&d>0;smk37_prefs_request=1;ui_message(SMK37_ENABLE_TRS?"TRS OUT / USB UNCHANGED":"TRS NOT ENABLED IN BUILD");}return 1;}
    if(smk37_page==SMK37_PRESET_PAGE && c==3){smk37_name_begin();smk37_open(SMK37_NAME_PAGE);return 1;}
    switch(smk37_page){
    case SMK37_EUCLID_PAGE:
        if(chain.running || chain.armed){ui_message("STOP SONG FIRST");return 1;}
        if(eu.on && eu.trk!=song.sel)eu_close(TSEL,1);
        if(!eu.on)eu_open(TSEL);
        if(c<3)eu_knob(TSEL,c,d);else {smk37_lane=smk37_wrap(smk37_lane,d,NLANE);eu_lane(TSEL);}return 1;
    case SMK37_MIDI_CHANNEL_PAGE:mch_set(c,clamp((int)mch_get(c)+d,0,MCH_N-1));midi_route=MIDI_ROUTE_AGAIN;return 1;
    case SMK37_NAV_PAGE:
        if(!c){smk37_nav_group=smk37_wrap(smk37_nav_group,d,8);smk37_nav_row=0;}
        if(c==1)smk37_nav_row=smk37_wrap(smk37_nav_row,d,smk37_nav_count());return 1;
    case SMK37_PREF_PAGE:
        if(!c)smk37_pref_row=smk37_wrap(smk37_pref_row,d,SMK37_PREF_COUNT);
        if(c==1)smk37_pref_set(smk37_pref_row,clamp((int)smk37_pref_get(smk37_pref_row)+d,0,smk37_pref_max(smk37_pref_row)));return 1;
    case SMK37_BROWSER_PAGE:
        if(!c){do{smk37_browser_engine=smk37_wrap(smk37_browser_engine,d,NENGINES+1);}while(smk37_browser_engine==ENGI_DIGITAL);smk37_browser_index=0;smk37_browser_fix();}
        if(c==1)smk37_browser_move(d);
        if(c==2){smk37_browser_fav=d>0;smk37_prefs_request=1;smk37_browser_fix();}
        if(c==3 && smk37_browser_valid(smk37_browser_engine,smk37_browser_index)){favorite_set(smk37_browser_engine,smk37_browser_index,d>0);smk37_prefs_request=1;smk37_browser_fix();}return 1;
    case SMK37_OP_PAGE:case SMK37_OPENV_PAGE:case SMK37_OP2_PAGE:smk37_op_turn(c,d);return 1;
    case SMK37_SLICE_PAGE:if(!c)smk37_slice_sel=clamp(smk37_slice_sel+d,0,smk37_slice_count());else smk37_slice_edit(c,d);return 1;
    case SMK37_NAME_PAGE:
        if(!c)smk37_name_pos=smk37_wrap(smk37_name_pos,d,12);
        if(c==1)smk37_name[smk37_name_pos]=(char)clamp(smk37_name[smk37_name_pos]+d,32,126);return 1;
    case SMK37_PHRASE_PAGE:if(!c){smk37_phrase=smk37_wrap(smk37_phrase,d,smk37_phrase_count());smk37_phrase_confirm=0;}return 1;
    case SMK37_UNDO_PAGE:return 1;
    case SMK37_STORAGE_PAGE:return 1;
    default:return 0;
    }
}
static int smk37_workflow_button(unsigned b){
    if(b!=2 && b!=3)return 0;
    switch(smk37_page){
    case SMK37_EUCLID_PAGE:
        if(chain.running || chain.armed){ui_message("STOP SONG FIRST");return 1;}
        if(eu.on){eu_close(TSEL,b==3);ui_message(b==3?"EUCLID APPLIED":"EUCLID CANCELLED");}
        else if(b==3)eu_open(TSEL);return 1;
    case SMK37_STORAGE_PAGE:
        if(b==2){smk37_open(SMK37_NAV_PAGE);return 1;}
        if(!SMK37_ENABLE_SAMPLES){ui_message("SAMPLES DISABLED IN BUILD");return 1;}
        if(smk37_samples_owned){ui_message("SAMPLES READY / USE EDITOR");return 1;}
        if(smk37_samples_state==3){ui_message("STORAGE IDENTITY MISMATCH");return 1;}
        if(!smk37_storage_confirm||fm1_ms-smk37_storage_confirm_at>5000u){smk37_storage_confirm=1;smk37_storage_confirm_at=fm1_ms;ui_message("REPLACE STOCK SAMPLE DATA?");return 1;}
        smk37_storage_confirm=0;smk37_samples_request=1;ui_message("INITIALIZING SAMPLES");return 1;
    case SMK37_NAV_PAGE:
        if(b==2)smk37_open(0);else {unsigned p=smk37_group_pages[smk37_nav_group][smk37_nav_row];
            if(!smk37_page_ok(p)){ui_message("SELECT MATCHING ENGINE");return 1;}if(p==SMK37_NAME_PAGE)smk37_name_begin();smk37_open(p);}return 1;
    case SMK37_PREF_PAGE:if(b==2)smk37_open(SMK37_NAV_PAGE);else {smk37_prefs_request=1;ui_message("SETTINGS SAVE QUEUED");}return 1;
    case SMK37_BROWSER_PAGE:
        if(b==2)smk37_open(0);else if(smk37_browser_valid(smk37_browser_engine,smk37_browser_index)){
            if(smk37_browser_engine==NENGINES)smk37_user_request=(smk37_browser_index<<2)|1;
            else {smk37_sound_take(TSEL,0xffff);smk37_undo_loading=1;TSEL->eng_req=smk37_browser_engine;smk37_load_preset(TSEL,smk37_browser_index);smk37_undo_loading=0;ui_message("PRESET LOADED");}}
        return 1;
    case SMK37_PHRASE_PAGE:if(b==2){smk37_phrase_confirm=0;smk37_open(SMK37_NAV_PAGE);}else smk37_phrase_load();return 1;
    case SMK37_UNDO_PAGE:if(b==2)smk37_pattern_undo();else smk37_sound_swap();return 1;
    case SMK37_NAME_PAGE:if(b==2)smk37_open(SMK37_PRESET_PAGE);else smk37_rename_request=1;return 1;
    default:return 0;
    }
}
