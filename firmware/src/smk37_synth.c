/* SPDX-License-Identifier: GPL-3.0-only */
/* SMK application sound layer. Included by smk37_app.c and the host integration
 * test. All note/chord ownership and synthesis reuse Felucca's actual engine.
 * The hardware scanner must call smk37_key_event from the audio consumer,
 * never from a nested scanner IRQ. */
#define FELUCCA_KEY_BASE 48
#define FELUCCA_PIANO_KEYS 37u
#define FELUCCA_KEY_COUNT 53u /* 37 piano keys plus 16 independent pad owners */
#include "felucca_tables.h"
#ifdef SMK37_SYNTH_HOST_TEST
#include <string.h>
#define memset felucca_memset
#define memcpy felucca_memcpy
#define memcmp felucca_memcmp
#endif
#include "libc.c"
#ifdef SMK37_SYNTH_HOST_TEST
#undef memset
#undef memcpy
#undef memcmp
#endif
static struct { uint32_t notes, buttons; } fm1_in; /* unused FM-1 UI layers */
static const uint8_t smk37_empty_sample[512];
#ifndef SMK37_ENABLE_SAMPLES
#define SMK37_ENABLE_SAMPLES 0
#endif
static uint8_t smk37_samples_owned,smk37_samples_state;
static volatile uint8_t smk37_samples_request;
#ifndef SMK37_ENABLE_TRS
#define SMK37_ENABLE_TRS 0
#endif
#ifndef SMK37_EXPANDED_PRESETS
#define SMK37_EXPANDED_PRESETS 0
#endif
#define SMK37_UP_SLOTS (SMK37_EXPANDED_PRESETS?32u:8u)
static uint8_t smk37_user_capacity=8;
static uint8_t smk37_trs_enabled;
static uint32_t smk37_trs_bytes;
#if SMK37_ENABLE_TRS
static void smk37_trs_output(uint32_t packet);
#define FELUCCA_MIDI_OUTPUT_TAP(packet) smk37_trs_output(packet)
#endif
#if SMK37_ENABLE_SAMPLES
static const uint8_t *smk37_sample_xip(unsigned k);
#define SMP_USER_XIP(k) smk37_sample_xip(k)
#else
#define SMP_USER_XIP(k) (smk37_empty_sample)
#endif
#include "core.h"
#include "engines.c"
#include "params.c"
#include "mod.c"
#include "voice.c"
#include "slicer.c"
#include "fx.c"
#ifdef SMK37_SYNTH_HOST_TEST
#define MQ 64u
static uint32_t midi_in_q[MQ], midi_in_ms[MQ], mi_r, mi_w;
static uint8_t midi_in_source[MQ], midi_in_overflow;
static void midi_out_event(uint32_t packet);
#else
#include "usb.c"
#endif
#include "song_chain.c"
/* SMK keys use per-key events, not the FM-1's 27-bit GPIO mask. */
#define FELUCCA_KEYBOARD_BLOCK() ((void)0)
#include "seq.c"
static uint8_t smk37_learn_map[32];
static volatile uint8_t smk37_learn_io[2], smk37_program_io[NTRK];
static uint8_t *const ml_tab=smk37_learn_map;
static volatile uint8_t *const ml_io=smk37_learn_io;
static volatile uint8_t *const mpc_io=smk37_program_io;
static unsigned smk37_learn_set(unsigned cc,unsigned track,unsigned id) {
    if(!ml_free_cc(cc) || track>=NTRK || id>=P_COUNT)return 1;
    unsigned code=id>=P_E0 ? ML_E0+id-P_E0 : id+1u;
    for(unsigned i=0;i<ML_N;++i){unsigned e=ml_tab[i*2]|ml_tab[i*2+1]<<8;
        if((e>>9) && ((e&127)==cc || ((e>>9)==code && ((e>>7)&3)==track)))ml_tab[i*2]=ml_tab[i*2+1]=0;
    }
    for(unsigned i=0;i<ML_N;++i)if(!(ml_tab[i*2+1]&254u)){
        unsigned e=cc|(track<<7)|(code<<9);ml_tab[i*2]=e;ml_tab[i*2+1]=e>>8;return 0;
    }
    return 5;
}
#include "smk37_ui.c"
#include "smk37_tools.c"
#include "smk37_phrases.c"
#include "smk37_workflow.c"
static void smk37_performance_poll(void) {
    for(unsigned k=0;k<NTRK;++k)if(mpc_io[k]){
        unsigned n=mpc_io[k]&127u;mpc_io[k]=0;track_t *t=&trk[k];
        if(t->eng_req==ENGI_DRUM){const param_desc_t *d=param_desc_of(ENGI_DRUM,P_E0);
            for(int v=d->min;v<=d->max;++v)if(enum_orig(d,v)==v && n--==0){t->p[P_E0]=v;break;}
        }else for(unsigned i=0;i<ENGINES[t->eng_req]->npresets;++i){
            if(ENGINES[t->eng_req]->presets==SMP_PRESET_TABLE && i<SMP_NSETS && SMP_SET_ORIG[i]!=i)continue;
            if(n--==0){smk37_load_preset(t,i);break;}
        }
    }
    if(ml_heard && smk37_learn_on){
        unsigned cc=ml_heard-1u;ml_heard=ml_arm=0;
        smk37_notice=smk37_learn_set(cc,smk37_learn_track,smk37_learn_param) ? "LEARN MAP FULL" : "CC LEARNED / SAVE PROJECT";
        smk37_notice_at=fm1_ms;
    }
}
#include "smk37_analog_controls.h"

