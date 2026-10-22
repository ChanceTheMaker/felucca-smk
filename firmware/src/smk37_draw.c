/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Chance Roth (@ChanceTheMaker)
 * Card geometry and visual treatment adapted from ui_draw.c / ui_graph.c:
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hugelton Instruments.
 * SMK navigation remains board-specific. This is not the full FM-1 menu stack.
 * The shared canvas, fonts, palettes and curve functions are the FM-1 ones.
 * LCD transfers run with IRQs enabled; the audio/scanner continue while drawing.
 */
#include "smk37_palette_draw.c"
#define PANEL_X0 10
#define PANEL_W 220
#define H_GRAPH 122
static int32_t graph_ht=100;
static unsigned smk37_draw_step, smk37_draw_lane, smk37_draw_track;
static unsigned smk37_draw_page; /* frozen for this frame; scanner may change smk37_page */
static unsigned smk37_large_frame;
static const page_t *cur_page(void){return &smk37_sound_pages[smk37_draw_page];}
#include "ui_shapes.c"

#define GOY 11
#define CARD_X(c) (3+59*(c))
static int32_t bar_x(uint32_t i){return 12+(i%16u)*13+(i%16u/4u)*4;}
static void note_name(char *b,uint32_t n){str_cpy(b,N_NOTE[n%12u],4);fmt_int(b+str_len(b),(int32_t)(n/12u)-1);}
static void panel_note(const char *a,const char *b,const char *c){
    if(b && str_eq(b,"[K1] DIA3: IN-KEY CHORDS"))b="K5: DIA3 IN-KEY CHORDS";
    const char *line[3]={a,b,c};
    for(unsigned i=0;i<3;++i)if(line[i])cv_text_in(10,27+i*25,220,&AF_S,line[i],i?T_MID:T_TEXT,T_SURF);
}
#include "ui_fm6format.c"
#include "ui_sound_graphs.c"
#include "ui_instrument_graphs.c"
#include "ui_sample_graph.c"
static int step_on(const step_t *s){return s->time==ST_NOTE && (s->n || s->hit);}
static int step_detailed(const step_t *s){return step_chance(s)<100u || step_cond(s) || step_ratchet(s)>1u || step_nudge(s)!=0;}
#include "smk37_sequence_graphs.c"
#include "smk37_panels.c"
#include "smk37_workflow_draw.c"
#include "smk37_roll.c"
#include "monitor_draw.c"

static void smk37_draw_header(unsigned track)
{
    char text[24];
    cv_begin(240,24,T_BG);
    for(unsigned i=0;i<NTRK;++i){
        int x=3+21*i;uint16_t bg=i==track?T_THEME:T_SURF;
        cv_rrect(x,3,18,18,4,bg,T_BG);
        text[0]='1'+i;text[1]=0;
        cv_text_in(x,4,18,&AF_S,text,i==track?T_INK:T_MID,bg);
    }
    fmt_int(text,song.g[G_BPM]);
    smk37_roll_prepare(4,text,1,0);
    int x=roll_text(4,93,2,text,T_TEXT);
    cv_text_on(x+3,5,&AF_S,"BPM",T_MID,T_BG);
    cv_text_r(237,5,&AF_S,song.rec?"REC":song.playing?"PLAY":"STOP",song.rec?T_REC:T_THEME,T_BG);
    cv_blit(0,0);
}

