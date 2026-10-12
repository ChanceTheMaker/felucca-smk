/* SPDX-License-Identifier: GPL-3.0-only
 * Preset-loading semantics derived from ui.c:
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hugelton Instruments */
/* SMK sound controls, serialized with the audio consumer. Encoder numbers are
 * electrical scan indices, not yet verified panel positions. No storage IO. */
static const page_t smk37_sound_pages[] = {
    {"MIX", 0, SC_TRACK, GR_NONE, {P_LEVEL,P_PAN,P_MUTE,P_TRANS}},
    {"ENGINE 1", 0, SC_ENGINE, GR_NONE, {P_E0,P_E1,P_E2,P_E3}},
    {"ENGINE 2", 0, SC_ENGINE, GR_NONE, {P_E4,P_E5,P_E6,P_E7}},
    {"ENVELOPE", 0, SC_TRACK, GR_NONE, {P_ATK,P_DEC,P_SUS,P_REL}},
    {"ENV DEST", 0, SC_TRACK, GR_NONE, {P_ED_FLT,P_ED_PIT,P_ED_SHP,P_ESYNC}},
    {"LFO", 0, SC_TRACK, GR_NONE, {P_LRATE,P_LWAVE,P_LPHASE,P_LFADE}},
    {"LFO DEST", 0, SC_TRACK, GR_NONE, {P_LD_PIT,P_LD_FLT,P_LD_SHP,P_LD_AMP}},
    {"EFFECTS", 0, SC_TRACK, GR_NONE, {P_DIST,P_CHOR,P_DLY,P_REV}},
    {"DELAY", 0, SC_GLOBAL, GR_NONE, {G_DTIME,G_DFDBK,G_DCOLOR,G_DMIX}},
    {"REVERB", 0, SC_GLOBAL, GR_NONE, {G_RTYPE,G_RSIZE,G_RDAMP,255}},
    {"CHORUS", 0, SC_GLOBAL, GR_NONE, {G_CRATE,G_CDEPTH,255,255}},
    {"VOICE", 0, SC_TRACK, GR_NONE, {P_VOICE,P_GLIDE,P_GLMODE,P_PRIO}},
    {"VOICE 2", 0, SC_TRACK, GR_NONE, {P_ALLOC,P_DETUNE,P_PAN,P_MUTE}},
    {"SCALE", 0, SC_TRACK, GR_NONE, {P_ROOT,P_SCALE,P_QUANT,P_TRANS}},
    {"CHORD", 0, SC_TRACK, GR_NONE, {P_CHRD,P_VOIC,255,255}},
    {"ARP", 0, SC_TRACK, GR_NONE, {P_AMODE,P_ARATE,P_AOCT,P_AGATE}},
    {"ARP 2", 0, SC_TRACK, GR_NONE, {P_ASWING,P_APROB,P_AHOLD,P_AORDER}},
    {"SLICER", 0, SC_TRACK, GR_NONE, {P_SLCR,P_SLPAT,P_SLRATE,P_SLDEPTH}},
    {"TEMPO", 0, SC_GLOBAL, GR_NONE, {G_BPM,G_SWING,G_TUNE,255}},
    {"MOD 1", 0, SC_TRACK, GR_NONE, {P_M1SRC,P_M1DST,P_M1AMT,255}},
    {"MOD 2", 0, SC_TRACK, GR_NONE, {P_M1SRC+3,P_M1DST+3,P_M1AMT+3,255}},
    {"MOD 3", 0, SC_TRACK, GR_NONE, {P_M1SRC+6,P_M1DST+6,P_M1AMT+6,255}},
    {"MOD 4", 0, SC_TRACK, GR_NONE, {P_M1SRC+9,P_M1DST+9,P_M1AMT+9,255}},
    {"SEQUENCER", 0, SC_TRACK, GR_NONE, {P_SLEN,P_SDIV,P_SSWING,P_SGATE}},
    {"STEP EDIT", 0, SC_TRACK, GR_NONE, {255,255,255,255}},
    {"CONTROLS", 0, SC_TRACK, GR_NONE, {255,255,255,255}},
    {"PROJECTS", 0, SC_TRACK, GR_NONE, {255,255,255,255}},
    {"INSERT",0,SC_TRACK,GR_NONE,{P_ITYPE,P_IA,P_IB,P_IC}},
    {"INSERT MIX",0,SC_TRACK,GR_NONE,{P_IMIX,P_FTYPE,P_SPRD,P_ESYNC}},
    {"LFO SYNC",0,SC_TRACK,GR_NONE,{P_LSYNC,P_LTRIG,P_LPOL,255}},
    {"DRUM LEVELS 1",0,SC_TRACK,GR_NONE,{P_LN0,P_LN1,P_LN2,P_LN3}},
    {"DRUM LEVELS 2",0,SC_TRACK,GR_NONE,{P_LN4,P_LN5,P_LN6,P_LN7}},
    {"STEP DETAIL",0,SC_TRACK,GR_NONE,{255,255,255,255}},
    {"MIDI",0,SC_GLOBAL,GR_NONE,{G_ROUTE,G_CLOCK,255,255}},
    {"MIDI LEARN",0,SC_TRACK,GR_NONE,{255,255,255,255}},
    {"CLICK / RECORD",0,SC_TRACK,GR_NONE,{255,255,255,255}},
    {"DISPLAY",0,SC_TRACK,GR_NONE,{255,255,255,255}},
    {"TIMING",0,SC_TRACK,GR_NONE,{P_SQNT,P_SDIV,P_SSWING,P_SGATE}},
    {"PERFORMANCE FX",0,SC_TRACK,GR_NONE,{255,255,255,255}},
    {"AUTOMATION",0,SC_TRACK,GR_NONE,{255,255,255,255}},

    {"SONG CHAIN",0,SC_TRACK,GR_NONE,{255,255,255,255}},
    {"SEQ TOOLS",0,SC_TRACK,GR_NONE,{255,255,255,255}},
    {"USER PRESETS",0,SC_TRACK,GR_NONE,{255,255,255,255}},
    {"NAVIGATE",0,SC_TRACK,GR_NONE,{255,255,255,255}},
    {"PRESET BROWSER",0,SC_TRACK,GR_NONE,{255,255,255,255}},
    {"OPERATOR",0,SC_TRACK,GR_NONE,{255,255,255,255}},
    {"OP ENV",0,SC_TRACK,GR_NONE,{255,255,255,255}},
    {"OPERATOR 2",0,SC_TRACK,GR_NONE,{255,255,255,255}},
    {"SLICES",0,SC_TRACK,GR_NONE,{255,255,255,255}},
    {"UNDO / REDO",0,SC_TRACK,GR_NONE,{255,255,255,255}},
    {"PRESET NAME",0,SC_TRACK,GR_NONE,{255,255,255,255}},
    {"SETTINGS",0,SC_TRACK,GR_NONE,{255,255,255,255}},
    {"PHRASES",0,SC_TRACK,GR_NONE,{255,255,255,255}},
    {"SAMPLE STORAGE",0,SC_TRACK,GR_NONE,{255,255,255,255}},

};
#define SMK37_PHRASE_PAGE 52u
#define SMK37_STORAGE_PAGE 53u
#define SMK37_NAV_PAGE 43u
#define SMK37_BROWSER_PAGE 44u
#define SMK37_OP_PAGE 45u
#define SMK37_OPENV_PAGE 46u
#define SMK37_OP2_PAGE 47u
#define SMK37_SLICE_PAGE 48u
#define SMK37_UNDO_PAGE 49u
#define SMK37_NAME_PAGE 50u
#define SMK37_PREF_PAGE 51u
#define SMK37_SONG_PAGE 40u
#define SMK37_TOOLS_PAGE 41u
#define SMK37_PRESET_PAGE 42u
#define SMK37_PERFORM_PAGE 38u
#define SMK37_AUTOMATION_PAGE 39u
#define SMK37_DETAIL_PAGE 32u
#define SMK37_LEARN_PAGE 34u
#define SMK37_CLICK_PAGE 35u
#define SMK37_DISPLAY_PAGE 36u
#define SMK37_STEP_PAGE 24u
#define SMK37_SETTINGS_PAGE 25u
#define SMK37_PROJECT_PAGE 26u
#define SMK37_SOUND_PAGES (sizeof smk37_sound_pages / sizeof smk37_sound_pages[0])
#define SMK37_ANALOG_PAGE SMK37_SOUND_PAGES
#define SMK37_BUTTON_PAGE (SMK37_SOUND_PAGES+1u)
#define SMK37_LIGHT_PAGE (SMK37_SOUND_PAGES+2u)
#define SMK37_PEDAL_PAGE (SMK37_SOUND_PAGES+3u)
#define SMK37_PAGE_COUNT (SMK37_SOUND_PAGES+4u)
static uint8_t smk37_page;
static uint8_t smk37_song_row,smk37_song_track,smk37_tool,smk37_lane,smk37_user_slot;
static volatile uint8_t smk37_song_request,smk37_user_request,smk37_song_loading;
static void smk37_tool_action(void);
static void smk37_pattern_undo(void);
static void smk37_pattern_gesture(track_t *,unsigned);
static uint8_t smk37_learn_on,smk37_learn_track,smk37_learn_param;
static uint8_t smk37_palette,smk37_bold;
static uint8_t smk37_auto_param,smk37_auto_lock;
static int16_t smk37_auto_value;
static uint8_t smk37_knob_bank, smk37_fader_bank, smk37_patch_mode = 1;
static uint8_t smk37_pad_bank, smk37_velocity_mode, smk37_edit_step, smk37_project_slot;
/* Requests are consumed by the main loop, never flash from the audio ISR. */
static volatile uint8_t smk37_project_request;
static uint8_t smk37_pedal_down;
static uint8_t smk37_saved_arp[NTRK], smk37_saved_chord[NTRK], smk37_saved_quant[NTRK];
static const char *smk37_notice;
static char smk37_master_notice[24];
static uint32_t smk37_notice_at;
/* Light diagnostics only: OFF, PAD1..16, INDICATOR1..19. */
static uint8_t smk37_light_target, smk37_light_color, smk37_light_level = 4;
static int8_t smk37_encoder_phase[8];

