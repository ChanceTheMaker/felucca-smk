/* SPDX-License-Identifier: GPL-3.0-only */
/* Reuse the versioned FUN10 (and import Test10-12 FUN8) wire format, without FM-1 runtime storage/UI. */
#define PROJ_HOST 1
#include "project.c"
#undef PROJ_HOST
#include "smk37_store.h"
#include "smk37_slice_edit.c"
typedef struct {
    project_store_t project;
    uint32_t generation;
    uint8_t version,octave,velocity,pad_bank,knob_bank,fader_bank,selected_slot,reserved[53];
} smk37_session_t;
_Static_assert(sizeof(smk37_session_t)==3904,"SMK session wire size");
static smk37_session_t smk37_session __attribute__((section(".pool"))),smk37_session_previous __attribute__((section(".pool")));
static project_t smk37_project_work __attribute__((section(".pool")));
static uint8_t smk37_storage_ready,smk37_storage_writable;
static uint32_t smk37_save_generation;
static int smk37_storage_read(uint32_t,void *,uint32_t);
static int smk37_storage_erase(uint32_t);
static int smk37_storage_program(uint32_t,const void *,uint32_t);
static const smk37_store_io_t smk37_storage_io={smk37_storage_read,smk37_storage_erase,smk37_storage_program};
static void smk37_session_capture(void) {
    project_t *p=&smk37_project_work;
    memset(p,0,sizeof *p);p->magic=PROJ_MAGIC;p->size=sizeof *p;
    p->parts=NPART;p->phys=PROJ_PHYS;p->sel=song.sel;p->chain=chain_config;p->motion=motion;
    memcpy(p->g,song.g,sizeof p->g);
    memcpy(p->mch,song.mch,sizeof p->mch);
    for(unsigned k=0;k<NTRK;++k){
        memcpy(p->t[k].p,trk[k].p,sizeof trk[k].p);
        memcpy(p->t[k].step,trk[k].step,sizeof trk[k].step);
        p->t[k].engine=trk[k].eng_req;p->t[k].preset=trk[k].preset;
        fm6_pack(fm6_patch[k],p->fm6[k]);
    }
    p->sum=proj_sum(p);
    memset(&smk37_session,0,sizeof smk37_session);
    proj_pack(&smk37_session.project,p);
    smk37_session.version=2;smk37_session.octave=(uint8_t)(song.octave+3);
    smk37_session.generation=smk37_save_generation+1u;
    smk37_session.velocity=smk37_velocity_mode;smk37_session.pad_bank=smk37_pad_bank;
    smk37_session.knob_bank=smk37_knob_bank;smk37_session.fader_bank=smk37_fader_bank;
    smk37_session.selected_slot=smk37_project_slot;
    memcpy(smk37_session.reserved,smk37_learn_map,32);
    smk37_session.reserved[32]=click_mode;smk37_session.reserved[33]=click_lvl;smk37_session.reserved[34]=cin_bars;
    smk37_session.reserved[35]=smk37_palette;smk37_session.reserved[36]=smk37_bold;
    smk37_session.reserved[37]=(song.master_q12+16u)/32u;smk37_session.reserved[38]=0x56;
}
static int smk37_session_valid(void) {
    return (smk37_session.version==1 || smk37_session.version==2) && smk37_session.octave<=6 && smk37_session.velocity<=3 &&
        smk37_session.pad_bank<=1 && smk37_session.knob_bank<=1 && smk37_session.fader_bank<=1 &&
        smk37_session.selected_slot<4 && proj_import(&smk37_project_work,
            &smk37_session.project,smk37_session.version==1 ? 3584u : sizeof smk37_session.project);
}
/* Audio serialization is supplied by caller. Stops playback and releases all
 * local ownership before restoring state; loading never starts a sequence. */
