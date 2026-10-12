/* SPDX-License-Identifier: GPL-3.0-only
 * FM6 envelope graph from upstream ui_graph.c, Copyright (C) 2026
 * Leo Kuroshita (@kurogedelic), Hugelton Instruments. */
static void graph_fm6env(const track_t *t, uint16_t c)
{
    static const char *const STG[4] = {"ATK", "DEC", "SUS", "REL"};
    const uint8_t *op = fm6_patch[t-trk]+(5u-smk37_op)*FP_OP;
    int32_t top = 24, bot = 88, x[6], y[6], k;
    uint32_t sg = smk37_op_stage & 3u;
    char b[24] = {'O', 'P', (char)('1' + smk37_op % 6u), 0};
#define FEGY(l) (bot - (int32_t)(l) * (bot - top) / 99)
#define FEGW(r) (4 + (99 - (int32_t)(r)) * (99 - (int32_t)(r)) * 44 / 9801)
    x[0] = 12;
    y[0] = FEGY(op[FP_L1 + 3]);
    for (k = 0; k < 3; k++) {
        x[k + 1] = x[k] + FEGW(op[FP_R1 + k]);
        y[k + 1] = FEGY(op[FP_L1 + k]);
    }
    x[5] = 228;
    x[4] = x[5] - FEGW(op[FP_R1 + 3]);
    y[4] = y[3];
    y[5] = y[0];
    cv_rect(PANEL_X0, bot + 2, PANEL_W, 1, T_RAISE);
    cv_rect(x[4], top, 1, bot - top + 1, T_RAISE);   /* the key let go */
    for (k = 0; k < 5; k++)                          /* (3: the hold, no stage of its own) */
        cv_line_t(x[k], y[k], x[k + 1], y[k + 1], k != 3 && (uint32_t)(k < 3 ? k : 3) == sg ? T_ACCENT : c, 2);
    k = sg < 3u ? (int32_t)sg + 1 : 5;
    fm_dot(x[k], y[k], T_ACCENT);
#undef FEGY
#undef FEGW
    cv_text(10, 4 - GOY, &AF_S, b, T_MID);
    str_cpy(b, STG[sg], sizeof b);
    str_cpy(b + str_len(b), "  R ", 5);
    fmt_int(b + str_len(b), op[FP_R1 + sg]);
    str_cpy(b + str_len(b), "  L ", 5);
    fmt_int(b + str_len(b), op[FP_L1 + sg]);
    cv_text_r(228, 4 - GOY, &AF_S, b, T_TEXT, T_SURF);
}
static void smk37_list_row(unsigned row,const char *s,int selected){
    int y=5+26*(int)row;uint16_t bg=selected?T_TINT:T_SURF;
    cv_rrect(7,y,226,24,3,bg,T_SURF);if(selected)cv_rect(7,y+4,2,16,T_THEME);cv_text_fit(12,y+1,&AF_S,s,selected?T_THEME:T_TEXT,bg,216);
}
/* Incremental source envelope: bounded decoding, never masks audio IRQs. */
static void smk37_slice_wave(const slc_src_t *src){
    static const slc_src_t *old;static uint32_t generation,length;
    static slc_dec_t decoder;static int16_t lo[216],hi[216];
    if(old!=src || generation!=smp_user_gen || length!=src->len){old=src;generation=smp_user_gen;length=src->len;slc_dec_at(&decoder,0,0);for(unsigned i=0;i<216;++i){lo[i]=32767;hi[i]=-32768;}}
    for(unsigned budget=4096;budget && decoder.pos<src->len;--budget){unsigned col=decoder.pos*216u/src->len;int x=slc_dec_next(src,&decoder);if(x<lo[col])lo[col]=x;if(x>hi[col])hi[col]=x;}
    for(unsigned i=0;i<216;++i)if(hi[i]>=lo[i]){int a=60-hi[i]*28/32768,b=60-lo[i]*28/32768;cv_rect(12+i,a,1,b-a+1,T_MID);}
}
static void smk37_workflow_panel(unsigned page){
    char text[40];
    if(page==SMK37_STORAGE_PAGE){
        if(!SMK37_ENABLE_SAMPLES)panel_note("SAMPLE STORAGE DISABLED","THIS BUILD HAS NO UPLOADS",0);
        else if(smk37_samples_owned)panel_note("3 USER SLOTS / 80 KIB EACH","UPLOAD IN STUDIO > SAMPLES",0);
        else if(smk37_samples_state==3)panel_note("STORAGE CHECK FAILED","EXISTING DATA PRESERVED",0);
        else if(smk37_storage_confirm&&fm1_ms-smk37_storage_confirm_at<=5000u)panel_note("REPLACES STOCK SAMPLE DATA","> AGAIN WITHIN 5S TO CONFIRM","< CANCEL / BACKUP REQUIRED");
        else panel_note("INITIALIZE USER SAMPLES","REQUIRES PRIVATE STOCK BACKUP","> REVIEW / < BACK");
    }else if(page==SMK37_NAV_PAGE){
        unsigned base=(smk37_nav_row/4)*4;
        for(unsigned i=0;i<4 && base+i<smk37_nav_count();++i){unsigned p=smk37_group_pages[smk37_nav_group][base+i];
            const char *s=p<SMK37_SOUND_PAGES?smk37_sound_pages[p].title:(const char *const[]){"ANALOG CHECK","BUTTON CHECK","LIGHT CHECK","PEDAL CHECK"}[p-SMK37_SOUND_PAGES];
            str_cpy(text,s,sizeof text);if(!smk37_page_ok(p))str_cpy(text+str_len(text)," (ENGINE)",sizeof text-str_len(text));smk37_list_row(i,text,base+i==smk37_nav_row);}
    }else if(page==SMK37_PREF_PAGE){
        unsigned base=(smk37_pref_row/4)*4;
        for(unsigned i=0;i<4 && base+i<SMK37_PREF_COUNT;++i){char v[24];smk37_pref_format(base+i,v);str_cpy(text,smk37_pref_names[base+i],sizeof text);str_cpy(text+str_len(text),": ",sizeof text-str_len(text));str_cpy(text+str_len(text),v,sizeof text-str_len(text));smk37_list_row(i,text,base+i==smk37_pref_row);}
    }else if(page==SMK37_BROWSER_PAGE){
        unsigned ordinal=0,total=smk37_browser_count(),n=0;
        if(!total){panel_note("NO MATCHING PRESETS","K5 ENGINE / K7 ALL",0);return;}
        for(unsigned p=0;p<smk37_browser_index;++p)ordinal+=smk37_browser_valid(smk37_browser_engine,p);
        unsigned base=ordinal/4*4;
        for(unsigned p=0;p<256;++p)if(smk37_browser_valid(smk37_browser_engine,p)){
            if(n>=base && n<base+4){text[0]=favorite_has(smk37_browser_engine,p)?'*':' ';text[1]=' ';smk37_browser_name(p,text+2);smk37_list_row(n-base,text,p==smk37_browser_index);}++n;}
    }else if(page>=SMK37_OP_PAGE && page<=SMK37_OP2_PAGE){
        if(TSEL->eng_req!=ENGI_FM6){panel_note("SELECT FM6 ENGINE",0,0);return;}
        cv_oy=GOY;if(page==SMK37_OPENV_PAGE)graph_fm6env(TSEL,T_THEME);else graph_fm6(TSEL,T_THEME,smk37_op,0);cv_oy=0;
    }else if(page==SMK37_SLICE_PAGE){
        const slc_src_t *s=smk37_slice_source();unsigned n=smk37_slice_count();
        if(!n){panel_note("SAMPLE NOT FOUND",smk37_samples_owned?"UPLOAD IN STUDIO > SAMPLES":"INITIALIZE SAMPLE STORAGE",0);return;}
        unsigned sel=smk37_slice_sel<=n?smk37_slice_sel:n;
        smk37_slice_wave(s);
        cv_rect(12,60,216,1,T_RAISE);
        for(unsigned i=0;i<=n;++i){unsigned x=12+smk37_slice_mark(i)*215u/s->len;cv_rect(x,30,1,60,i==sel?T_ACCENT:T_THEME);}
        cv_text_in(10,4,220,&AF_S,"SLICE STARTS / END",T_TEXT,T_SURF);
        cv_text_in(10,94,220,&AF_S,slc_slot_of(s)<SMP_USER_SLOTS?"K6 MOVE / K7 SPLIT / K8 JOIN":"BUILT-IN: READ ONLY",T_MID,T_SURF);
    }else if(page==SMK37_NAME_PAGE){
        cv_text_in(10,18,220,&AF_M,smk37_name,T_THEME,T_SURF);
        fmt_int(text,smk37_name_pos+1);cv_text_in(10,52,220,&AF_S,text,T_ACCENT,T_SURF);
        cv_text_in(10,83,220,&AF_S,"K5 POSITION / K6 CHARACTER",T_MID,T_SURF);
    }else if(page==SMK37_PHRASE_PAGE){
        unsigned base=smk37_phrase/4*4;for(unsigned i=0;i<4 && base+i<smk37_phrase_count();++i)smk37_list_row(i,smk37_phrase_name(base+i),base+i==smk37_phrase);
    }else if(page==SMK37_UNDO_PAGE){
        panel_note("< PATTERN UNDO / REDO","> SOUND UNDO / REDO","ONE STEP PER EDIT GESTURE");
    }
}
static const char *const wf_names_0[]={"FAMILY","PAGE","","> OPEN"};
static const char *const wf_names_1[]={"SETTING","VALUE","","> SAVE"};
static const char *const wf_names_2[]={"ENGINE","PRESET","FILTER","FAVORITE"};
static const char *const wf_names_3[]={"OP","RATIO","FINE","LEVEL"};
static const char *const wf_names_4[]={"OP","STAGE","RATE","LEVEL"};
static const char *const wf_names_5[]={"OP","MODE","DTUN","VEL"};
static const char *const wf_names_6[]={"ATK","DEC","SUS","REL"};
static const char *const wf_names_7[]={"OP","FIXED","FINE","LEVEL"};
static const char *const wf_names_8[]={"1","10","100","1000"};
static const char *const wf_names_9[]={"SLICE","POS","SPLIT","JOIN"};
static const char *const wf_names_10[]={"SOUND","TRACK","PATTERN","TRACK"};
static const char *const wf_names_11[]={"POSITION","CHAR","USER SLOT",""};
static void smk37_workflow_card(unsigned page,unsigned c,const char **label,char *value,const char **unit){
    static const char *const empty[4]={"","","",""};const char *const *names=empty;
    if(page==SMK37_STORAGE_PAGE){static const char *const sn[]={"STATUS","SLOTS","SIZE","> INIT"};names=sn;
        if(!c)str_cpy(value,smk37_samples_owned?"READY":smk37_samples_state==3?"LOCKED":SMK37_ENABLE_SAMPLES?"SETUP":"OFF",24);
        if(c==1)fmt_int(value,smk37_samples_owned?3:0);if(c==2){fmt_int(value,80);*unit="KIB";}}
    if(page==SMK37_NAV_PAGE){names=wf_names_0;if(!c)str_cpy(value,smk37_groups[smk37_nav_group],24);else if(c==1)fmt_int(value,smk37_nav_row+1);}
    if(page==SMK37_PREF_PAGE){names=wf_names_1;if(!c)str_cpy(value,smk37_pref_names[smk37_pref_row],24);if(c==1)smk37_pref_format(smk37_pref_row,value);}
    if(page==SMK37_BROWSER_PAGE){names=wf_names_2;
        if(!c)str_cpy(value,smk37_browser_engine==NENGINES?"USER":ENGINES[smk37_browser_engine]->name,24);
        if(c==1)fmt_int(value,smk37_browser_count()?smk37_browser_index+1:0);
        if(c==2)str_cpy(value,smk37_browser_fav?"FAV":"ALL",24);
        if(c==3)str_cpy(value,favorite_has(smk37_browser_engine,smk37_browser_index)?"ON":"OFF",24);}
    if(page>=SMK37_OP_PAGE && page<=SMK37_OP2_PAGE){
        names=page==SMK37_OP_PAGE?wf_names_3:page==SMK37_OPENV_PAGE?wf_names_4:wf_names_5;
        if(!c)fmt_int(value,smk37_op+1);
        else if(page==SMK37_OPENV_PAGE && c==1){fmt_int(value,smk37_op_stage+1);*unit=wf_names_6[smk37_op_stage];}
        else {unsigned v=fm6_patch[smk37_draw_track][smk37_op_byte(c)];fmt_int(value,v);
            if(page==SMK37_OP2_PAGE && c==1)str_cpy(value,v?"FIXED":"RATIO",24);
            if(page==SMK37_OP2_PAGE && c==2)fmt_int(value,(int)v-7);
            if(page==SMK37_OP_PAGE && c==1){const uint8_t *op=fm6_patch[smk37_draw_track]+(5-smk37_op)*FP_OP;
                if(op[FP_MODE]){names=wf_names_7;str_cpy(value,wf_names_8[v&3],24);*unit="HZ";}
                else if(!v)str_cpy(value,"0.50",24);}}
    }
    if(page==SMK37_SLICE_PAGE){names=wf_names_9;if(!c){if(smk37_slice_sel>=smk37_slice_count())str_cpy(value,"END",24);else fmt_int(value,smk37_slice_sel+1);}if(c==1)fmt_int(value,smk37_slice_mark(smk37_slice_sel));}
    if(page==SMK37_UNDO_PAGE){names=wf_names_10;fmt_int(value,c==0?smk37_sound_undo.valid:c==1?smk37_sound_undo.track+1:c==2?smk37_undo.valid:smk37_undo.track+1);}
    if(page==SMK37_NAME_PAGE){names=wf_names_11;if(!c)fmt_int(value,smk37_name_pos+1);if(c==1){value[0]=smk37_name[smk37_name_pos];value[1]=0;}if(c==2)fmt_int(value,smk37_name_slot+1);}
    if(page==SMK37_PHRASE_PAGE){static const char *const pn[]={"PHRASE","TRACK","STEPS","> LOAD"};names=pn;if(!c)fmt_int(value,smk37_phrase+1);if(c==1)fmt_int(value,song.sel+1);if(c==2){unsigned k=smk37_phrase_slot(smk37_phrase);fmt_int(value,k<smk37_user_capacity?smk37_user_phrase[k].timing[0]:16);}}
    *label=names[c];
}
