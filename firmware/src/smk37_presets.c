/* SPDX-License-Identifier: GPL-3.0-only
 * Record/protocol helpers from upreset.c, Copyright (C) 2026 Leo Kuroshita
 * (@kurogedelic), Hugelton Instruments. SMK allocation is independent of FM-1. */
#define UP_HOST 1
#include "upreset.c"
#undef UP_HOST
static void up_values(const up_rec_t *r, int16_t *v)   /* mapped and clamped for its engine */
{
    int16_t def[P_COUNT];
    uint32_t i;
    for (i = 0; i < P_COUNT; i++)
        def[i] = param_desc_of(r->engine, i)->def;
    up_params(r, v, def);
    if (r->ver == 1u && ENGINES[r->engine] == &ENG_PHYS)   /* (before 1.0: MODEL 2 was DUST) */
        phys_legacy(&v[P_E0]);
    for (i = 0; i < P_COUNT; i++)
        v[i] = (int16_t)param_fit(param_desc_of(r->engine, i), v[i]);   /* (a retired KIT: the kit it plays) */
}


#include "smk37_expanded_presets.h"
#include "smk37_object_store.h"
#define SMK37_UP_MAGIC (SMK37_EXPANDED_PRESETS?0x32505553u:0x31505553u)
#if SMK37_EXPANDED_PRESETS
static smk37_expanded_presets_t smk37_up __attribute__((section(".pool"))),smk37_up_work __attribute__((section(".pool")));
static smk37_legacy_presets_t smk37_up_legacy __attribute__((section(".pool"))),smk37_up_previous __attribute__((section(".pool")));
static uint8_t smk37_up_expanded;
#else
static smk37_legacy_presets_t smk37_up __attribute__((section(".pool"))),smk37_up_work __attribute__((section(".pool"))),smk37_up_previous __attribute__((section(".pool")));
#endif
static uint8_t smk37_up_ready,smk37_up_writable;
/* Reuse tested A/B commit machinery with a private virtual slot0 mapping.
 * Neither mapping can reach project sectors or cross either 4KB boundary. */
static uint32_t smk37_up_address(uint32_t a,uint32_t n){
    if(a<SMK37_STORE_LO || a>=SMK37_STORE_LO+8192u || !n)return 0;
    uint32_t d=a-SMK37_STORE_LO;if(n>4096u-(d&4095u))return 0;
    return (d<4096u?SMK37_PRESET_A:SMK37_PRESET_B)+(d&4095u);
}
static int smk37_up_read(uint32_t a,void *p,uint32_t n){a=smk37_up_address(a,n);return a?smk37_storage_read(a,p,n):-1;}
static int smk37_up_erase(uint32_t a){a=smk37_up_address(a,4096);return a&&smk37_up_writable?smk37_storage_erase(a):-1;}
static int smk37_up_program(uint32_t a,const void *p,uint32_t n){a=smk37_up_address(a,n);return a&&smk37_up_writable?smk37_storage_program(a,p,n):-1;}
static const smk37_store_io_t smk37_up_io={smk37_up_read,smk37_up_erase,smk37_up_program};
static void smk37_up_phrase_mirror(void){
    smk37_phrase_confirm=0; /* saving/renaming cannot leave a stale replacement confirmation */
    for(unsigned k=0;k<smk37_user_capacity;++k){
        const up_rec_t *r=&smk37_up.record[k];
        memset(&smk37_user_phrase[k],0,sizeof smk37_user_phrase[k]);
        if(!up_valid(r) || up_pat_empty(r))continue;
        int16_t values[P_COUNT];up_values(r,values);
        smk37_user_phrase[k].used=1;smk37_user_phrase[k].grid=up_grid(r);
        memcpy(smk37_user_phrase[k].note,r->note,16);memcpy(smk37_user_phrase[k].flags,r->flags,16);
        smk37_user_phrase[k].name[0]='U';smk37_user_phrase[k].name[1]='0'+(k+1)/10;smk37_user_phrase[k].name[2]='0'+(k+1)%10;smk37_user_phrase[k].name[3]=' ';
        memcpy(smk37_user_phrase[k].name+4,r->name,8);
        for(unsigned i=0;i<4;++i)smk37_user_phrase[k].timing[i]=values[P_SLEN+i];
        smk37_user_phrase[k].timing[0]=clamp(values[P_SLEN],1,16);
    }
    if(smk37_phrase>=smk37_phrase_count())smk37_phrase=0;
}
static void smk37_up_names(void){smk37_up_phrase_mirror();smk37_user_used=0;for(unsigned k=0;k<smk37_user_capacity;++k){
    memset(smk37_user_names[k],0,13);if(up_valid(&smk37_up.record[k])){smk37_user_used|=1u<<k;memcpy(smk37_user_names[k],smk37_up.record[k].name,12);}}}
