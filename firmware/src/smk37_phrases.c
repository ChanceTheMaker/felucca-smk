/* SPDX-License-Identifier: GPL-3.0-only
 * Factory phrase loading from ui.c, Copyright (C) 2026 Leo Kuroshita
 * (@kurogedelic), Hugelton Instruments. SMK panel adapter by Chance Roth. */
static int seq_is_empty(const track_t *t)
{
    uint32_t i;
    for (i = 0; i < NSTEP; i++)
        if (t->step[i].n || t->step[i].hit)
            return 0;
    return 1;
}
/* Compact read-only mirror published with preset names; panel never reads flash. */
static struct {uint8_t note[16],flags[16],used,grid;int16_t timing[4];char name[13];} smk37_user_phrase[SMK37_UP_SLOTS];
static unsigned smk37_phrase_count(void){unsigned n=NPATTERNS;for(unsigned i=0;i<smk37_user_capacity;++i)n+=smk37_user_phrase[i].used;return n;}
static unsigned smk37_phrase_slot(unsigned p){if(p<NPATTERNS)return SMK37_UP_SLOTS;p-=NPATTERNS;for(unsigned i=0;i<smk37_user_capacity;++i)if(smk37_user_phrase[i].used){if(!p)return i;--p;}return SMK37_UP_SLOTS;}
static const char *smk37_phrase_name(unsigned p){unsigned k=smk37_phrase_slot(p);return p<NPATTERNS?PATTERNS[p].name:k<smk37_user_capacity?smk37_user_phrase[k].name:"EMPTY";}
static uint8_t smk37_phrase;
static void load_grid16(track_t *t, const uint8_t *hit, const uint8_t *acc)
{
    uint32_t i;
    for (i = 0; i < NSTEP; i++) {
        step_t *s = &t->step[i];
        step_clear(s);
        if (i < 16u && hit[i]) {
            s->time = ST_NOTE;
            s->hit = hit[i];
            s->acc = acc[i] & hit[i];
        }
    }
    t->p[P_SLEN] = 16;
}


static uint32_t smk37_phrase_sig[NTRK],smk37_phrase_confirm_sig,smk37_phrase_confirm_at;
static uint8_t smk37_phrase_confirm,smk37_phrase_confirm_track;
static void load_pat16(track_t *t, const uint8_t *note, const uint8_t *flags)
{
    uint32_t i;
    for (i = 0; i < NSTEP; i++) {
        step_t *s = &t->step[i];
        uint8_t n = i < 16u ? note[i] : 0, fl = i < 16u ? flags[i] : 0;
        s->note[0] = n;
        s->n = n ? 1 : 0;
        s->time = (fl & 4u) ? ST_TIE : n ? ST_NOTE : ST_REST;
        s->flags = n ? (fl & (SF_ACCENT | SF_SLIDE | SF_RATCH)) : 0;
        s->vel = n ? 96 : 0;
        s->hit = s->acc = 0;
        s->probability = 0;
        if (drum_track(t))                           /* a DRUM track: the lanes' notes as its grid */
            step_to_grid(s);
    }
    t->p[P_SLEN] = 16;
}


static void smk37_phrase_load(void){
    track_t *t=TSEL;unsigned k=trk_index(t),user=smk37_phrase_slot(smk37_phrase);uint32_t sig=steps_sig(t);
    if(smk37_phrase>=NPATTERNS && user>=smk37_user_capacity){ui_message("PHRASE EMPTY");return;}
    if(chain.running || chain.armed || song.rec){ui_message("STOP SONG / RECORD FIRST");return;}
    /* Same upstream safeguard for replacing edited notes. Confirmation is
     * bound to track, phrase and pattern signature, expires, and has undo. */
    if(!seq_is_empty(t) && sig!=smk37_phrase_sig[k] &&
       (smk37_phrase_confirm!=smk37_phrase+1 || smk37_phrase_confirm_track!=k ||
        smk37_phrase_confirm_sig!=sig || fm1_ms-smk37_phrase_confirm_at>3000u)){
        smk37_phrase_confirm=smk37_phrase+1;smk37_phrase_confirm_track=k;
        smk37_phrase_confirm_sig=sig;smk37_phrase_confirm_at=fm1_ms;
        ui_message("> AGAIN: REPLACE STEPS");return;
    }
    smk37_pattern_take(t);motion_restore(t);motion_reset(t);
    if(smk37_phrase<NPATTERNS)load_pat16(t,PATTERNS[smk37_phrase].note,PATTERNS[smk37_phrase].flags);
    else {
        if(smk37_user_phrase[user].grid)load_grid16(t,smk37_user_phrase[user].note,smk37_user_phrase[user].flags);
        else load_pat16(t,smk37_user_phrase[user].note,smk37_user_phrase[user].flags);
        for(unsigned i=0;i<4;++i)t->p[P_SLEN+i]=smk37_user_phrase[user].timing[i];
    }
    smk37_phrase_sig[k]=steps_sig(t);smk37_phrase_confirm=0;
    ui_message("PHRASE LOADED / UNDO READY");
}
