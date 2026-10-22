/* SPDX-License-Identifier: GPL-3.0-only */
#define main smk37_completion_baseline
#include "smk37_completion_test.c"
#undef main
int main(void){
    memset(flash,255,sizeof flash);smk37_synth_init();smk37_storage_ready=smk37_storage_writable=1;
    assert(!smk37_up_boot());song.g[G_BPM]=0;assert(beat_samples()==FS*60u/120u);song.g[G_BPM]=120;smk37_knob_bank=1;smk37_patch_mode=0;song.sel=0;
    /* The physical detail encoder reaches every condition, including FILL. */
    smk37_page=SMK37_DETAIL_PAGE;smk37_edit_step=0;step_set_chance(&TSEL->step[0],100);
    smk37_turn(4,1);assert(step_cond(&TSEL->step[0])==1);
    for(unsigned i=1;i<NCOND;++i)smk37_turn(4,1);
    assert(step_cond(&TSEL->step[0])==COND_NFILL);
    smk37_page=SMK37_TOOLS_PAGE;smk37_tool=TL_N;smk37_button_input(3,1);assert(seq_fill);
    smk37_page=0;smk37_button_input(3,0);assert(!seq_fill);
    /* Per-track routing survives a real packed project save/load. */
    song.g[G_ROUTE]=ROUTE_TRACK;smk37_page=SMK37_MIDI_CHANNEL_PAGE;
    for(unsigned i=0;i<8;++i)smk37_turn(4,1);assert(mch_get(0)==8);mch_set(1,MCH_OMNI);mch_set(2,MCH_OFF);
    assert(!smk37_session_save(0));memset(song.mch,0,sizeof song.mch);TSEL->step[0].probability=0;
    assert(!smk37_session_load(0));assert(mch_get(0)==8 && mch_get(1)==MCH_OMNI && mch_get(2)==MCH_OFF);
    assert(song.g[G_ROUTE]==ROUTE_TRACK && step_cond(&TSEL->step[0])==COND_NFILL);
    /* Double/half preserve condition bytes and remain a single undo action. */
    TSEL->p[P_SLEN]=8;TSEL->step[0].time=ST_NOTE;TSEL->step[0].n=1;TSEL->step[0].note[0]=60;
    smk37_tool=TL_DOUBLE;smk37_tool_action();assert(TSEL->p[P_SLEN]==16 && step_cond(&TSEL->step[8])==COND_NFILL);
    smk37_pattern_undo();assert(TSEL->p[P_SLEN]==8);
    smk37_tool=TL_HALF;smk37_tool_action();assert(TSEL->p[P_SLEN]==4);smk37_pattern_undo();assert(TSEL->p[P_SLEN]==8);
    step_t original[NSTEP];memcpy(original,TSEL->step,sizeof original);
    smk37_tool=TL_EUCLID;smk37_tool_action();assert(eu.on && smk37_page==SMK37_EUCLID_PAGE);
    smk37_turn(4,1);assert(eu.hits==5);smk37_button_event(2);assert(!eu.on && !memcmp(original,TSEL->step,sizeof original));
    smk37_button_event(3);assert(eu.on);smk37_button_event(3);assert(!eu.on);
    /* Palette preview is RAM only, store survives reboot, malformed colors do nothing. */
    uint8_t pal[16]={0,31,0,0,0,63,0,0,0,31,31,63,0,31,0,31};
    memcpy(saved,flash,sizeof flash);assert(req(81,pal,16) && !smk37_ed_out[5]);assert(smk37_preview_palette && !memcmp(saved,flash,sizeof flash));
    pal[0]=1;assert(req(81,pal,16) && !smk37_ed_out[5]);assert(cpal_on() && palette_cur()==UI_NPALETTES);
    memset(&favorites,0,sizeof favorites);assert(!smk37_up_boot());assert(cpal_on() && palette_cur()==UI_NPALETTES);
    uint8_t before[10];memcpy(before,ui_cpal,10);pal[1]=32;assert(req(81,pal,16) && smk37_ed_out[5]==1);assert(!memcmp(before,ui_cpal,10));
    uint8_t remove=2;assert(req(81,&remove,1) && !smk37_ed_out[5]);assert(!cpal_on() && palette_n()==UI_NPALETTES);
    assert(!smk37_up_boot() && !cpal_on());
    puts("SMK 1.6.1: physical conditions/Fill, MIDI routing save, double/half undo, Euclid cancel and palette persistence passed");
    return 0;
}