static int smk37_session_restore(void) {
    if(!smk37_session_valid())return -1;
    project_t *p=&smk37_project_work;
    seq_stop();song.rec=0;transport_req=0;smk37_sustain(0);
    for(unsigned k=0;k<FELUCCA_KEY_COUNT;++k)if(kb_chn[k])key_off(k,&trk[kb_trk[k]]);
    panic_req=(1u<<NTRK)-1u;
    for(unsigned i=0;i<G_COUNT;++i) song.g[i]=clamp(p->g[i],GP[i].min,GP[i].max);
    for(unsigned k=0;k<NTRK;++k){
        track_t *t=&trk[k];unsigned e=p->t[k].engine%NENGINES;
        if(e==ENGI_DIGITAL)e=ENGI_FM6;
        t->eng_req=e;t->preset=p->t[k].preset;t->user=0;
        for(unsigned i=0;i<P_COUNT;++i){const param_desc_t *d=param_desc_of(e,i);t->p[i]=clamp(p->t[k].p[i],d->min,d->max);}
        memcpy(t->step,p->t[k].step,sizeof t->step);
        uint8_t values[FP_SIZE+1];fm6_unpack(p->fm6[k],values);fm6_set_patch(k,values);fm6_adopt(k);
    }
    memcpy(song.mch,p->mch,sizeof song.mch);midi_route=MIDI_ROUTE_AGAIN;
    smk37_song_row=0;smk37_undo.valid=0;smk37_sound_undo.valid=0;
    chain_config=p->chain;motion=p->motion;memset(motion_active,0,sizeof motion_active);motion_base_valid=0;
    song.sel=p->sel%NTRK;song.octave=smk37_session.octave-3;
    smk37_velocity_mode=smk37_session.velocity;smk37_pad_bank=smk37_session.pad_bank;
    smk37_knob_bank=smk37_session.knob_bank;smk37_fader_bank=smk37_session.fader_bank;
    if(smk37_session.version==2){
        memcpy(smk37_learn_map,smk37_session.reserved,32);
        click_mode=smk37_session.reserved[32]%3;click_lvl=smk37_session.reserved[33]%3;cin_bars=smk37_session.reserved[34]%3;
        if(!smk37_global_prefs){smk37_palette=smk37_session.reserved[35]%23;smk37_bold=!!smk37_session.reserved[36];}
    }else{memset(smk37_learn_map,0,32);click_mode=cin_bars=0;click_lvl=1;if(!smk37_global_prefs)smk37_palette=smk37_bold=0;}
    song.master_q12=smk37_session.version==2 && smk37_session.reserved[38]==0x56 && smk37_session.reserved[37]<=128 ? smk37_session.reserved[37]*32u : 1024u;
    ml_arm=ml_heard=0;smk37_learn_on=0;
    smk37_analog_ready=0;memset(smk37_encoder_phase,0,sizeof smk37_encoder_phase);
    if(smk37_session.generation-smk37_save_generation<0x80000000u)
        smk37_save_generation=smk37_session.generation;
    return 0;
}
static int smk37_session_load(unsigned slot) {
    if(!smk37_storage_ready || slot>=4)return -1;
    if(smk37_store_load(&smk37_storage_io,slot,&smk37_session,sizeof smk37_session,0)<0)return -1;
    fm1_irq_off();int rc=smk37_session_restore();if(!rc)smk37_project_slot=(uint8_t)slot;fm1_irq_on();return rc;
}
static int smk37_session_save(unsigned slot) {
    if(!smk37_storage_writable || slot>=4 || song.playing || song.rec || transport_req)return -1;
    fm1_irq_off();
    for(unsigned k=0;k<FELUCCA_KEY_COUNT;++k)if(kb_chn[k]){fm1_irq_on();return -1;}
    for(unsigned k=0;k<NTRK;++k)if(midi_owners[k]){fm1_irq_on();return -1;}
    if(smk37_pedal_down || song.playing || song.rec || transport_req){fm1_irq_on();return -1;}
    smk37_session_capture();fm1_irq_on();
    smk37_session.selected_slot=(uint8_t)slot;
    int rc=smk37_store_save(&smk37_storage_io,slot,&smk37_session,sizeof smk37_session,&smk37_session_previous);
    if(!rc){smk37_save_generation=smk37_session.generation;smk37_project_slot=(uint8_t)slot;}
    return rc;
}
static int smk37_session_used(unsigned slot){
    if(!smk37_storage_ready || slot>=4)return 0;
    return smk37_store_load(&smk37_storage_io,slot,&smk37_session,sizeof smk37_session,0)>=0 && smk37_session_valid();
}
static int smk37_session_resume(void){
    unsigned newest=4;uint32_t generation=0;
    for(unsigned slot=0;slot<4;++slot)if(smk37_session_used(slot)){
        uint32_t g=smk37_session.generation;
        if(newest==4 || (g!=generation && g-generation<0x80000000u)){newest=slot;generation=g;}
    }
    if(newest==4)return -1;
    smk37_save_generation=generation;smk37_project_slot=(uint8_t)newest;
    return smk37_session_load(newest);
}

/* Read-only preparation; publish only a fully loaded chain, never from ISR. */
static unsigned smk37_chain_prepare(void){
    unsigned used=0,rc=0;uint8_t engines[NTRK];
    fm1_irq_off();
    if(song.playing || song.rec || transport_req || chain.armed){fm1_irq_on();return 2;}
    if(!smk37_storage_ready || !chain_config.count || !chain_valid(&chain_config)){fm1_irq_on();return 1;}
    smk37_song_loading=1;chain.config=chain_config;for(unsigned k=0;k<NTRK;++k)engines[k]=trk[k].eng_req;fm1_irq_on();
    for(unsigned r=0;r<chain.config.count;++r)for(unsigned k=0;k<NTRK;++k){unsigned slot=chain.config.row[r].slot[k];if(slot<4)used|=1u<<slot;}
    for(unsigned i=0;i<4;++i)if(used&(1u<<i)){
        if(!smk37_session_used(i)){rc=3+i;break;}
        project_t *p=&smk37_project_work;chain_pattern_t *q=&chain.source[i];q->motion=p->motion;
        unsigned n=0;
        for(unsigned e=0;e<q->motion.count;++e){motion_event_t *v=&q->motion.event[e];unsigned owner=v->place>>6;
            if(MOTION_ID(v)>=P_FM1_ATK && p->t[owner].engine!=engines[owner])continue;q->motion.event[n++]=*v;}
        q->motion.count=n;
        for(unsigned k=0;k<NTRK;++k){memcpy(q->step[k],p->t[k].step,sizeof q->step[k]);for(unsigned j=0;j<NSTEP;++j){step_t *st=&q->step[k][j];if(st->n>4)st->n=4;if(st->time>ST_REST)st->time=ST_REST;for(unsigned z=0;z<4;++z)st->note[z]&=127;st->acc&=st->hit;}q->qnt[k]=!!p->t[k].p[P_SQNT];
            for(unsigned j=0;j<4;++j)q->timing[k][j]=clamp(p->t[k].p[P_SLEN+j],TP[P_SLEN+j].min,TP[P_SLEN+j].max);}
    }
    fm1_irq_off();
    for(unsigned k=0;k<NTRK;++k)if(engines[k]!=trk[k].eng_req)rc=2;
    if(!rc && (song.playing || song.rec || transport_req))rc=2;
    if(!rc){RING_PUBLISH();chain.armed=1;transport_req=1;}
    smk37_song_loading=0;fm1_irq_on();return rc;
}

#include "smk37_presets.c"
#if SMK37_ENABLE_SAMPLES
#include "smk37_samples.c"
#endif
