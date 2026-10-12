/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#define SMK37_SYNTH_HOST_TEST 1
#include "../firmware/src/smk37_synth.c"
static void midi_out_event(uint32_t p){(void)p;}
static unsigned locked;
static void fm1_irq_off(void){locked=1;}
static void fm1_irq_on(void){locked=0;}
#include "../firmware/src/smk37_session.c"
#include "../firmware/src/smk37_editor.c"
static uint8_t flash[0x100000],saved[0x100000];
static int allowance=-1,header_cut=-1;
static int operation(void){if(allowance<0)return 0;if(!allowance)return -1;--allowance;return 0;}
static int smk37_storage_read(uint32_t a,void *p,uint32_t n){assert(!locked);assert(smk37_store_range(a,n)||SMK37_PRESET_RANGE(a,n)||(SMK37_EXPANDED_PRESETS&&SMK37_EXPANDED_RANGE(a,n)));memcpy(p,flash+a,n);return 0;}
static int smk37_storage_erase(uint32_t a){assert(!locked && !(a&4095));assert(smk37_store_range(a,4096)||SMK37_PRESET_RANGE(a,4096)||(SMK37_EXPANDED_PRESETS&&SMK37_EXPANDED_RANGE(a,4096)));if(operation())return -1;memset(flash+a,255,4096);return 0;}
static int smk37_storage_program(uint32_t a,const void *p,uint32_t n){assert(!locked && n<=256 && (a&255)+n<=256);assert(smk37_store_range(a,n)||SMK37_PRESET_RANGE(a,n)||(SMK37_EXPANDED_PRESETS&&SMK37_EXPANDED_RANGE(a,n)));if(operation())return -1;
    int cut=!(a&4095)&&header_cut>=0;unsigned bytes=cut?(unsigned)header_cut:n;
    for(unsigned i=0;i<bytes;++i)flash[a+i]&=((const uint8_t *)p)[i];return cut?-1:0;}