static void smk37_draw_card(unsigned column,const char *label,const char *value,const char *unit,int ratio)
{
    /* FM-1 standard cards: 57 x 44, x 3+59*c, y 28. */
    int width=smk37_large_frame?116:57,height=smk37_large_frame?82:44,space=width-8;
    cv_begin(width,height,T_BG);
    if(smk37_style){cv_rect(0,0,width,height,T_SURF);cv_rect(0,height-1,width,1,T_RAISE);}
    else cv_rrect(0,0,width,height,4,T_SURF,T_BG);
    cv_text_fit(5,3,smk37_large_frame?&AF_M:&AF_S,label,T_MID,T_SURF,space);
    if(str_eq(label,unit))unit="";
    int uw=unit[0]?text_w(&AF_S,unit)+3:0;
    const aafont_t *font=smk37_large_frame && text_w(&AF_L,value)+uw<=space?&AF_L:text_w(&AF_M,value)+uw<=space?&AF_M:&AF_S;
    /* Preserve a long value's space; its full text is also in custom-page rows. */
    if(text_w(font,value)+uw>space)uw=0;
    unsigned sig=smk37_draw_page*65537u+smk37_draw_track*104729u+TSEL->eng_req*257u+smk37_palette*31u+smk37_bold;
    for(const char *p=label;*p;++p)sig=sig*33u+(unsigned char)*p;
    for(const char *p=unit;*p;++p)sig=sig*33u+(unsigned char)*p;
    smk37_roll_prepare(column,value,sig,font!=&AF_M);
    int y=smk37_large_frame?35:17;
    int x=font==&AF_M?roll_text(column,5,y,value,T_THEME):cv_text_fit(5,y,font,value,T_THEME,T_SURF,space-uw);
    if(uw)cv_text_on(x+3,y+3,&AF_S,unit,T_MID,T_SURF);
    if(ratio>=0){
        cv_rrect(5,height-6,width-10,3,1,T_BG,T_SURF);
        int w=ratio*(width-10)/1000;
        cv_rrect(5,height-6,w<3?3:w,3,1,T_THEME,T_BG);
    }
    cv_blit(smk37_large_frame?3+119*(column%2):3+59*column,smk37_large_frame?28+85*(column/2):28);
}

static void smk37_draw_tracks(unsigned selected)
{
    for(unsigned i=0;i<NTRK;++i){
        const track_t *t=&trk[i];int y=4+29*i;
        uint16_t bg=i==selected?T_TINT:T_SURF;
        cv_rrect(5,y,230,27,4,bg,T_SURF);
        char number[2]={(char)('1'+i),0};
        cv_rrect(8,y+4,18,18,3,i==selected?T_THEME:T_RAISE,bg);
        cv_text_in(8,y+4,18,&AF_S,number,i==selected?T_INK:T_MID,i==selected?T_THEME:T_RAISE);
        cv_text_fit(32,y+1,&AF_S,ENGINES[t->eng_req]->name,T_TEXT,bg,89);
        int w=clamp(t->p[P_LEVEL],0,127)*92/127;
        cv_rrect(126,y+17,100,3,1,T_RAISE,bg);
        if(w)cv_rrect(126,y+17,w,3,1,t->p[P_MUTE]?T_DIM:T_THEME,T_RAISE);
        char value[12];const char *unit;param_format(&TP[P_LEVEL],t->p[P_LEVEL],value,&unit);
        cv_text_r(226,y,&AF_S,t->p[P_MUTE]?"MUTE":value,t->p[P_MUTE]?T_REC:T_MID,bg);
        const step_t *st=seq_steps(t);unsigned base=(song.playing?t->seq_idx:smk37_edit_step)&~15u;
        for(unsigned j=0;j<16 && base+j<(unsigned)t->p[P_SLEN];++j){
            int on=st[base+j].time==ST_NOTE && (st[base+j].n||st[base+j].hit);
            cv_rect(32+5*j,y+19,3,on?4:2,song.playing && base+j==t->seq_idx?T_ACCENT:on?T_THEME:T_RAISE);
        }
    }
}