static void smk37_synth_init(void)
{
    uint32_t k, i;
    for (i = 0; i < G_COUNT; ++i) song.g[i] = GP[i].def;
    fm6_init();
    for (k = 0; k < NTRK; ++k) {
        track_t *t = &trk[k];
        for (i = 0; i < P_E0; ++i) t->p[i] = TP[i].def;
        smk37_load_preset(t, 0);
        for (i = 0; i < NSTEP; ++i) t->step[i].time = ST_REST;
    }
    /* Moderate bench level until the actual DAC output has been measured. */
    song.master_q12 = 1024;
    smk37_sound_undo.valid=0;
}

/* Velocity zero releases the original track/note even if selection or octave
 * changed while held. Duplicate contact events cannot retrigger a held key.
 * Called serially with mix_block; no pointers to scanner DMA buffers retained. */
static void smk37_key_event(unsigned key, unsigned note, unsigned velocity)
{
    if (key >= 37u || note > 127 || velocity > 127) return;
    if (velocity) {
        if (kb_chn[key]) return;
        kb_trk[key] = song.sel % NTRK;
        kb_note[key] = (uint8_t)note;
        key_on_velocity(key, &trk[kb_trk[key]], velocity);
    } else if (kb_chn[key]) {
        key_off(key, &trk[kb_trk[key]]);
    }
}

/* Sustain keeps the key's original track/chord ownership until pedal-up.
 * Same-key retrigger first releases its pedal-held owner, avoiding stuck notes. */
static uint8_t smk37_pedal_pending[37];
static void smk37_sustain(unsigned down)
{
    down=!!down;
    if(down==smk37_pedal_down) return;
    smk37_pedal_down=(uint8_t)down;
    for(unsigned i=0;i<NTRK;++i)
        midi_out_event(0x0bu | (0xb0u|trk_midi_ch(i))<<8 | 64u<<16 | (down?127u:0u)<<24);
    if(!down) for(unsigned k=0;k<37;++k) if(smk37_pedal_pending[k]) {
        smk37_pedal_pending[k]=0; smk37_key_event(k,48+k,0);
    }
}

/* SMK C3..C6 base, preserving the original owner on release. Scale behavior
 * follows seq.c kb_map, adapted from its FM-1 F3 base to absolute SMK notes. */
static void smk37_play_key_event(unsigned key, unsigned velocity)
{
    if (key>=37) return;
    if(velocity)smk37_input_at=fm1_ms;
    if (!velocity) {
        if(smk37_pedal_down && kb_chn[key]) smk37_pedal_pending[key]=1;
        else smk37_key_event(key,48+key,0);
        return;
    }
    if(smk37_pedal_pending[key]) {
        smk37_pedal_pending[key]=0; smk37_key_event(key,48+key,0);
    }
    const track_t *t=TSEL;
    int note=48+(int)key;
    if (t->p[P_QUANT]==QN_WHITE) {
        static const int8_t degree[12]={0,-1,1,-1,2,3,-1,4,-1,5,-1,6};
        int d=degree[note%12], count=0;
        if (d<0) return;
        d+=(note/12-5)*7;
        uint32_t mask=scale_mask(t);
        for(unsigned i=0;i<12;++i) count+=(mask>>i)&1u;
        if (!count) return;
        int oct=d/count; d%=count;
        if(d<0) { d+=count; --oct; }
        unsigned semitone=0;
        for(;semitone<12;++semitone) if(mask&(1u<<semitone)) { if(!d) break; --d; }
        note=60+12*oct+t->p[P_ROOT]+semitone+12*song.octave+t->p[P_TRANS];
    } else {
        note+=12*song.octave+t->p[P_TRANS];
        if(t->p[P_QUANT]==QN_SNAP || t->p[P_QUANT]==QN_SEQ) note=scale_snap(t,note);
    }
    smk37_key_event(key,(unsigned)clamp(note,0,127),velocity);
}

/* Pads reuse local ownership so overlapping keyboard/MIDI notes release safely.
 * Provisional notes match the owner's stock bank-1 capture, on selected part. */