static void protected(void){for(unsigned i=0;i<sizeof flash;++i)if(!SMK37_PRESET_RANGE(i,1))assert(flash[i]==saved[i]);}
static unsigned req(unsigned cmd,const uint8_t *a,unsigned n){uint8_t f[640]={125,70,76};f[3]=cmd;memcpy(f+4,a,n);return smk37_editor_main_command(f,n+4);}
static void emit(unsigned cmd,const uint8_t *a,unsigned n){unsigned size=req(cmd,a,n);assert(size);putchar('[');for(unsigned i=0;i<size;++i)printf("%s%u",i?",":"",smk37_ed_out[i]);puts("]");}
int main(int argc,char **argv){
    memset(flash,255,sizeof flash);smk37_synth_init();smk37_storage_ready=smk37_storage_writable=1;
    assert(!smk37_up_boot() && smk37_up_ready && smk37_up_writable);
    assert(!smk37_up_used(0));
    assert(!SMK37_PRESET_RANGE(0xf4000,1)&&!SMK37_PRESET_RANGE(0xfefff,2)&&!SMK37_PRESET_RANGE(0xff000,1)&&!SMK37_PRESET_RANGE(0xf3000,UINT32_MAX));
    assert(!smk37_up_address(SMK37_STORE_LO+4095,2) && !smk37_up_address(SMK37_STORE_LO+8192,1));
    /* Every engine roundtrips; FM6 patch is in the same atomic commit. */
    for(unsigned e=0;e<NENGINES;++e){if(e==ENGI_DIGITAL)continue;TSEL->eng_req=e;smk37_load_preset(TSEL,0);TSEL->p[P_ATK]=17;
        assert(!smk37_up_store(0,"MY SOUND"));TSEL->p[P_ATK]=0;assert(!smk37_up_boot());assert(!smk37_up_load(0));assert(TSEL->p[P_ATK]==17 && TSEL->eng_req==e);}
    TSEL->eng_req=ENGI_FM6;smk37_load_preset(TSEL,0);fm6_patch[0][0]=71;TSEL->p[P_ATK]=23;
    assert(!smk37_up_store(7,"FM6 OWN"));fm6_patch[0][0]=9;assert(!smk37_up_load(7));assert(fm6_patch[0][0]==71);
    assert(smk37_up_store(8,"BAD"));
    /* A/B survives every failed erase/program and partial header. Flash IO must never run under IRQ lock. */
    memcpy(saved,flash,sizeof flash);
    for(int cut=0;cut<15;++cut){memcpy(flash,saved,sizeof flash);assert(!smk37_up_boot());TSEL->p[P_ATK]=45;allowance=cut;
        int rc=smk37_up_store(7,"NEW SOUND");allowance=-1;assert(!smk37_up_boot());assert(!smk37_up_load(7));assert(TSEL->p[P_ATK]==(rc?23:45));protected();}
    for(header_cut=0;header_cut<32;++header_cut){memcpy(flash,saved,sizeof flash);assert(!smk37_up_boot());assert(smk37_up_store(7,"PARTIAL"));assert(!smk37_up_boot());assert(!smk37_up_load(7));assert(TSEL->p[P_ATK]==23);protected();}
    header_cut=-1;
    /* An interrupted first save does not authorize erasing unknown flash on boot. */
    memset(flash+SMK37_PRESET_A,255,4096);memset(flash+SMK37_PRESET_B,255,4096);assert(!smk37_up_boot());allowance=2;
    assert(smk37_up_store(0,"FIRST"));allowance=-1;assert(smk37_up_boot() && !smk37_up_writable);
    memcpy(flash,saved,sizeof flash);assert(!smk37_up_boot());
    smk37_play_key_event(1,80);assert(smk37_up_store(0,"HELD")==2);smk37_play_key_event(1,0);
    song.playing=1;assert(smk37_up_store(0,"PLAY")==2);song.playing=0;
    /* Sequence tools move locks with notes and undo/redo leaves other tracks intact. */
    track_t *t=TSEL;t->p[P_SLEN]=16;memset(t->step,0,sizeof t->step);for(unsigned i=0;i<NSTEP;++i)t->step[i].time=ST_REST;
    t->step[0].time=ST_NOTE;t->step[0].n=1;t->step[0].note[0]=60;t->step[0].vel=90;
    motion_clear(t);assert(!motion_set_lock(t,0,P_ATK,20));assert(!motion_set_event(&trk[1],2,P_LEVEL,42));
    smk37_tool=TL_RIGHT;smk37_tool_action();assert(t->step[1].note[0]==60 && motion_find(t,1,P_ATK)>=0);
    smk37_pattern_undo();assert(t->step[0].note[0]==60 && motion_find(t,0,P_ATK)>=0 && motion_find(&trk[1],2,P_LEVEL)>=0);
    smk37_pattern_undo();assert(t->step[1].note[0]==60);
    /* Save two patterns; lane selection uses their notes while keeping current sound and editable steps. */
    t->step[0].time=ST_NOTE;t->step[0].n=1;t->step[0].note[0]=62;assert(!smk37_session_save(0));
    t->step[0].note[0]=67;trk[1].step[0]=t->step[0];assert(!smk37_session_save(1));
    t->step[0].note[0]=75;chain_defaults(&chain_config);chain_config.count=1;chain_config.row[0]=chain_row_of(0,2);chain_config.row[0].slot[1]=1;chain_config.row[0].slot[2]=CHAIN_SILENT;
    assert(!smk37_chain_prepare());assert(chain.armed && transport_req==1);assert(t->step[0].note[0]==75);
    assert(chain.source[0].step[0][0].note[0]==62 && chain.source[1].step[1][0].note[0]==67);
    int len=t->p[P_SLEN];transport_req=0;seq_start();assert(chain.running && chain.lane[2]==CHAIN_SILENT);
    assert(seq_steps(t)[0].note[0]==62 && seq_steps(&trk[1])[0].note[0]==67);seq_stop();assert(!chain.running && t->p[P_SLEN]==len);
    chain_config.row[0].slot[0]=3;assert(smk37_chain_prepare()==6 && !chain.armed);chain_config.row[0].slot[0]=0;
    uint8_t lanes[]={5,1,0,1,4,0,3};assert(req(33,lanes,sizeof lanes));assert(!smk37_ed_out[6] && chain_config.row[0].slot[2]==4);
    lanes[4]=5;assert(req(33,lanes,sizeof lanes) && smk37_ed_out[6]==1 && chain_config.row[0].slot[2]==4);
    uint8_t store[]={3,'T','E','S','T',0};assert(req(19,store,sizeof store));assert(!smk37_ed_out[6] && smk37_up_used(3));
    /* Browser import exercises UP_PUT against real packed records, with slot bounds. */
    uint8_t put[320]={2,0,'C','O','P','Y',0};unsigned pn=7;
    for(unsigned i=0;i<P_COUNT;++i){int v=param_desc_of(0,i)->def+8192;put[pn++]=v&127;put[pn++]=v>>7;}
    memset(put+pn,0,32);pn+=32;assert(req(18,put,pn) && !smk37_ed_out[6] && smk37_up_used(2));
    put[0]=8;assert(req(18,put,pn) && smk37_ed_out[6]==1);
    uint8_t erase[]={2};assert(req(21,erase,1) && !smk37_ed_out[6] && !smk37_up_used(2));
    if(argc>1){
        uint8_t info[]={125,70,76,1};unsigned size=smk37_editor_handle(info,4);putchar('[');for(unsigned i=0;i<size;++i)printf("%s%u",i?",":"",smk37_ed_out[i]);puts("]");
        uint8_t a[]={0,8};emit(16,a,2);a[0]=3;emit(17,a,1);a[0]=4;emit(33,a,1);a[0]=3;a[1]=7;emit(68,a,2);return 0;}
    puts("SMK completion: 8 presets/FM6, interrupted commits, protected sectors, tool undo/redo, song lanes and main-loop IO passed");
}
