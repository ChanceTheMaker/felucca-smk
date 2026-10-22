/* SPDX-License-Identifier: GPL-3.0-only
 * Grid helpers and tools from ui.c/ui_tools.c, Copyright (C) 2026
 * Leo Kuroshita (@kurogedelic), Hugelton Instruments. */
static struct { step_t step[NSTEP]; int16_t timing[4]; motion_store_t mo; uint8_t track,valid; }
    smk37_undo __attribute__((section(".pool")));
static motion_store_t smk37_undo_swap_motion __attribute__((section(".pool")));
static void smk37_pattern_take(track_t *t) {
    smk37_undo.track=trk_index(t);smk37_undo.valid=1;
    memcpy(smk37_undo.step,t->step,sizeof t->step);
    for(unsigned i=0;i<4;++i)smk37_undo.timing[i]=t->p[P_SLEN+i];
    motion_snapshot_track(t,&smk37_undo.mo);
}
static void smk37_pattern_gesture(track_t *t,unsigned gesture){
    static uint32_t last,at;static uint8_t track;
    if(!smk37_undo.valid || gesture==0xffff || last!=gesture || track!=trk_index(t) || fm1_ms-at>=800u)smk37_pattern_take(t);
    last=gesture;at=fm1_ms;track=trk_index(t);
}
static void smk37_pattern_undo(void) {
    if(!smk37_undo.valid || chain.running || chain.armed)return;
    track_t *t=&trk[smk37_undo.track];
    motion_snapshot_track(t,&smk37_undo_swap_motion);
    if(motion_replace_track(t,&smk37_undo.mo)){smk37_notice="UNDO: AUTOMATION FULL";smk37_notice_at=fm1_ms;return;}
    for(unsigned i=0;i<NSTEP;++i){step_t s=t->step[i];t->step[i]=smk37_undo.step[i];smk37_undo.step[i]=s;}
    smk37_undo.mo=smk37_undo_swap_motion;
    for(unsigned i=0;i<4;++i){int16_t v=t->p[P_SLEN+i];t->p[P_SLEN+i]=smk37_undo.timing[i];smk37_undo.timing[i]=v;}
    smk37_notice="UNDO / REDO";smk37_notice_at=fm1_ms;
}
static char smk37_message[64];
static void ui_message(const char *s){str_cpy(smk37_message,s,sizeof smk37_message);smk37_notice=smk37_message;smk37_notice_at=fm1_ms;}
static uint32_t steps_sig(const track_t *t){
    uint32_t h=2166136261u;const uint8_t *p=(const uint8_t *)t->step;
    for(unsigned i=0;i<sizeof t->step;++i)h=(h^p[i])*16777619u;return h;
}
static void step_clear(step_t *st)
{
    st->n = 0;
    st->time = ST_REST;
    st->flags = 0;
    st->vel = 0;
    st->hit = st->acc = 0;
    st->probability = 0;
}

static void grid_own(step_t *s, uint32_t l)
{
    uint32_t k, j = 0;
    step_to_grid(s);
    for (k = 0; k < s->n; k++)
        if (drum_lane(s->note[k]) == l)
            s->hit |= (uint8_t)(1u << l);
        else
            s->note[j++] = s->note[k];
    for (k = j; k < 4u; k++)
        s->note[k] = 0;
    s->n = (uint8_t)j;
}

/* lane l of step i: on 1, off 0, toggled 2 */
static void grid_hit(track_t *t, uint32_t i, uint32_t l, uint32_t on)
{
    step_t *s = &t->step[i % NSTEP];
    uint32_t b = 1u << (l % NLANE);
    if (on == 2u)
        on = !(step_lanes(s) & b);
    if (s->time != ST_NOTE) {                    /* a REST or a TIE: an empty step (nothing to turn off) */
        if (!on)
            return;
        step_clear(s);
        s->time = ST_NOTE;
    }
    grid_own(s, l % NLANE);
    if (on) {
        s->hit |= (uint8_t)b;
    } else {
        s->hit &= (uint8_t)~b;
        s->acc &= (uint8_t)~b;
        if (!s->n && !s->hit)
            step_clear(s);
    }
}

/* the accent of lane l at step i (on 1, off 0, toggled 2); an accent on an empty lane adds the hit */
static void grid_acc(track_t *t, uint32_t i, uint32_t l, uint32_t on)
{
    step_t *s = &t->step[i % NSTEP];
    uint32_t b = 1u << (l % NLANE);
    if (on == 2u)
        on = !(step_accents(s) & b);
    if (on)
        grid_hit(t, i, l, 1);
    if (!(step_lanes(s) & b))
        return;
    grid_own(s, l % NLANE);
    if (s->flags & SF_ACCENT) {                  /* a step accent: each hit's own from now on */
        s->acc |= s->hit;
        s->flags &= (uint8_t)~SF_ACCENT;
    }
    s->acc = (uint8_t)(on ? s->acc | b : s->acc & ~b);
}


#define FELUCCA_TOOLS_UNDO(t) smk37_pattern_take(t)
#define FELUCCA_TOOLS_STEPS smk37_undo.step
/* SMK has spare ordinary RAM; keep these two new tool buffers out of the
 * pool shared with the remote screen. Linker boundaries remain unchanged. */
#define FELUCCA_TOOLS_SCRATCH
#define FELUCCA_TOOLS_LANE smk37_lane
#include "ui_tools.c"
#undef FELUCCA_TOOLS_LANE
#undef FELUCCA_TOOLS_SCRATCH
#undef FELUCCA_TOOLS_UNDO
#undef FELUCCA_TOOLS_STEPS
static void smk37_tool_action(void){
    if(chain.running || chain.armed){ui_message("STOP SONG FIRST");return;}
    if(smk37_tool==TL_N){ui_message("HOLD > FOR FILL");return;}
    if(!tl_cell(TSEL,smk37_tool)){ui_message("DRUM TOOL / SELECT DRUM");return;}
    if(smk37_tool==TL_EUCLID){eu_open(TSEL);smk37_page=SMK37_EUCLID_PAGE;return;}
    const char *s=tl_do(TSEL,smk37_tool,smk37_lane);if(s)ui_message(s);
}
