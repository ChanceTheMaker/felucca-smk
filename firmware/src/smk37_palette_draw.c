/* SPDX-License-Identifier: GPL-3.0-only */
static void smk37_palette_apply(void){
    if(smk37_preview_palette){ux_cust.bg=smk37_custom_preview[0];ux_cust.surf=smk37_custom_preview[1];ux_cust.text=smk37_custom_preview[2];ux_cust.theme=smk37_custom_preview[3];ux_cust.accent=smk37_custom_preview[4];palette_set(NPALETTES);}
    else if(cpal_on()){palette_custom_load(ui_cpal);palette_set(NPALETTES);}
    else palette_set(smk37_palette);
}