static void smk37_draw_panel(unsigned track,unsigned page,const char *title,char labels[4][24],char values[4][24])
{
    cv_begin(240,H_GRAPH,T_BG);
    cv_rrect(3,0,234,H_GRAPH,5,T_SURF,T_BG);
    cv_bg=T_SURF;
    const track_t *t=&trk[track];
    cv_oy=GOY;
    switch(page){
    case 0: cv_oy=0;if(smk37_home)graph_scope(T_THEME);else smk37_draw_tracks(track);break;
    case 1: case 2:
        if(t->eng_req==ENGI_FM6)graph_fm6(t,T_THEME,6,0);
        else if(ENGINES[t->eng_req]==&ENG_WHEEL)graph_wheel(t,T_THEME,-1);
        else if(ENGINES[t->eng_req]==&ENG_SAMPLE){
            for(unsigned i=0;i<8;++i)sample_wave_tick(t);
            if(sample_wave.zone)graph_sample(t,T_THEME);else panel_note("NO SAMPLE",0,0);
        }else{cv_oy=0;graph_scope(T_THEME);cv_text_fit(10,2,&AF_S,title,T_MID,T_SURF,220);}
        break;
    case 3: graph_adsr(t,T_THEME);break;
    case 5: case 6: case 29: graph_lfo(t,T_THEME);break;
    case 7: graph_fx(t,T_THEME);break;
    case 13: graph_scale(t,T_THEME);break;
    case 14: cv_oy=0;graph_chord(t,T_THEME);break;
    case 17: graph_slicer(t,T_THEME);break;
    case 19: case 20: case 21: case 22: cv_oy=0;graph_mod(t,T_THEME,page-19);break;
    case 27: case 28: graph_insert(t,T_THEME);break;
    case 23: case SMK37_STEP_PAGE: case 37:
        cv_oy=0;pr_follow(t);
        if(drum_track(t))graph_grid(t,T_THEME);else graph_roll(t,T_THEME);
        break;
    case SMK37_DETAIL_PAGE: cv_oy=0;smk37_graph_detail(t);break;
    case 30: case 31: cv_oy=0;smk37_graph_drums(t,page-30);break;
    case SMK37_SONG_PAGE: cv_oy=0;smk37_graph_song();break;
    case SMK37_PROJECT_PAGE: cv_oy=0;smk37_graph_projects();break;
    case SMK37_PRESET_PAGE: cv_oy=0;smk37_graph_presets();break;
    case SMK37_AUTOMATION_PAGE: cv_oy=0;smk37_graph_automation(t);break;
    case SMK37_PERFORM_PAGE: cv_oy=0;smk37_graph_performance();break;
    case SMK37_DISPLAY_PAGE: cv_oy=0;smk37_graph_display();break;
    default:
        cv_oy=0;if(page>=SMK37_NAV_PAGE)smk37_workflow_panel(page);else smk37_graph_values(title,labels,values);
        break;
    }
    cv_oy=0;
    if(page==0 || page==1 || page==2)draw_monitor();
    cv_blit(0,76);
}

static void smk37_draw_footer(const char *engine,const char *preset,const char *hint)
{
    cv_begin(240,38,T_BG);
    const char *notice=smk37_notice && fm1_ms-smk37_notice_at<3000u?smk37_notice:0;
    if(smk37_analog_failed)notice="ADC TIMEOUT";
    if(smk37_light_failed)notice="LED TIMEOUT - STOPPED";
    char master[24];fmt_int(master,(song.master_q12*100u+2048u)/4096u);
    smk37_roll_prepare(ROLL_MASTER,master,smk37_palette*2u+smk37_bold,0);
    if(notice==smk37_master_notice){
        int x=cv_text_on(5,2,&AF_S,"MASTER",T_ACCENT,T_BG);
        x=roll_text(ROLL_MASTER,x+8,0,master,T_ACCENT);
        cv_text_on(x+3,2,&AF_S,"%",T_ACCENT,T_BG);
    }else if(notice)cv_text_fit(5,0,&AF_S,notice,smk37_analog_failed||smk37_light_failed?T_REC:T_ACCENT,T_BG,230);
    else{
        cv_text_fit(5,0,&AF_S,hint,T_MID,T_BG,175);
        if(text_w(&AF_S,hint)<100){
            char oct[12]="OCT ";fmt_int(oct+4,song.octave);
            cv_text_on(109,0,&AF_S,oct,T_MID,T_BG);
        }
        char bank[12]="K1 / VOL";bank[1]='1'+smk37_knob_bank;
        if(smk37_fader_bank)str_cpy(bank+5,"ENV",sizeof bank-5);
        cv_text_r(235,0,&AF_S,bank,T_THEME,T_BG);
    }
    int x=cv_text_fit(5,19,&AF_S,engine,T_THEME,T_BG,78);
    cv_text_fit(x+8,19,&AF_S,preset,T_TEXT,T_BG,235-x-8);
    cv_blit(0,202);
}