static unsigned smk37_wrap(unsigned value, int direction, unsigned count)
{
    return direction > 0 ? (value+1u)%count : (value+count-1u)%count;
}

static uint8_t smk37_undo_loading;
static void smk37_sound_take(track_t *,unsigned);
static int smk37_workflow_turn(unsigned,int);
static int smk37_workflow_button(unsigned);
static unsigned smk37_next_page(unsigned,int);
static unsigned smk37_edit_next(unsigned);
static uint8_t smk37_accel,smk37_bpm_lock,smk37_hold=1,smk37_home,smk37_screen_off,smk37_help=1,smk37_led_level=4;
static uint32_t smk37_input_at;
static uint8_t smk37_step_preview;
static void smk37_preview_step(void){
    if(smk37_step_preview && !song.playing && !song.rec && !seq_counting() && transport_req!=1 && !chain.running && !chain.armed &&
       (smk37_page==SMK37_STEP_PAGE || smk37_page==SMK37_DETAIL_PAGE))
        aud_req=(uint16_t)((unsigned)smk37_edit_step<<8 | (song.sel+1u));
}
/* Same sound/preserved-track semantics as ui.c apply_preset_to. */
static void smk37_load_preset(track_t *t, unsigned index)
{
    const engine_t *e = ENGINES[t->eng_req];
    if (!e->npresets) return;
    smk37_sound_take(t,0xffff);
    index %= e->npresets;
    if (e->presets == SMP_PRESET_TABLE && index < SMP_NSETS)
        index = SMP_SET_ORIG[index];
    const preset_t *p = &e->presets[index];
    panic_req |= (uint8_t)(1u << trk_index(t));
    t->preset = (uint8_t)index;
    t->user = 0;
    for (unsigned i = 0; i < P_E0; ++i)
        if (!(i == P_LEVEL || i == P_PAN || i == P_MUTE ||
              (i >= P_AMODE && i <= P_SGATE) ||
              (i >= P_SLCR && i <= P_SLDEPTH) || i == P_CHRD || i == P_VOIC))
            t->p[i] = TP[i].def;
    for (unsigned i = 0; i < 8; ++i) t->p[P_E0+i] = p->e[i];
    for (unsigned i = 0; i < 4; ++i) t->p[P_ATK+i] = p->env[i];
    t->p[P_ED_FLT] = p->fenv;
    t->p[P_VOICE] = p->mono ? V_LEGATO : V_POLY;
    static const uint8_t defaults[4] = {0,24,28,36};
    for (unsigned i = 0; i < 4; ++i)
        t->p[P_DIST+i] = p->fx[i] ? p->fx[i]-1 : defaults[i];
    fm6_track_loaded(t);
}