static void smk37_prefs_restore(void){
    favorites=smk37_up.favorites;favorites.filter=!!favorites.filter;if(smk37_user_capacity==8)favorites.user&=255u;
    smk37_global_prefs=smk37_up.prefs[31]==0x18;
    if(smk37_global_prefs)for(unsigned i=0;i<(smk37_up.prefs[30]==0x23?SMK37_PREF_COUNT:smk37_up.prefs[30]==0x21?22u:smk37_up.prefs[30]==0x20?21u:smk37_up.prefs[30]==0x19?20u:11u);++i)smk37_pref_set(i,smk37_up.prefs[i]);
}
static void smk37_prefs_capture(void){
    smk37_up_work.favorites=favorites;
    for(unsigned i=0;i<SMK37_PREF_COUNT;++i)smk37_up_work.prefs[i]=smk37_pref_get(i);
    smk37_up_work.prefs[30]=0x23;smk37_up_work.prefs[31]=0x18;
}
static int smk37_up_boot(void){
    smk37_up_ready=smk37_up_writable=0;smk37_user_capacity=8;
#if SMK37_EXPANDED_PRESETS
    smk37_up_expanded=0;
    if(smk37_object_load(&smk37_storage_io,&smk37_object_presets,&smk37_up,sizeof smk37_up)>=0){
        if(smk37_up.magic!=SMK37_UP_MAGIC)return -1;
        smk37_up_expanded=1;
    }else{
        int copy=smk37_store_load(&smk37_up_io,0,&smk37_up_legacy,sizeof smk37_up_legacy,0);
        if(copy>=0){if(smk37_up_legacy.magic!=0x31505553u)return -1;}
        else{
            for(uint32_t a=SMK37_STORE_LO;a<SMK37_STORE_LO+8192u;a+=256){
                if(smk37_up_read(a,smk37_store_scratch,256))return -1;
                for(unsigned i=0;i<256;++i)if(smk37_store_scratch[i]!=255)return -1;
            }
            memset(&smk37_up_legacy,0,sizeof smk37_up_legacy);smk37_up_legacy.magic=0x31505553u;
        }
        if(smk37_expand_presets(&smk37_up,&smk37_up_legacy,sizeof smk37_up_legacy))return -1;
        smk37_up_expanded=smk37_object_owned(&smk37_storage_io,&smk37_object_presets,sizeof smk37_up);
    }
    if(smk37_up_expanded)smk37_user_capacity=32;
#else
    int copy=smk37_store_load(&smk37_up_io,0,&smk37_up,sizeof smk37_up,0);
    if(copy>=0){if(smk37_up.magic!=SMK37_UP_MAGIC)return -1;}
    else {
        for(uint32_t a=SMK37_STORE_LO;a<SMK37_STORE_LO+8192u;a+=256){
            if(smk37_up_read(a,smk37_store_scratch,256))return -1;
            for(unsigned i=0;i<256;++i)if(smk37_store_scratch[i]!=255)return -1;
        }
        memset(&smk37_up,0,sizeof smk37_up);smk37_up.magic=SMK37_UP_MAGIC;
    }
#endif
    for(unsigned i=0;i<SMK37_UP_SLOTS;++i)up_migrate(&smk37_up.record[i]);
    smk37_up_names();smk37_prefs_restore();smk37_up_ready=1;smk37_up_writable=smk37_storage_writable;return 0;
}
static int smk37_up_used(unsigned k){return smk37_up_ready && k<smk37_user_capacity && up_valid(&smk37_up.record[k]);}
static int smk37_up_idle(void){
    if(song.playing || song.rec || transport_req || smk37_pedal_down || smk37_song_loading)return 0;
    for(unsigned k=0;k<FELUCCA_KEY_COUNT;++k)if(kb_chn[k])return 0;
    for(unsigned k=0;k<NTRK;++k)if(midi_owners[k])return 0;
    return 1;
}
/* RAM mirror changes only after a fully verified commit. Main loop only. */
static int smk37_up_commit(void){
    if(!smk37_up_writable || !smk37_up_idle())return 2;
    fm1_irq_off();smk37_prefs_capture();fm1_irq_on();
    #if SMK37_EXPANDED_PRESETS
    if(smk37_up_expanded){
        if(!smk37_object_owned(&smk37_storage_io,&smk37_object_presets,sizeof smk37_up_work) ||
           smk37_object_save(&smk37_storage_io,&smk37_object_presets,&smk37_up_work,sizeof smk37_up_work,1))return 2;
    }else{
        smk37_up_legacy.magic=0x31505553u;
        memcpy(smk37_up_legacy.record,smk37_up_work.record,sizeof smk37_up_legacy.record);
        memcpy(smk37_up_legacy.fm6,smk37_up_work.fm6,sizeof smk37_up_legacy.fm6);
        smk37_up_legacy.favorites=smk37_up_work.favorites;
        memcpy(smk37_up_legacy.prefs,smk37_up_work.prefs,32);
        if(smk37_store_save(&smk37_up_io,0,&smk37_up_legacy,sizeof smk37_up_legacy,&smk37_up_previous))return 2;
    }
#else
    if(smk37_store_save(&smk37_up_io,0,&smk37_up_work,sizeof smk37_up_work,&smk37_up_previous))return 2;
#endif
    fm1_irq_off();smk37_up=smk37_up_work;smk37_global_prefs=1;smk37_up_names();fm1_irq_on();return 0;
}
static int smk37_up_put(unsigned k,const up_rec_t *r){
    if(k>=smk37_user_capacity || (r&&!up_valid(r)))return 1;
    smk37_up_work=smk37_up;
    if(r)smk37_up_work.record[k]=*r;else memset(&smk37_up_work.record[k],0,sizeof(up_rec_t));
    /* Importing/renaming the same FM6 slot keeps its OWN patch; a new slot gets factory 0. */
    if(!r || r->engine!=ENGI_FM6 || !smk37_up_used(k) || smk37_up.record[k].engine!=ENGI_FM6)
        memcpy(smk37_up_work.fm6[k],FM6_FACTORY[0],FM6_PACKED);
    return smk37_up_commit();
}
static int smk37_up_store(unsigned k,const char *name){
    if(k>=smk37_user_capacity)return 1;
    fm1_irq_off();
    if(!smk37_up_idle()){fm1_irq_on();return 2;}
    smk37_up_work=smk37_up;up_rec_t *r=&smk37_up_work.record[k];memset(r,0,sizeof *r);
    r->used=UP_USED;r->ver=UP_VER;r->engine=TSEL->eng_req;r->np=P_COUNT;
    if(name&&*name)memcpy(r->name,name,str_len(name)>12?12:str_len(name));
    else {const char *src=ENGINES[TSEL->eng_req]->presets[TSEL->preset%ENGINES[TSEL->eng_req]->npresets].name;memcpy(r->name,src,str_len(src)>12?12:str_len(src));}
    for(unsigned i=0;i<P_COUNT;++i)up_set_value(r,i,motion_base_value(TSEL,i));
    up_pat_from(r,TSEL->step);
    if(drum_track(TSEL)){unsigned hits=0;for(unsigned i=0;i<16;++i)hits|=step_lanes(&TSEL->step[i]);
        if(hits){r->ver=UP_VER_GRID;for(unsigned i=0;i<16;++i){r->note[i]=step_lanes(&TSEL->step[i]);r->flags[i]=step_accents(&TSEL->step[i]);}}}
    fm6_pack(fm6_patch[song.sel],smk37_up_work.fm6[k]);fm1_irq_on();return smk37_up_commit();
}
static int smk37_up_load(unsigned k){
    if(!smk37_up_used(k))return 1;
    int16_t v[P_COUNT];up_values(&smk37_up.record[k],v);unsigned e=smk37_up.record[k].engine;uint8_t fp[FP_SIZE+1];
    if(e==ENGI_DIGITAL){fm4_convert(v,fp);e=ENGI_FM6;}else fm6_unpack(smk37_up.fm6[k],fp);
    fm1_irq_off();track_t *t=TSEL;smk37_sound_take(t,0xffff);unsigned tr=song.sel;panic_req|=1u<<tr;
    motion_restore(t);
    for(unsigned i=0;i<P_COUNT;++i)if(!(i==P_LEVEL || i==P_PAN || i==P_MUTE || (i>=P_AMODE&&i<=P_SGATE) || (i>=P_SLCR&&i<=P_SLDEPTH) || i==P_CHRD || i==P_VOIC || i==P_SQNT))t->p[i]=v[i];
    t->eng_req=e;t->preset=0;t->user=k+1;
    if(e==ENGI_FM6){fm6_set_patch(tr,fp);fm6_adopt(tr);}fm1_irq_on();return 0;
}

static int smk37_preferences_save(void){smk37_up_work=smk37_up;fm1_irq_off();smk37_prefs_capture();fm1_irq_on();if(!memcmp(&smk37_up_work,&smk37_up,sizeof smk37_up))return 0;return smk37_up_commit();}
static int smk37_preset_rename(void){
    unsigned k=smk37_name_slot;if(!smk37_up_used(k))return 1;
    up_rec_t r=smk37_up.record[k];memcpy(r.name,smk37_name,12);return smk37_up_put(k,&r);
}