static void smk37_sound_status(void)
{
    if(ux.style!=(smk37_style?ST_LINE:ST_FLAT)){
        ux.style=smk37_style?ST_LINE:ST_FLAT;smk37_palette_apply();
    }
    smk37_large_frame=smk37_large && ((smk37_page>0 && smk37_page<24) || (smk37_page>=27 && smk37_page<=31) || smk37_page==33 || smk37_page==37);
    const param_desc_t *desc[4];
    int16_t values[4];
    const char *engine, *preset, *page;
    char user_name[13]={0};
    unsigned track;
    /* Copy a coherent view while audio cannot change selection/parameters;
     * formatting and slow LCD transfers happen after interrupts are restored. */
    fm1_irq_off();
    if (smk37_page >= SMK37_SOUND_PAGES) { fm1_irq_on(); return; }
    track = song.sel;
    const engine_t *e = ENGINES[TSEL->eng_req];
    engine = e->name;
    preset = e->npresets ? e->presets[TSEL->preset % e->npresets].name : "INIT";
    unsigned user=TSEL->user;
    if(smk37_page==SMK37_PRESET_PAGE)user=smk37_user_slot+1;
    if(user && smk37_up_used(user-1)){memcpy(user_name,smk37_up.record[user-1].name,12);preset=user_name;}
    else if(smk37_page==SMK37_PRESET_PAGE)preset="EMPTY USER SLOT";
    const page_t *pg = &smk37_sound_pages[smk37_page];
    page = pg->title;
    for (unsigned i = 0; i < 4; ++i) {
        int16_t *v;
        desc[i] = page_desc(pg, i, &v);
        values[i] = v ? *v : 0;
    }
    unsigned custom=smk37_page;
    smk37_draw_page=custom;
    smk37_draw_track=track;
    smk37_draw_step=smk37_edit_step;
    if(song.playing && (custom==23 || custom==37))smk37_draw_step=trk[track].seq_idx;
    smk37_draw_lane=smk37_lane;
    if(custom==SMK37_PERFORM_PAGE)for(unsigned i=0;i<4;++i)values[i]=perf_k[i];
    if(custom==SMK37_SONG_PAGE){chain_row_t *r=&chain_config.row[smk37_song_row];values[0]=chain_config.count?smk37_song_row+1:0;values[1]=smk37_song_track+1;values[2]=r->slot[smk37_song_track]+1;values[3]=r->repeat;}
    if(custom==SMK37_TOOLS_PAGE){values[0]=smk37_tool;values[1]=smk37_lane+1;values[2]=smk37_undo.valid;values[3]=smk37_undo.track+1;}
    if(custom==SMK37_PRESET_PAGE){values[0]=smk37_user_slot+1;values[1]=smk37_up_used(smk37_user_slot);values[2]=smk37_up_writable;values[3]=8;}
    if(custom==SMK37_AUTOMATION_PAGE){values[0]=smk37_edit_step+1;values[1]=smk37_auto_param;values[2]=smk37_auto_value;values[3]=smk37_auto_lock;}
    if(custom==SMK37_DETAIL_PAGE){step_t *s=&TSEL->step[smk37_edit_step];values[0]=step_chance_v(s);values[1]=step_ratchet(s);values[2]=step_nudge(s);values[3]=s->vel;}
    if(custom==SMK37_LEARN_PAGE){values[0]=smk37_learn_on;values[1]=smk37_learn_track+1;values[2]=smk37_learn_param;values[3]=ml_arm;}
    if(custom==SMK37_CLICK_PAGE){values[0]=click_mode;values[1]=click_lvl;values[2]=cin_bars;values[3]=seq_counting();}
    if(custom==SMK37_DISPLAY_PAGE){values[0]=palette_cur();values[1]=smk37_bold;values[2]=0;values[3]=0;}
    if(custom==SMK37_STEP_PAGE){step_t *s=&TSEL->step[smk37_edit_step];values[0]=smk37_edit_step+1;values[1]=s->note[0];values[2]=s->vel;values[3]=s->time;}
    if(custom==SMK37_SETTINGS_PAGE){values[0]=smk37_velocity_mode;values[1]=smk37_pad_bank+1;values[2]=smk37_last_velocity;values[3]=smk37_pedal_down;}
    if(custom==SMK37_PROJECT_PAGE){values[0]=smk37_project_slot+1;values[1]=smk37_storage_ready;values[2]=smk37_storage_writable;values[3]=0;}
    fm1_irq_on();
    char value[24];
    char labels[4][24], formatted[4][24];
    smk37_draw_header(track);
    for (unsigned i = 0; i < 4; ++i) {
        const char *unit = "";
        const char *label=desc[i] ? desc[i]->label : "";
        static const char *const step_labels[]={"STEP","NOTE","VELOCITY","TYPE"};
        static const char *const control_labels[]={"VELOCITY","PAD BANK","LAST VEL","SUSTAIN"};
        static const char *const project_labels[]={"SLOT","READ OK","SAVE READY",""};
        if(custom==SMK37_STEP_PAGE)label=step_labels[i];
        if(custom==SMK37_PERFORM_PAGE)label=(const char *const[]){"FILTER","CRUSH","THROW","DEPTH"}[i];
        if(custom==SMK37_SONG_PAGE)label=(const char *const[]){"SECTION","TRACK","PROJECT","REPEAT"}[i];
        if(custom==SMK37_TOOLS_PAGE)label=(const char *const[]){"TOOL","DRUM LANE","UNDO READY","UNDO TRACK"}[i];
        if(custom==SMK37_PRESET_PAGE)label=(const char *const[]){"USER SLOT","USED","SAVE ENABLED","CAPACITY"}[i];
        if(custom==SMK37_AUTOMATION_PAGE)label=(const char *const[]){"STEP","PARAMETER","VALUE","LOCK"}[i];
        if(custom==SMK37_DETAIL_PAGE)label=(const char *const[]){"CHANCE","RATCHET","NUDGE","VELOCITY"}[i];
        if(custom==SMK37_LEARN_PAGE)label=(const char *const[]){"LEARN","TRACK","PARAMETER","WAITING CC"}[i];
        if(custom==SMK37_CLICK_PAGE)label=(const char *const[]){"CLICK","LEVEL","COUNT-IN","COUNTING"}[i];
        if(custom==SMK37_DISPLAY_PAGE)label=(const char *const[]){"PALETTE","BOLD","",""}[i];
        if(custom==SMK37_SETTINGS_PAGE)label=control_labels[i];
        if(custom==SMK37_PROJECT_PAGE)label=project_labels[i];

        value[0] = 0;
        if(custom==33 && i>=2){label=i==2?"TRS OUT":"TX";if(i==2)str_cpy(value,smk37_trs_enabled?"ON":"OFF",sizeof value);else fmt_int(value,smk37_trs_bytes%1000000u);}
        if (desc[i]) param_format(desc[i], values[i], value, &unit);
        if((custom>=SMK37_STEP_PAGE && custom<=SMK37_PROJECT_PAGE) || custom==SMK37_DETAIL_PAGE || custom==SMK37_LEARN_PAGE || custom==SMK37_CLICK_PAGE || custom==SMK37_DISPLAY_PAGE || custom==SMK37_PERFORM_PAGE || custom==SMK37_AUTOMATION_PAGE || custom==SMK37_SONG_PAGE || custom==SMK37_TOOLS_PAGE || custom==SMK37_PRESET_PAGE)fmt_int(value,values[i]);
        if(custom==SMK37_LEARN_PAGE && i==2)str_cpy(value,param_desc_of(trk[smk37_learn_track].eng_req,values[i])->label,sizeof value);
        if(custom==SMK37_TOOLS_PAGE && i==0)str_cpy(value,(const char *const[]){"CLEAR","REVERSE","SHIFT LEFT","SHIFT RIGHT","RANDOM","COOK","BEAT","EUCLID","LANE CLEAR","LANE REVERSE","LANE FILL","LANE RANDOM","DOUBLE","HALF","HOLD FILL"}[smk37_tool],sizeof value);
        if(custom==SMK37_DETAIL_PAGE && !i && values[i]>100)str_cpy(value,(const char *const[]){"1:2","2:2","1:3","2:3","3:3","1:4","2:4","3:4","4:4","FILL","!FILL"}[values[i]-101],sizeof value);
        if(custom==SMK37_SONG_PAGE && i==2 && values[i]==5)str_cpy(value,"SILENT",sizeof value);
        if(custom==SMK37_AUTOMATION_PAGE && i==1)str_cpy(value,track_desc(TSEL,values[i])->label,sizeof value);
        if(custom==SMK37_DISPLAY_PAGE){if(!i)str_cpy(value,palette_name(values[i]),sizeof value);else if(i>1)value[0]=0;}
        if(custom==SMK37_CLICK_PAGE && !i)str_cpy(value,(const char *const[]){"OFF","REC","ON"}[values[i]%3],sizeof value);
        if(custom==SMK37_SETTINGS_PAGE && !i){static const char *const curves[]={"LINEAR","SOFT","HARD","FIXED 100"};str_cpy(value,curves[values[i]],sizeof value);}
        if(custom==SMK37_STEP_PAGE && i==3){static const char *const types[]={"NOTE","TIE","REST"};str_cpy(value,types[values[i]%3],sizeof value);}
        if(custom==SMK37_PROJECT_PAGE && i==3)value[0]=0;
        if(custom>=SMK37_NAV_PAGE)smk37_workflow_card(custom,i,&label,value,&unit);
        str_cpy(labels[i],label,sizeof labels[i]);
        str_cpy(formatted[i],value,sizeof formatted[i]);
        if(unit[0] && !str_eq(label,unit)){
            unsigned n=str_len(formatted[i]);
            if(n+1<sizeof formatted[i]){formatted[i][n++]=' ';formatted[i][n]=0;str_cpy(formatted[i]+n,unit,sizeof formatted[i]-n);}
        }
        int ratio=desc[i] && desc[i]->max>desc[i]->min ?
            clamp((enum_rank(desc[i],values[i])-desc[i]->min)*1000/(desc[i]->max-desc[i]->min),0,1000) : -1;
        smk37_draw_card(i,label,value,unit,ratio);

    }
    const char *hint=smk37_patch_mode ? "< > PRESET" : "< > PAGE";
    if(custom==SMK37_PERFORM_PAGE)hint="HOLD PADS: FX / THROWS";
    if(custom==SMK37_SONG_PAGE)hint="< DELETE / > ADD / PLAY";
    if(custom==SMK37_TOOLS_PAGE)hint="< UNDO-REDO / > RUN";
    if(custom==SMK37_PRESET_PAGE || custom==SMK37_PROJECT_PAGE)hint="< LOAD / > SAVE";
    if(custom==SMK37_AUTOMATION_PAGE)hint="< DELETE / > SET";
    if(custom==SMK37_LEARN_PAGE)hint="TURN PARAM / SEND MIDI CC";
    if(custom==SMK37_STEP_PAGE)hint="PADS: STEPS / < >: BANK";
    if(custom==SMK37_NAV_PAGE)hint="K5 FAMILY / K6 PAGE / > OPEN";
    if(custom==SMK37_PREF_PAGE)hint="< BACK / > SAVE SETTINGS";
    if(custom==SMK37_BROWSER_PAGE)hint="< HOME / > LOAD / K8 FAV";
    if(custom==SMK37_UNDO_PAGE)hint="< PATTERN / > SOUND";
    if(custom==SMK37_PHRASE_PAGE)hint="K5 PHRASE / > LOAD / < BACK";
    if(custom==SMK37_NAME_PAGE)hint="< CANCEL / > RENAME";
    if(custom==SMK37_STORAGE_PAGE)hint=smk37_samples_owned?"< BACK / STUDIO UPLOAD":smk37_storage_confirm&&fm1_ms-smk37_storage_confirm_at<=5000u?"< CANCEL / > CONFIRM":"< BACK / > REVIEW";
    if(!smk37_help)hint="";
    if(!smk37_large_frame)smk37_draw_panel(track,custom,page,labels,formatted);
    smk37_draw_footer(engine,preset,hint);
}