static void smk37_turn(unsigned encoder, int direction)
{
    if (encoder >= 8 || !direction || smk37_song_loading) return;
    direction = direction > 0 ? 1 : -1;
    smk37_input_at=fm1_ms;
    if(smk37_workflow_turn(encoder,direction))return;
    if (smk37_page == SMK37_LIGHT_PAGE && encoder != 3) {
        if (encoder == 0) smk37_light_target = smk37_wrap(smk37_light_target, direction, 36);
        if (encoder == 1) smk37_light_color = smk37_wrap(smk37_light_color, direction, 3);
        if (encoder == 2) smk37_light_level = clamp(smk37_light_level+direction, 1, 8);
        return;
    }
    if ((smk37_page == SMK37_BUTTON_PAGE || smk37_page == SMK37_PEDAL_PAGE) && encoder != 3) return;
    /* Bank 1 follows current FM-1 panel functions. Bank 2 exposes SMK
     * navigation; page controls remain physically K5..K8 in both banks. */
    if(smk37_page<SMK37_SOUND_PAGES && !smk37_knob_bank){
        if(encoder==0){
            char *notice=smk37_master_notice;song.master_q12=clamp((int)song.master_q12+direction*32,0,4096);
            str_cpy(notice,"MASTER ",sizeof smk37_master_notice);fmt_int(notice+7,(song.master_q12*100u+2048u)/4096u);
            str_cpy(notice+str_len(notice),"%",sizeof smk37_master_notice-str_len(notice));smk37_notice=notice;smk37_notice_at=fm1_ms;return;
        }
        if(encoder==1){if(smk37_bpm_lock){smk37_notice="BPM LOCK / USE TEMPO PAGE";smk37_notice_at=fm1_ms;return;}song.g[G_BPM]=clamp(song.g[G_BPM]+direction,GP[G_BPM].min,GP[G_BPM].max);smk37_notice="SELECT: TEMPO";smk37_notice_at=fm1_ms;return;}
        if(encoder==3){song.sel=clamp((int)song.sel+direction,0,NTRK-1);return;}
    }
    track_t *t = TSEL;
    if (encoder == 0) song.sel = smk37_wrap(song.sel, direction, NTRK);
    else if (encoder == 1) {
        unsigned engine = t->eng_req;
        do { engine = smk37_wrap(engine, direction, NENGINES); }
        while (!FELUCCA_FM4 && engine == ENGI_DIGITAL);
        smk37_sound_take(t,0xffff);
        smk37_undo_loading=1;
        t->eng_req = (uint8_t)engine;
        for (unsigned i = 0; i < 8; ++i) t->p[P_E0+i] = ENGINES[engine]->edit[i].def;
        smk37_load_preset(t, 0);
        smk37_undo_loading=0;
    } else if (encoder == 2) {
        const engine_t *e = ENGINES[t->eng_req];
        if (!e->npresets) return;
        unsigned p = t->preset;
        do { p = smk37_wrap(p, direction, e->npresets); }
        while (e->presets == SMP_PRESET_TABLE && p < SMP_NSETS && SMP_SET_ORIG[p] != p);
        smk37_load_preset(t, p);
    } else if (encoder == 3) {
        smk37_page = smk37_next_page(smk37_page,direction);
        smk37_patch_mode = 0;
    }
    else if (smk37_page < SMK37_SOUND_PAGES) {
        if(smk37_page==SMK37_SONG_PAGE){
            if(chain.running || chain.armed)return;
            if(encoder==4)smk37_song_row=smk37_wrap(smk37_song_row,direction,chain_config.count?chain_config.count:1);
            if(encoder==5)smk37_song_track=smk37_wrap(smk37_song_track,direction,NTRK);
            if(chain_config.count){chain_row_t *r=&chain_config.row[smk37_song_row];
                if(encoder==6)r->slot[smk37_song_track]=smk37_wrap(r->slot[smk37_song_track],direction,5);
                if(encoder==7)r->repeat=clamp(r->repeat+direction,1,16);}
            return;
        }
        if(smk37_page==SMK37_TOOLS_PAGE){
            if(encoder==4){do{smk37_tool=smk37_wrap(smk37_tool,direction,12);}while(smk37_tool==7);}
            if(encoder==5)smk37_lane=smk37_wrap(smk37_lane,direction,NLANE);return;
        }
        if(smk37_page==SMK37_PRESET_PAGE){if(encoder==4)smk37_user_slot=smk37_wrap(smk37_user_slot,direction,smk37_user_capacity);return;}
        if(smk37_page==SMK37_PERFORM_PAGE){perf_k[encoder-4]=clamp(perf_k[encoder-4]+direction,encoder==4?-100:0,100);return;}
        if(smk37_page==SMK37_AUTOMATION_PAGE){
            if(encoder==4)smk37_edit_step=smk37_wrap(smk37_edit_step,direction,NSTEP);
            if(encoder==5){do{smk37_auto_param=smk37_wrap(smk37_auto_param,direction,P_COUNT);}while(!motion_param(smk37_auto_param));}
            if(encoder==6){const param_desc_t *d=track_desc(t,smk37_auto_param);smk37_auto_value=clamp(smk37_auto_value+direction,d->min,d->max);}
            if(encoder==7)smk37_auto_lock=direction>0;
            return;
        }
        if (smk37_page==SMK37_DETAIL_PAGE) {
            smk37_pattern_gesture(t,smk37_page*512+smk37_edit_step*4+encoder);
            step_t *s=&t->step[smk37_edit_step];
            if(encoder==4)step_set_chance(s,clamp((int)step_chance(s)+direction,0,100));
            if(encoder==5)step_set_ratchet(s,clamp((int)step_ratchet(s)+direction,1,4));
            if(encoder==6)step_set_nudge(s,clamp(step_nudge(s)+direction,-8,7));
            if(encoder==7)s->vel=clamp(s->vel+direction,1,127);
            return;
        }
        if(smk37_page==SMK37_LEARN_PAGE){
            if(encoder==4)smk37_learn_on=direction>0;
            if(encoder==5)smk37_learn_track=smk37_wrap(smk37_learn_track,direction,NTRK);
            if(encoder==6)smk37_learn_param=smk37_wrap(smk37_learn_param,direction,P_COUNT);
            ml_heard=0;ml_arm=smk37_learn_on;return;
        }
        if(smk37_page==SMK37_CLICK_PAGE){
            if(encoder==4)click_mode=clamp(click_mode+direction,0,2);
            if(encoder==5)click_lvl=clamp(click_lvl+direction,0,2);
            if(encoder==6)cin_bars=clamp(cin_bars+direction,0,2);
            return;
        }
        if(smk37_page==SMK37_DISPLAY_PAGE){
            if(encoder==4)smk37_palette=smk37_wrap(smk37_palette,direction,23);
            if(encoder==5)smk37_bold=direction>0;
            return;
        }
        if (smk37_page==SMK37_SETTINGS_PAGE) {
            if(encoder==4) smk37_velocity_mode=smk37_wrap(smk37_velocity_mode,direction,4);
            if(encoder==5) smk37_pad_bank^=1;
            return;
        }
        if (smk37_page==SMK37_PROJECT_PAGE) {
            if(encoder==4) smk37_project_slot=smk37_wrap(smk37_project_slot,direction,4);
            return;
        }
        if (smk37_page==SMK37_STEP_PAGE) {
            if(encoder>4)smk37_pattern_gesture(t,smk37_page*512+smk37_edit_step*4+encoder);
            step_t *s=&TSEL->step[smk37_edit_step];
            if(encoder==4){smk37_edit_step=smk37_wrap(smk37_edit_step,direction,NSTEP);smk37_preview_step();}
            if(encoder==5) { s->note[0]=clamp(s->note[0]+direction,0,127); s->n=1; }
            if(encoder==6) s->vel=clamp(s->vel+direction,1,127);
            if(encoder==7) s->time=smk37_wrap(s->time,direction,3);
            return;
        }
        int16_t *value;
        const param_desc_t *d = page_desc(&smk37_sound_pages[smk37_page], encoder-4, &value);
        if (d && value) {
            if(chain.running && smk37_sound_pages[smk37_page].scope!=SC_GLOBAL && chain_owns(smk37_sound_pages[smk37_page].id[encoder-4]))return;
            if(*value!=clamp(*value+direction,d->min,d->max))smk37_sound_take(t,smk37_page*4+encoder);
            *value = (int16_t)enum_step(d, *value, clamp(*value+direction, d->min, d->max));
            const page_t *pg=&smk37_sound_pages[smk37_page];
            if(smk37_learn_on && pg->scope!=SC_GLOBAL){smk37_learn_track=song.sel;smk37_learn_param=pg->id[encoder-4];ml_heard=0;ml_arm=1;}
        }
    }
}