static uint32_t smk37_pad_hit_at[16];
static uint16_t smk37_pad_seen,smk37_perf_pads;
static void smk37_pad_event(unsigned pad, unsigned velocity)
{
    static const uint8_t notes[16] = {40,41,42,43,48,49,50,51,36,37,38,39,44,45,46,47};
    if (pad >= 16 || velocity > 127) return;
    if(velocity)smk37_input_at=fm1_ms;
    unsigned key = 37u+pad;
    if(!velocity && (smk37_perf_pads&(1u<<pad))){smk37_perf_pads&=~(1u<<pad);perf_press(pad<PF_NFX?pad:PF_M1+pad-14,0);return;}
    if(velocity && smk37_page==SMK37_PERFORM_PAGE){
        if(!(smk37_perf_pads&(1u<<pad))){smk37_perf_pads|=1u<<pad;perf_press(pad<PF_NFX?pad:PF_M1+pad-14,1);}return;
    }
    if(velocity && smk37_page==SMK37_DETAIL_PAGE){smk37_edit_step=(smk37_edit_step/16u)*16u+pad;smk37_preview_step();return;}
    if(velocity && smk37_page==SMK37_STEP_PAGE) {
        smk37_pattern_gesture(TSEL,0xffff);
        unsigned step=(smk37_edit_step/16u)*16u+pad;
        step_t *s=&TSEL->step[step];
        if(s->time==ST_NOTE && (s->n || s->hit)) s->time=ST_REST;
        else { s->time=ST_NOTE; s->n=1; s->note[0]=last_note; s->vel=(uint8_t)velocity; }
        smk37_edit_step=(uint8_t)step;smk37_preview_step();
        return;
    }
    if (velocity) {
        if (kb_chn[key]) return;
        smk37_pad_hit_at[pad]=fm1_ms;
        smk37_pad_seen |= (uint16_t)(1u<<pad);
        kb_trk[key] = song.sel % NTRK;
        kb_note[key] = notes[pad]+(smk37_pad_bank ? 16u : 0u);
        key_on_velocity(key, &trk[kb_trk[key]], velocity);
    } else if (kb_chn[key]) key_off(key, &trk[kb_trk[key]]);
}

/* Audio-consumer only. Local wheel follows selected part and parts with local
 * keys/pads held. Existing MIDI input can subsequently replace expression. */
static void smk37_wheels(int16_t bend, uint8_t wheel, unsigned targets)
{
    for (unsigned i=0; i<NTRK; ++i) if (targets & (1u<<i)) {
        if (!drum_track(&trk[i]))
            midi_bend_target[i] = (int32_t)bend*512/(bend<0 ? 8192 : 8191);
        mod_midi(&trk[i], 0xb0, 1, wheel);
        unsigned ch=trk_midi_ch(i),bend14=(unsigned)(bend+8192);
        midi_out_event(0x0eu|(0xe0u|ch)<<8|(bend14&127u)<<16|(bend14>>7)<<24);
        midi_out_event(0x0bu|(0xb0u|ch)<<8|1u<<16|(uint32_t)wheel<<24);
    }
}

/* Continuous controls are latest-value snapshots, separate from queued pad
 * edges. Faders take over only after movement; startup must retain preset level. */
static uint16_t smk37_analog_anchor[6];
static uint8_t smk37_analog_ready;
static unsigned smk37_wheel_targets;
static int16_t smk37_local_bend;
static uint8_t smk37_local_mod;
static void smk37_analog_playback(const uint16_t raw[6], unsigned valid)
{
    static uint8_t previous_bank;
    if (previous_bank!=smk37_fader_bank) {
        smk37_analog_ready &= 0x30u; /* bank change must not jump parameters */
        previous_bank=smk37_fader_bank;
    }
    unsigned targets = 1u << (song.sel % NTRK);
    for (unsigned k=0; k<FELUCCA_KEY_COUNT; ++k)
        if (kb_chn[k]) targets |= 1u << kb_trk[k];
    unsigned wheel_changed = targets != smk37_wheel_targets;
    for (unsigned i=0; i<6; ++i) {
        if (!(valid & (1u<<i))) continue;
        unsigned first = !(smk37_analog_ready & (1u<<i));
        unsigned prev = smk37_analog_anchor[i];
        unsigned delta = raw[i] > prev ? raw[i]-prev : prev-raw[i];
        if (i<4) {
            if (!first && (delta >= 8 || (raw[i] != prev && (raw[i]==0 || raw[i]>=1023))))
                { smk37_sound_take(smk37_fader_bank?TSEL:&trk[i],0x800+i);
                  smk37_input_at=fm1_ms;
                  if (smk37_fader_bank) TSEL->p[P_ATK+i]=smk37_fader_value(raw[i]);
                  else trk[i].p[P_LEVEL] = smk37_fader_value(raw[i]); }
            else if (!first) continue;
        } else if (i==4) {
            int16_t bend = smk37_pitch_value(raw[i]);
            if (!first && delta<4 && !(bend==0 && smk37_local_bend) &&
                !(bend==-8192 || bend==8191)) continue;
            wheel_changed |= first || bend != smk37_local_bend;
            smk37_local_bend = bend;
        } else {
            uint8_t mod = smk37_mod_value(raw[i]);
            if (!first && delta<3 && !(mod==0 || mod==127)) continue;
            wheel_changed |= first || mod != smk37_local_mod;
            smk37_local_mod = mod;
        }
        smk37_analog_anchor[i] = raw[i];
        smk37_analog_ready |= (uint8_t)(1u<<i);
    }
    if ((smk37_analog_ready & 0x30u)==0x30u && wheel_changed) {
        smk37_wheels(smk37_local_bend, smk37_local_mod, targets);
        smk37_wheel_targets = targets;
    }
}