/* Owner-confirmed physical button IDs B01..B18. Press edges only, serialized
 * with notes in the audio consumer. Diagnostics deliberately ignore actions. */
static void smk37_button_event(unsigned button)
{
    if (button >= 18 || smk37_page >= SMK37_SOUND_PAGES || smk37_song_loading) return;
    smk37_input_at=fm1_ms;
    if(smk37_workflow_button(button))return;
    track_t *t=TSEL;
    unsigned part=song.sel%NTRK;
    int direction=button==2 ? -1 : 1;
    if(smk37_page==SMK37_SONG_PAGE && (button==2 || button==3)){
        if(chain.running || chain.armed)return;
        if(button==3 && chain_config.count<CHAIN_ROWS){chain_config.row[chain_config.count]=chain_row_of(smk37_project_slot,1);smk37_song_row=chain_config.count++;}
        if(button==2 && chain_config.count){for(unsigned i=smk37_song_row;i+1<chain_config.count;++i)chain_config.row[i]=chain_config.row[i+1];--chain_config.count;if(smk37_song_row && smk37_song_row>=chain_config.count)--smk37_song_row;}
        return;
    }
    if(smk37_page==SMK37_TOOLS_PAGE && (button==2 || button==3)){if(button==2)smk37_pattern_undo();else smk37_tool_action();return;}
    if(smk37_page==SMK37_PRESET_PAGE && (button==2 || button==3)){
        if(!smk37_user_request)smk37_user_request=(smk37_user_slot<<2)|(button==2?1:2);return;
    }
    if(smk37_page==SMK37_AUTOMATION_PAGE && (button==2 || button==3)){
        smk37_pattern_gesture(t,0xffff);
        if(button==2){motion_delete_event(t,smk37_edit_step,smk37_auto_param);smk37_notice="EVENT REMOVED";}
        else{unsigned rc=smk37_auto_lock?motion_set_lock(t,smk37_edit_step,smk37_auto_param,smk37_auto_value):motion_set_event(t,smk37_edit_step,smk37_auto_param,smk37_auto_value);
            motion_set_enabled(t,1);smk37_notice=rc?"INVALID / AUTOMATION FULL":"EVENT SET / SAVE PROJECT";}
        smk37_notice_at=fm1_ms;return;
    }
    if(smk37_page==SMK37_PROJECT_PAGE && (button==2 || button==3)) {
        if(!smk37_project_request) smk37_project_request=(uint8_t)((smk37_project_slot<<2)|(button==2 ? 1u : 2u));
        return; /* < load, > save; explicit presses, no autosave */
    }
    if((smk37_page==SMK37_STEP_PAGE || smk37_page==SMK37_DETAIL_PAGE) && (button==2 || button==3)) {
        smk37_edit_step=(smk37_edit_step+NSTEP+direction*16)%NSTEP;smk37_preview_step();
        return;
    }
    if(button>=6 && button<=9)smk37_sound_take(t,0xffff);
    switch (button) {
    case 0:
        smk37_knob_bank ^= 1;
        smk37_notice=smk37_knob_bank?"BANK 2: TRACK ENGINE PAGE":"BANK 1: FM-1 CONTROLS";smk37_notice_at=fm1_ms;
        memset(smk37_encoder_phase, 0, sizeof smk37_encoder_phase);
        break;
    case 1: smk37_fader_bank ^= 1; break;
    case 2: case 3:
        if (smk37_patch_mode) {
            /* Preset navigation independent of the physical encoder bank. */
            uint8_t bank=smk37_knob_bank; smk37_knob_bank=0;
            smk37_turn(2,direction); smk37_knob_bank=bank;
        } else smk37_page=smk37_next_page(smk37_page,direction);
        break;
    case 4: song.octave=clamp(song.octave-1,-3,3); break;
    case 5: song.octave=clamp(song.octave+1,-3,3); break;
    case 6:
        if (t->p[P_AMODE] && t->p[P_AMODE]!=6) {
            smk37_saved_arp[part]=t->p[P_AMODE]; t->p[P_AMODE]=0;
        } else t->p[P_AMODE]=smk37_saved_arp[part] ? smk37_saved_arp[part] : 1;
        smk37_page=15; smk37_patch_mode=0; break;
    case 7:
        if (t->p[P_AMODE]==6) t->p[P_AMODE]=0;
        else {
            if (t->p[P_AMODE]) smk37_saved_arp[part]=t->p[P_AMODE];
            t->p[P_AMODE]=6;
        }
        smk37_page=15; smk37_patch_mode=0; break;
    case 8:
        if (t->p[P_QUANT]) { smk37_saved_quant[part]=t->p[P_QUANT]; t->p[P_QUANT]=0; }
        else {
            if (!t->p[P_SCALE]) t->p[P_SCALE]=1; /* useful initial C major */
            t->p[P_QUANT]=smk37_saved_quant[part] ? smk37_saved_quant[part] : QN_SNAP;
        }
        smk37_page=13; smk37_patch_mode=0; break;
    case 9:
        if (t->p[P_CHRD]) { smk37_saved_chord[part]=t->p[P_CHRD]; t->p[P_CHRD]=0; }
        else t->p[P_CHRD]=smk37_saved_chord[part] ? smk37_saved_chord[part] : 3; /* major */
        smk37_page=14; smk37_patch_mode=0; break;
    case 10: smk37_page=smk37_page==18 ? SMK37_SETTINGS_PAGE : smk37_page==SMK37_SETTINGS_PAGE ? SMK37_PROJECT_PAGE : 18; smk37_patch_mode=0; break;
    case 11: smk37_page=smk37_edit_next(smk37_page); smk37_patch_mode=0; break; /* BT: edit; radio remains disabled */
    case 12: smk37_page=0; smk37_patch_mode=1; break;
    case 13: smk37_page=SMK37_PROJECT_PAGE; smk37_patch_mode=0; break; /* PARA: save menu, no write */
    case 14: smk37_page=smk37_page==7?27:smk37_page==27?SMK37_PERFORM_PAGE:7; smk37_patch_mode=0; break;
    case 15: smk37_page=smk37_page==23 ? SMK37_STEP_PAGE : smk37_page==SMK37_STEP_PAGE ? SMK37_DETAIL_PAGE : smk37_page==SMK37_DETAIL_PAGE ? SMK37_AUTOMATION_PAGE : 23; smk37_patch_mode=0; break;
    case 16:
        if(smk37_page==SMK37_SONG_PAGE && !song.playing && !transport_req){smk37_song_request=1;break;}
        transport_req=(song.playing || transport_req==1) ? 2 : 1;
        if (transport_req==2) { song.rec=0; for(unsigned i=0;i<NTRK;++i) trk[i].rh_n=0; }
        break;
    case 17:
        song.rec ^= (uint8_t)(1u<<part);
        if (!(song.rec&(1u<<part))) t->rh_n=0;
        else if (!song.playing) transport_req=1;
        smk37_page=23; smk37_patch_mode=0;
        smk37_notice="RECORDING IS RAM ONLY"; smk37_notice_at=fm1_ms;
        break;
    }
}
static uint32_t smk37_indicator_status(void)
{
    if (smk37_page>=SMK37_SOUND_PAGES) return 0;
    const track_t *t=TSEL;
    uint32_t bits=0;
    if (smk37_knob_bank) bits|=1u;
    if (smk37_fader_bank) bits|=1u<<1;
    if (song.octave<0) bits|=1u<<4;
    if (song.octave>0) bits|=1u<<5;
    if (t->p[P_AMODE] && t->p[P_AMODE]!=6) bits|=1u<<6;
    if (t->p[P_AMODE]==6) bits|=1u<<7;
    if (t->p[P_QUANT]) bits|=1u<<8;
    if (t->p[P_CHRD]) bits|=1u<<9;
    if (smk37_page==18 || smk37_page==SMK37_SETTINGS_PAGE || smk37_page==SMK37_PROJECT_PAGE || smk37_page==SMK37_NAV_PAGE || smk37_page==SMK37_PREF_PAGE) bits|=1u<<10;
    if (smk37_patch_mode || smk37_page==SMK37_BROWSER_PAGE) bits|=1u<<12;
    if ((smk37_page>=1 && smk37_page<=6) || (smk37_page>=19 && smk37_page<=22) || (smk37_page>=SMK37_OP_PAGE && smk37_page<=SMK37_SLICE_PAGE)) bits|=1u<<11;
    if (smk37_page==SMK37_PROJECT_PAGE) bits|=1u<<13;
    if ((smk37_page>=7 && smk37_page<=10) || smk37_page==27 || smk37_page==28 || smk37_page==SMK37_PERFORM_PAGE) bits|=1u<<14;
    if (smk37_page==23 || smk37_page==SMK37_STEP_PAGE || smk37_page==SMK37_DETAIL_PAGE || smk37_page==SMK37_AUTOMATION_PAGE) bits|=1u<<15;
    if (song.playing) bits|=1u<<(song.rec ? 17 : 16);
    if (song.rec) bits|=1u<<18;
    return bits;
}

/* Four valid quadrature transitions per step, pending physical confirmation. */
static void smk37_encoder_event(unsigned encoder, int direction)
{
    if (encoder >= 8 || (direction != 1 && direction != -1)) return;
    int phase = smk37_encoder_phase[encoder] + direction;
    if (phase == 4 || phase == -4) {
        static uint32_t last[8];
        unsigned steps=smk37_accel && fm1_ms-last[encoder]<60u && encoder>=4 && smk37_page<SMK37_NAV_PAGE ? 3:1;
        last[encoder]=fm1_ms;
        while(steps--)smk37_turn(encoder, direction);
        phase = 0;
    }
    smk37_encoder_phase[encoder] = (int8_t)phase;
}
