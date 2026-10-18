/* SPDX-License-Identifier: GPL-3.0-only */
/* Offline SMK-37 integration image. USB MIDI -> Felucca DSP -> SMK ALNK3,
 * plus the stock-derived SMK display transport. Not an installable release:
 * clock handoff/recovery and physical peripheral behavior need validation.
 * Test10 supports private opt-in save writes after unit/storage validation.
 * The SMK RAM updater, samples and stock-derived TRS OUT are opt-in;
 * FM-1 ADC/UART initialization and radio remain disabled. */
#include <stdint.h>
#include "smk37_unit.h"
#include "smk37_public_profile.h"
#include "smk37_version.h"
#define FELUCCA_FLASH 0
#ifndef SMK37_ENABLE_PRESETS
#define SMK37_ENABLE_PRESETS 0
#endif
#ifndef SMK37_ENABLE_USB_UPDATE
#define SMK37_ENABLE_USB_UPDATE 0
#endif
#define FELUCCA_OTA SMK37_ENABLE_USB_UPDATE
#define FELUCCA_SYSEX 1
#define FELUCCA_UART 0
#define FELUCCA_CDC 0
#ifndef SMK37_ENABLE_USB_AUDIO
#define SMK37_ENABLE_USB_AUDIO 0
#endif
#define FELUCCA_UAC SMK37_ENABLE_USB_AUDIO
#define FELUCCA_ID "SMK37_DEV"
#if SMK37_ENABLE_USB_UPDATE
#define FELUCCA_VERSION SMK37_UPDATE_VERSION
#elif SMK37_ENABLE_SAVES
#define FELUCCA_VERSION "SMK37 TEST10 SAVE"
#else
#define FELUCCA_VERSION "SMK37 TEST10"
#endif
/* Shared AC79 CPU facilities; vector/mailbox addresses match Pro016 static
 * evidence. Timing and clock handoff have not been measured on this board. */
#include "fm1_time.h"
#include "fm1_sys.h"
#include "fm1_irq.h"
#include "fm1_timer.h"
#include "smk37_audio.h"
#include "smk37_lcd_hw.h"
#include "smk37_lights_hw.h"
#include "smk37_panel.h"
#include "smk37_scanner.h"
#include "smk37_contacts.h"
#include "smk37_keyscan.h"
#include "smk37_events.h"
#include "smk37_controls.h"
#include "smk37_button_watch.h"
#include "smk37_rawscan.h"
#include "smk37_adc.h"
#include "smk37_analog_watch.h"
#include "smk37_boot.h"
#include "smk37_bootguard.h"
#include "smk37_time.h"
#include "smk37_memory.h"
#include "smk37_velocity.h"
#include "smk37_pedal.h"
#include "smk37_synth.c"
#include "smk37_remote_input.c"
#if SMK37_REMOTE_PANEL
#define SMK37_PHYSICAL_KEY(k,v) smk37_surface_key(0,k,v)
#else
#define SMK37_PHYSICAL_KEY(k,v) smk37_play_key_event(k,v)
#endif
#if SMK37_REMOTE_PANEL
static unsigned smk37_remote_command(const uint8_t *,unsigned);
static void smk37_remote_blit(unsigned,unsigned,unsigned,unsigned,const uint16_t *);
#endif
#include "smk37_session.c"
#include "smk37_editor.c"
/* SMK writes stay gated until raw flash reads and stock restoration are checked.
 * This is deliberately independent of the FM-1 FELUCCA_FLASH configuration. */
#ifndef SMK37_ENABLE_SAVES
#define SMK37_ENABLE_SAVES 0
#endif
/* Pure arithmetic: RAM flash routines must never call an XIP helper. */
#define FL_RANGE_OK(off,n) (SMK37_ENABLE_SAVES && (((uint32_t)(off)>=SMK37_STORE_LO && \
    (uint32_t)(off)<SMK37_STORE_HI && (uint32_t)(n)>0 && (uint32_t)(n)<=SMK37_STORE_HI-(uint32_t)(off)) || \
    (SMK37_ENABLE_PRESETS && (SMK37_PRESET_RANGE(off,n) || (SMK37_EXPANDED_PRESETS && SMK37_EXPANDED_RANGE(off,n)))) || SMK37_SAMPLE_WRITE_OK(off,n)))
#if SMK37_ENABLE_SAMPLES
static uint8_t smk37_samples_formatting;
#define SMK37_SAMPLE_WRITE_OK(off,n) (((smk37_samples_owned || smk37_samples_formatting) && SMK37_SAMPLE_RANGE(off,n)) || (smk37_samples_formatting && SMK37_SAMPLE_JOURNAL_RANGE(off,n)))
#else
#define SMK37_SAMPLE_WRITE_OK(off,n) 0
#endif
#include "fm1_flash.h"
#include "smk37_flash_profile.h"
#if SMK37_ENABLE_TRS
#if !FELUCCA_UAC
#error "SMK TRS service requires the existing 2 kHz USB timer"
#endif
#include "smk37_midi_output.c"
#endif

volatile smk37_bootguard_t smk37_boot_state __attribute__((section(".noinit")));

static int32_t smk37_dma[HALF_FRAMES * 4u] __attribute__((aligned(4)));
static volatile uint32_t smk37_rendered, smk37_late, smk37_scan_frames;
static volatile uint32_t smk37_audio_irqs;
static volatile uint32_t smk37_frame_ticks, smk37_last_velocity;
static uint32_t smk37_frame_last;
static smk37_keyscan_t smk37_keys[37];
static smk37_controls_t smk37_controls;
static volatile smk37_rawscan_t smk37_rawscan;
/* Aligned single-word diagnostic snapshots; no synth mutation in scan IRQ. */
static volatile uint32_t smk37_last_button, smk37_last_encoder;
static volatile smk37_button_watch_t smk37_buttons;
static smk37_adc_t smk37_adc;
static volatile smk37_analog_watch_t smk37_analog_watch;
/* Keep contact duration for measured-time generic velocity curves. Never call
 * synthesis from the scanner ISR or relabel generic response as factory trim. */
static smk37_events_t smk37_key_events;
/* Separate queue: losing an encoder update cannot lose a key release. */
static smk37_events_t smk37_encoder_events;
static smk37_events_t smk37_button_events;
static smk37_events_t smk37_pad_events;
static smk37_pad_state_t smk37_pad_state[16];
static smk37_pedal_t smk37_pedal_input;
static volatile uint32_t smk37_pedal_latest; /* stable level, applied only by audio consumer */
static volatile uint32_t smk37_analog_latest[6]; /* bit 16 valid, low bits raw */
static volatile uint8_t smk37_analog_failed;
static volatile uint32_t smk37_control_overflows;
static volatile uint32_t smk37_key_overflows;
static uint16_t smk37_pedal_gpio_base[4],smk37_pedal_gpio_changed[4];
static uint8_t smk37_pedal_probe_active;
static uint8_t smk37_pedal_frame[16],smk37_pedal_last_contact;
static uint32_t smk37_pedal_contact_changes;
static const uint32_t smk37_pedal_ports[4]={0x50004u,0x50044u,0x50084u,0x501c4u};
static const uint16_t smk37_pedal_exclude[4]={0x07ffu,0x00e0u,0x07c7u,0x0340u};
void smk37_scanner_irq(void)
{
    unsigned key;
    if (!(SMK37_SCAN_CON & 0x8000u)) return;
    unsigned analog_index = smk37_adc.scan.index;
    int analog_result = smk37_adc_spi_done(&smk37_adc);
    if (analog_result < 0) smk37_analog_failed = 1;
    if (analog_result > 0) {
        uint16_t raw = smk37_adc.scan.raw[analog_index];
        smk37_analog_watch_update(&smk37_analog_watch, analog_index,
                                raw);
        if(analog_index==22){
            int pedal=smk37_pedal_sample(&smk37_pedal_input,raw);
            if(pedal>=0)smk37_pedal_latest=(uint32_t)pedal;
        }
        for (unsigned i=0; i<16; ++i) if (analog_index == smk37_pad_input[i]) {
            smk37_pad_change_t change = smk37_pad_sample(&smk37_pad_state[i], raw);
            if (change.velocity) smk37_events_push(&smk37_pad_events, i, change.velocity);
            if (change.release) smk37_events_push(&smk37_pad_events, i, 0);
        }
        for (unsigned i=0; i<4; ++i) if (analog_index==smk37_fader_input[i])
            smk37_analog_latest[i] = 0x10000u | raw;
        if (analog_index==16 || analog_index==17)
            smk37_analog_latest[analog_index-12] = 0x10000u | raw;
    }
    smk37_scan_set_analog(smk37_analog_mask(&smk37_adc.scan));
    if (!smk37_scan_tick()) return;
    uint32_t now=fm1_ticks(), elapsed=now-smk37_frame_last;
    smk37_frame_last=now;
    if(smk37_scan_frames && elapsed && elapsed<240000u)
        smk37_frame_ticks=smk37_frame_ticks ? (smk37_frame_ticks*7u+elapsed)/8u : elapsed;
    ++smk37_scan_frames;
    if(smk37_page==SMK37_PEDAL_PAGE) {
        for(unsigned i=0;i<4;++i){
            uint16_t v=(uint16_t)SMK37_SCAN_REG(smk37_pedal_ports[i])&~smk37_pedal_exclude[i];
            if(!smk37_pedal_probe_active){smk37_pedal_gpio_base[i]=v;smk37_pedal_gpio_changed[i]=0;}
            else smk37_pedal_gpio_changed[i]|=v^smk37_pedal_gpio_base[i];
        }
        if(!smk37_pedal_probe_active){smk37_pedal_contact_changes=0;smk37_pedal_last_contact=0;}
        else for(unsigned i=0;i<16;++i){
            unsigned delta=smk37_scan_frame[i]^smk37_pedal_frame[i];
            for(unsigned b=0;b<8;++b)if(delta&(1u<<b)){
                smk37_pedal_last_contact=(i<<4)|b;++smk37_pedal_contact_changes;
            }
        }
        memcpy(smk37_pedal_frame,smk37_scan_frame,sizeof smk37_pedal_frame);
        smk37_pedal_probe_active=1;
    } else smk37_pedal_probe_active=0;
    smk37_rawscan_update(&smk37_rawscan, smk37_scan_frame);
    smk37_control_changes_t controls = smk37_controls_tick(&smk37_controls, smk37_scan_frame);
    smk37_button_watch_update(&smk37_buttons, &controls);
    for (unsigned i = 0; i < SMK37_BUTTON_COUNT; ++i) {
        if (controls.pressed & (1u << i)) {
            smk37_last_button = ((i+1u) << 8) | 1u;
            smk37_events_push(&smk37_button_events,i,1);
        }
        if (controls.released & (1u << i)) {smk37_last_button = (i+1u) << 8;smk37_events_push(&smk37_button_events,i,0);}
    }
    for (unsigned i = 0; i < SMK37_ENCODER_COUNT; ++i)
        if (controls.encoder[i]) {
            smk37_last_encoder = ((i+1u) << 8) | (controls.encoder[i] > 0 ? 1u : 2u);
            smk37_events_push(&smk37_encoder_events, i, controls.encoder[i] > 0 ? 1u : 2u);
        }
    for (key = 0; key < 37; ++key) {
        uint32_t duration;
        unsigned raw = smk37_contact_closed(smk37_scan_frame, smk37_key_contacts[key][0]) |
                       smk37_contact_closed(smk37_scan_frame, smk37_key_contacts[key][1]) << 1;
        if (!smk37_keyscan_tick(&smk37_keys[key], raw, &duration)) continue;
        (void)smk37_events_push(&smk37_key_events, key, duration);
    }
}
static void smk37_consume_keys(void)
{
    smk37_performance_poll();
#if SMK37_REMOTE_PANEL
    smk37_remote_poll();
#endif
    if (smk37_button_events.overflow) {
        fm1_irq_off(); smk37_events_discard(&smk37_button_events); fm1_irq_on();
        song.rec=0; transport_req=2;smk37_button_down=smk37_button_held=0;
#if SMK37_REMOTE_PANEL
        memset(smk37_surface_buttons,0,sizeof smk37_surface_buttons);
#endif
        smk37_notice="BUTTON QUEUE LOST / STOP"; smk37_notice_at=fm1_ms;
    }
    smk37_event_t button;
    unsigned button_budget=18;
    while (button_budget-- && smk37_events_pop(&smk37_button_events,&button))
#if SMK37_REMOTE_PANEL
        smk37_surface_button(0,button.key,button.duration);
#else
        smk37_button_input(button.key,button.duration);
#endif
    smk37_hold_poll();
    if (smk37_pad_events.overflow || smk37_analog_failed) {
        fm1_irq_off();
        smk37_events_discard(&smk37_pad_events);
        fm1_irq_on();
        for (unsigned i=0; i<16; ++i)
#if SMK37_REMOTE_PANEL
            smk37_surface_pad(0,i,0);
#else
            smk37_pad_event(i,0);
#endif
        if (smk37_analog_failed && smk37_analog_ready) {
            smk37_wheels(0, 0, smk37_wheel_targets);
            smk37_analog_ready = 0;
        }
    } else {
        smk37_event_t pad;
        unsigned budget = SMK37_EVENTS;
        while (budget-- && smk37_events_pop(&smk37_pad_events, &pad))
#if SMK37_REMOTE_PANEL
            smk37_surface_pad(0,pad.key,pad.duration);
#else
            smk37_pad_event(pad.key,pad.duration);
#endif
    }
    /* No synthesis or USB from scanner IRQ. A scanner failure releases pedal
     * ownership and requires a fresh physical release before arming again. */
    if(smk37_analog_failed){
        fm1_irq_off();smk37_pedal_reset(&smk37_pedal_input);smk37_pedal_latest=0;fm1_irq_on();
    }
#if SMK37_REMOTE_PANEL
    smk37_sustain(smk37_pedal_latest || smk37_remote_pedal);
#else
    smk37_sustain(smk37_pedal_latest);
#endif
    uint16_t analog[6];
    unsigned analog_valid = 0;
    for (unsigned i=0; i<6; ++i) {
        uint32_t value = smk37_analog_latest[i];
        analog[i] = (uint16_t)value;
        if (value & 0x10000u) analog_valid |= 1u<<i;
    }
    if (!smk37_analog_failed) smk37_analog_playback(analog, analog_valid);
    if (smk37_encoder_events.overflow) {
        fm1_irq_off();
        smk37_events_discard(&smk37_encoder_events);
        fm1_irq_on();
        memset(smk37_encoder_phase, 0, sizeof smk37_encoder_phase);
        ++smk37_control_overflows;
    }
    /* At most one quarter-step per audio half. Preset/engine work is bounded,
     * and a full encoder queue cannot starve note releases. */
    smk37_event_t control;
    if (smk37_events_pop(&smk37_encoder_events, &control))
        smk37_encoder_event(control.key, control.duration == 1 ? 1 : -1);
    unsigned budget = SMK37_EVENTS;
    if (smk37_key_events.overflow) {
        /* Mask the producer while discarding its queue; release all local keys
         * rather than leave a missed release sounding indefinitely. */
        fm1_irq_off();
        smk37_events_discard(&smk37_key_events);
        fm1_irq_on();
        ++smk37_key_overflows;
        for (unsigned k = 0; k < 37; ++k)
#if SMK37_REMOTE_PANEL
            smk37_surface_key(0,k,0);
#else
            smk37_key_event(k,48+k,0);
#endif
    }
    smk37_event_t event;
    while (budget-- && smk37_events_pop(&smk37_key_events, &event)) {
        unsigned key = event.key;
        unsigned velocity=smk37_key_velocity(event.duration,smk37_frame_ticks,smk37_velocity_mode);
        if(velocity) smk37_last_velocity=velocity;
        SMK37_PHYSICAL_KEY(key, velocity);
    }
}
static int smk37_panel_command(uint8_t c) { return smk37_lcd_command(c, 4000000u); }
static int smk37_panel_data(const void *p, uint32_t n) { return smk37_lcd_data(p, n, 4000000u); }
static const smk37_panel_io_t smk37_panel_io = {
    smk37_panel_command, smk37_panel_data, fm1_delay_ms
};
static const smk37_audio_hooks_t smk37_audio_hooks = {
    fm1_delay_ms, fm1_irq_attach
};
static uint8_t smk37_display_ok;
static int smk37_flash_read_raw(uint32_t off,void *dst,uint32_t n)
{
    uint8_t *p=dst;
    if(off>0x100000u || n>0x100000u-off)return -1;
    while(n){unsigned k=n>256?256:n;fm1_irq_off();int rc=FL_FAR(fl_read_ram)(off,p,k);fm1_irq_on();
        if(rc)return rc;p+=k;off+=k;n-=k;fm1_wdt_feed();}
    return 0;
}
static int smk37_storage_read(uint32_t off,void *dst,uint32_t n)
{
#if SMK37_ENABLE_SAMPLES
    if(SMK37_SAMPLE_RANGE(off,n)||SMK37_SAMPLE_JOURNAL_RANGE(off,n))return smk37_flash_read_raw(off,dst,n);
#endif
    return (smk37_store_range(off,n)||(SMK37_ENABLE_PRESETS && (SMK37_PRESET_RANGE(off,n) || (SMK37_EXPANDED_PRESETS && SMK37_EXPANDED_RANGE(off,n)))))?smk37_flash_read_raw(off,dst,n):-1;
}
static int smk37_storage_erase(uint32_t off)
{
    if(!SMK37_ENABLE_SAVES || !smk37_storage_writable || !FL_RANGE_OK(off,4096) || (off&4095))return -1;
    memset(smk37_dma,0,sizeof smk37_dma);
    uint32_t took_us;return fl_erase4k(off,&took_us);
}
static int smk37_storage_program(uint32_t off,const void *src,uint32_t n)
{
    if(!SMK37_ENABLE_SAVES || !smk37_storage_writable || !FL_RANGE_OK(off,n))return -1;
    memset(smk37_dma,0,sizeof smk37_dma);return fl_write(off,src,n);
}
#if SMK37_ENABLE_SAMPLES
static const uint32_t smk37_sample_stock_crc[3]=SMK37_UNIT_SAMPLE_CRCS;
#endif
static uint32_t smk37_current_trim_crc=SMK37_UNIT_TRIM_CRC;
#if SMK37_ENABLE_SAMPLES
static const uint8_t *smk37_sample_xip(unsigned k){return smk37_samples_owned&&k<3?fm1_xip_ptr(smk37_sample_address(k)):smk37_empty_sample;}
static void smk37_sample_invalidate(unsigned k){fm1_irq_off();fl_inval(smk37_sample_address(k),SMP_USER_SIZE);fm1_irq_on();}
static void smk37_samples_probe(void){
    int state=smk37_sample_owner_state(&smk37_storage_io,smk37_current_trim_crc);
    smk37_samples_state=state<0?3:state;smk37_samples_owned=0;
    if(state!=2)return;
    /* SMK's application ends at 0x9d000; never use the FM-1's 0x93000
     * plain-window boundary, which overlaps the SMK application allocation. */
    fm1_irq_off();SFCENC_CON&=(uint8_t)~2u;SFCENC_UNENC_L=FL_XIP(0x9d000u);SFCENC_UNENC_H=0x07ffffffu;SFCENC_CON|=2u;fm1_irq_on();
    uint8_t raw[256];
    for(unsigned k=0;k<3;++k){smk37_sample_invalidate(k);
        if(smk37_storage_read(smk37_sample_address(k),raw,256)||memcmp(raw,fm1_xip_ptr(smk37_sample_address(k)),256)){smk37_samples_state=3;return;}}
    smk37_samples_owned=1;slc_store_boot();for(unsigned k=0;k<3;++k)smp_user_scan(k);
}
static int smk37_sample_format(void){
    if(!smk37_storage_writable||!smk37_up_idle())return -1;
    smk37_samples_formatting=1;
    const uint32_t *expected=smk37_sample_stock_crc;
#if SMK37_PUBLIC_BUILD
    /* Explicit initialization replaces this instrument's old sample contents.
     * Existing ownership still binds to this instrument's own trim CRC. */
    uint32_t current[3];uint8_t block[256];
    for(unsigned k=0;k<3;++k){uint32_t crc=~0u;
        for(unsigned p=0;p<SMK37_SAMPLE_SIZE;p+=256){
            if(smk37_storage_read(smk37_sample_address(k)+p,block,256)){smk37_samples_formatting=0;return -1;}
            crc=smk37_profile_crc(block,256,crc);
        }current[k]=~crc;
    }expected=current;
#endif
    int rc=smk37_sample_initialize(&smk37_storage_io,smk37_current_trim_crc,expected);
    smk37_samples_formatting=0;
    if(!rc)smk37_samples_probe();
    return rc||!smk37_samples_owned?-1:0;
}
#endif
static int smk37_storage_probe(void)
{
    static uint8_t trim[556];
    fm1_irq_off();uint32_t id=FL_FAR(fl_jedec_ram)();int profile_ok=smk37_flash_supported_id(id);fm1_irq_on();
    if(!profile_ok || smk37_flash_read_raw(0xc1000u,trim,sizeof trim))return -1;
#if SMK37_PUBLIC_BUILD
    if(!smk37_trim_keys_valid(trim,sizeof trim))return -1;
    uint8_t block[256];uint32_t head_crc=~0u;
    for(unsigned p=0;p<0x4000;p+=256){
        if(smk37_flash_read_raw(p,block,256))return -1;
        head_crc=smk37_profile_crc(block,256,head_crc);
    }
    if(~head_crc!=SMK37_PUBLIC_HEAD_CRC)return -1;
    smk37_current_trim_crc=smk37_store_crc(trim,sizeof trim);
#else
    /* Legacy private builds retain their original unit-bound gate. */
    if(smk37_store_crc(trim,sizeof trim)!=SMK37_UNIT_TRIM_CRC)return -1;
#endif
    if(!smk37_store_allocation_ok(&smk37_storage_io,sizeof smk37_session))return -1;
    smk37_storage_ready=1;smk37_storage_writable=SMK37_ENABLE_SAVES;
    if(SMK37_ENABLE_PRESETS)(void)smk37_up_boot();
#if SMK37_ENABLE_SAMPLES
    smk37_samples_probe();
#endif
    return 0;
}
static void smk37_project_service(void)
{
#if SMK37_ENABLE_SAMPLES
    if(smk37_samples_request){smk37_samples_request=0;int rc=smk37_sample_format();smk37_notice=rc?"SAMPLES: STOP / CHECK STORAGE":"SAMPLES READY / RECONNECT WEB";smk37_notice_at=fm1_ms;}
#endif
    if(smk37_prefs_request && smk37_up_idle()){smk37_prefs_request=0;int rc=smk37_preferences_save();smk37_notice=rc?"SETTINGS UNSAVED / STOP FIRST":"SETTINGS / FAVORITES SAVED";smk37_notice_at=fm1_ms;}
    if(smk37_rename_request){smk37_rename_request=0;int rc=smk37_preset_rename();smk37_notice=rc?"RENAME FAILED / STOP FIRST":"PRESET RENAMED";smk37_notice_at=fm1_ms;}
    if(smk37_song_request){smk37_song_request=0;unsigned rc=smk37_chain_prepare();smk37_notice=rc?"SONG: STOP / CHECK SLOTS":"SONG PLAYING";smk37_notice_at=fm1_ms;}
    if(smk37_user_request){unsigned r=smk37_user_request;smk37_user_request=0;
        int rc=(r&3)==1?smk37_up_load(r>>2):smk37_up_store(r>>2,0);
        smk37_notice=rc?"PRESET: EMPTY / STOP FIRST":(r&3)==1?"USER PRESET LOADED":"USER PRESET SAVED";smk37_notice_at=fm1_ms;}
    unsigned request=smk37_project_request;
    if(!request)return;
    smk37_project_request=0;
    if(song.playing || song.rec || transport_req){smk37_notice="STOP BEFORE PROJECT IO";goto done;}
    if(!smk37_storage_ready && smk37_storage_probe()){smk37_notice="FLASH READ CHECK FAILED";goto done;}
    if((request&3u)==1u)smk37_notice=smk37_session_load(request>>2)?"EMPTY / INVALID SLOT":"PROJECT LOADED";
    else if(!smk37_storage_writable)smk37_notice="READ OK / SAVES DISABLED";
    else smk37_notice=smk37_session_save(request>>2)?"SAVE FAILED / HELD NOTES":"PROJECT SAVED";
done:smk37_notice_at=fm1_ms;
}
static uint32_t ota_now_ms(void){return fm1_ticks()/24000u;}
static void ota_idle(void){
#if !FELUCCA_UAC
    usb_poll();
#endif
    fm1_wdt_feed();
}
#if SMK37_ENABLE_USB_UPDATE
#include "smk37_loader_image.h"
static int smk37_update_identity(const uint8_t *frame,uint32_t count){
    static const uint8_t query[]={0x00,0x32,0x45,0x00,0x00,0x00,0x40,0x7f};
    if(count!=sizeof query)return 0;
    for(unsigned i=0;i<sizeof query;++i)if(frame[i]!=query[i])return 0;
    ota_frame_done();ota_wire_send(smk37_update_identity_wire,sizeof smk37_update_identity_wire);return 1;
}
#endif
static void smk37_editor_service(void)
{
    const uint8_t *frame;uint32_t count;unsigned n=0;
    static uint32_t changed_at,resets;
    if(resets!=usb.resets){resets=usb.resets;smk37_ed_watch=0;}
    if(ota_frame_get(&frame,&count)){
#if SMK37_ENABLE_USB_UPDATE
        if(smk37_update_identity(frame,count))return;
#endif
#if SMK37_REMOTE_PANEL
        if(count>=4 && frame[0]==125 && frame[1]==70 && frame[2]==76 && frame[3]==126){
            n=smk37_remote_command(frame,count);ota_frame_done();
        }else
#endif
        if(count==6 && frame[0]==0x7d && frame[1]==0x46 && frame[2]==0x4c && frame[3]==9 && frame[4]<=2 && frame[5]<4){
            unsigned op=frame[4],slot=frame[5];ota_frame_done();
            int rc=0;
            if(op<2 && !smk37_storage_ready)rc=smk37_storage_probe();
            if(!rc && op==1)rc=smk37_session_save(slot);
            if(!rc && op==0)rc=smk37_session_load(slot);
            if(!rc){smk37_ed_begin(9);smk37_ed_byte(op);smk37_ed_byte(slot);smk37_ed_byte(smk37_session_used(slot));n=smk37_ed_finish();}
        }else if(count>=4 && frame[0]==0x7d && frame[1]==0x46 && frame[2]==0x4c &&
            (frame[3]==72 || frame[3]==73 || frame[3]==33 || (SMK37_ENABLE_SAMPLES&&frame[3]>=11&&frame[3]<=15) || (frame[3]>=34&&frame[3]<=38) || (frame[3]>=16&&frame[3]<=21) || ((frame[3]==68||frame[3]==69)&&count>=6&&frame[4]==3))){
            n=smk37_editor_main_command(frame,count);ota_frame_done();
        }else{
            fm1_irq_off();n=smk37_editor_handle(frame,count);fm1_irq_on();ota_frame_done();
        }
    }else if(fm1_ms-changed_at>=20u && so_w-so_r<SXQ/4u){
        changed_at=fm1_ms;fm1_irq_off();n=smk37_editor_changed();fm1_irq_on();
    }
    if(n)ota_wire_send(smk37_ed_out,n);
}
static void lcd_sync(void) {} /* SMK transport completes DMA synchronously. */
static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *pixels)
{
    if (!smk37_display_ok) return;
#if SMK37_REMOTE_PANEL
    smk37_remote_blit(x,y,w,h,pixels);
#endif
    if (!smk37_panel_window(&smk37_panel_io, x, y, w, h) ||
        !smk37_panel_data(pixels, w*h*2u)) smk37_display_ok = 0;
}
#include "gfx.c"
#include "smk37_remote_screen.c"
static uint8_t smk37_light_buffer[SMK37_LIGHT_BYTES] __attribute__((aligned(4)));
static uint8_t smk37_light_failed;
static uint32_t smk37_light_transfers;
static void smk37_light_service(void)
{
    static uint32_t previous = UINT32_MAX;
    unsigned target, color, level, page;
    uint16_t pads=0;
    uint32_t indicators;
    fm1_irq_off();
    page=smk37_page;
    target = smk37_page == SMK37_LIGHT_PAGE ? smk37_light_target : 0;
    color = smk37_light_color; level = smk37_page==SMK37_LIGHT_PAGE?smk37_light_level:smk37_led_level;
    indicators=smk37_indicator_status();
    if(page==SMK37_STEP_PAGE) {
        color=1;level=smk37_led_level;
        unsigned bank=smk37_edit_step&~15u;
        for(unsigned i=0;i<16;++i)if(TSEL->step[bank+i].time!=ST_REST)
            pads|=(uint16_t)(1u<<i);
    } else if(page<SMK37_SOUND_PAGES) {
        color=2; level=smk37_led_level;
        for(unsigned i=0;i<16;++i)
            if(kb_chn[37+i] || ((smk37_pad_seen&(1u<<i)) && fm1_ms-smk37_pad_hit_at[i]<120u))
                pads|=(uint16_t)(1u<<i);
    } else if(target>=1 && target<=16) pads=(uint16_t)(1u<<(target-1));
    fm1_irq_on();
    /* Publish both modes together so the scanner never combines old diagnostic
     * selection with newly selected performance indicators. */
    fm1_irq_off();
    smk37_scan_indicator = !smk37_light_failed && target > 16 ? target-16 : 0;
    smk37_scan_indicator_bits = !smk37_light_failed && (page>=SMK37_SOUND_PAGES || smk37_led_level) ? indicators : 0;
    fm1_irq_on();
#if SMK37_REMOTE_PANEL
    smk37_remote_led_pads=smk37_light_failed?0:pads;
    smk37_remote_led_buttons=smk37_scan_indicator_bits |
        (smk37_scan_indicator ? 1u<<(smk37_scan_indicator-1) : 0);
    smk37_remote_led_color=color;smk37_remote_led_level=smk37_light_failed?0:(level>8?8:level);
#endif
    uint32_t state = pads | (color<<16) | (level<<24);
    if (state == previous || smk37_light_failed) return;
    previous = state;
    smk37_light_encode_mask(smk37_light_buffer,pads,color,level);
    if (smk37_lights_send(smk37_light_buffer, 4000000u)) ++smk37_light_transfers;
    else {
        smk37_light_failed=1;
        fm1_irq_off(); smk37_scan_indicator=smk37_scan_indicator_bits=0; fm1_irq_on();
    }
}
static void smk37_button_status(void)
{
    smk37_button_watch_t b;
    uint32_t encoder;
    fm1_irq_off(); b=smk37_buttons; encoder=smk37_last_encoder; fm1_irq_on();
    char text[32];
    draw_text_line(0, 40, 240, &AF_S, "TEST10 BUTTON CHECK", 0x07ff, 0, 1);
    for (unsigned i=0; i<18; ++i) {
        text[0]='B'; text[1]='0'+(i+1)/10; text[2]='0'+(i+1)%10;
        text[3]=b.held&(1u<<i) ? '+' : b.seen&(1u<<i) ? '*' : '-'; text[4]=0;
        draw_text_line(12+(i/6)*78, 64+(i%6)*18, 72, &AF_S, text,
                       b.held&(1u<<i) ? 0x07e0 : b.seen&(1u<<i) ? 0xffe0 : 0xffff, 0, 0);
    }
    str_cpy(text, "LAST PRESS B", sizeof text); fmt_int(text+12, b.last_press);
    draw_text_line(8, 174, 224, &AF_S, text, 0x07ff, 0, 0);
    str_cpy(text, "PRESS ", sizeof text); fmt_int(text+6, b.presses%1000000u);
    draw_text_line(8, 191, 108, &AF_S, text, 0xffff, 0, 0);
    str_cpy(text, "UP ", sizeof text); fmt_int(text+3, b.releases%1000000u);
    draw_text_line(120, 191, 112, &AF_S, text, 0xffff, 0, 0);
    str_cpy(text, "KNOB 0 -", sizeof text);
    if (encoder) { text[5]='0'+(encoder>>8); text[7]=(encoder&255)==1 ? '+' : '-'; }
    draw_text_line(0, 207, 240, &AF_S, text, 0xffff, 0, 1);
    draw_text_line(0, 224, 240, &AF_S, "+ HELD  * SEEN  E4: LIGHTS", 0x07ff, 0, 1);
}
static void smk37_light_status(void)
{
    unsigned target, color, level;
    fm1_irq_off(); target=smk37_light_target; color=smk37_light_color; level=smk37_light_level; fm1_irq_on();
    char text[32];
    draw_text_line(0, 40, 240, &AF_S, "TEST10 LIGHT CHECK", 0x07ff, 0, 1);
    str_cpy(text, target ? target<=16 ? "PAD " : "INDICATOR " : "ALL OFF", sizeof text);
    if (target) fmt_int(text+(target<=16 ? 4 : 10), target<=16 ? target : target-16);
    draw_text_line(0, 70, 240, &AF_M, text, 0xffff, 0, 1);
    static const char *colors[3]={"RED", "GREEN", "BLUE"};
    draw_text_line(0, 100, 240, &AF_S, target>16 ? "FIXED COLOR / 25% DUTY" : colors[color], 0xffff, 0, 1);
    str_cpy(text, "PAD LEVEL ", sizeof text); fmt_int(text+10, level);
    draw_text_line(0, 121, 240, &AF_S, text, 0xffff, 0, 1);
    draw_text_line(8, 151, 224, &AF_S, "E1: SELECT ONE LIGHT", 0xffff, 0, 0);
    draw_text_line(8, 170, 224, &AF_S, "E2: RGB  E3: LEVEL 1-8", 0xffff, 0, 0);
    draw_text_line(8, 189, 224, &AF_S, "E4: EXIT LIGHT TEST", 0xffff, 0, 0);
    draw_text_line(0, 220, 240, &AF_S, smk37_light_failed ? "LED TIMEOUT - STOPPED" : "LOW BRIGHTNESS / SINGLE LED",
                   smk37_light_failed ? 0xf800 : 0x07ff, 0, 1);
}
#include "smk37_draw.c"
static void smk37_status(void)
{
    if (!smk37_display_ok) return;
    static unsigned shown_page = UINT32_MAX;
#if SMK37_REMOTE_PANEL
    if(smk37_remote_refresh){smk37_remote_refresh=0;shown_page=UINT32_MAX;}
#endif
    static unsigned shown_palette=UINT32_MAX,shown_bold=UINT32_MAX,shown_style=UINT32_MAX,shown_large=UINT32_MAX;
    if(shown_palette!=smk37_palette || shown_bold!=smk37_bold || shown_style!=smk37_style || shown_large!=smk37_large){
        ux.style=smk37_style?ST_LINE:ST_FLAT;shown_style=smk37_style;shown_large=smk37_large;
        palette_set(smk37_palette);font_set(smk37_bold);shown_palette=smk37_palette;shown_bold=smk37_bold;shown_page=UINT32_MAX;
    }
    static uint8_t sleeping;
    static const uint32_t timeout[]={0,300000,900000,1800000,3600000};
    if(smk37_screen_off && fm1_ms-smk37_input_at>=timeout[smk37_screen_off]){
        if(!sleeping){cv_begin(240,120,0);cv_blit(0,0);cv_blit(0,120);sleeping=1;}return;
    }
    if(sleeping){sleeping=0;shown_page=UINT32_MAX;}
    unsigned page = smk37_page;
    if (shown_page != page) {
        cv_begin(240, 120, page<SMK37_SOUND_PAGES ? T_BG : 0); cv_blit(0, 0); cv_blit(0, 120);
        if(page>=SMK37_SOUND_PAGES)draw_text_line(0,16,240,&AF_M,"FELUCCA [Salt]",0xffff,0,1);
        shown_page = page;
    }
    if (page == SMK37_BUTTON_PAGE) { smk37_button_status(); return; }
    if (page == SMK37_LIGHT_PAGE) { smk37_light_status(); return; }
    if (page == SMK37_PEDAL_PAGE) {
        draw_text_line(0,44,240,&AF_M,"PEDAL CHECK",0x07ff,0,1);
        draw_text_line(0,72,240,&AF_S,"PRESS / RELEASE PEDAL ONLY",0xffff,0,1);
        for(unsigned i=0;i<4;++i){
            char line[24]="PA CHANGED 0000";line[1]="ABCH"[i];
            uint16_t bits=smk37_pedal_gpio_changed[i];
            for(unsigned b=0;b<4;++b)line[11+b]="0123456789ABCDEF"[(bits>>(12-4*b))&15];
            line[15]=0;draw_text_line(8,92+i*20,224,&AF_S,line,0xffff,0,0);
        }
        char line[32]="ADC23 ";fmt_int(line+6,smk37_analog_watch.current[22]);
        draw_text_line(8,174,110,&AF_S,line,0xffff,0,0);
        str_cpy(line,"ADC24 ",sizeof line);fmt_int(line+6,smk37_analog_watch.current[23]);
        draw_text_line(120,174,112,&AF_S,line,0xffff,0,0);
        str_cpy(line,"CONTACT -- / ",sizeof line);
        if(smk37_pedal_contact_changes){line[8]="0123456789ABCDEF"[smk37_pedal_last_contact>>4];line[9]="0123456789ABCDEF"[smk37_pedal_last_contact&15];}
        fmt_int(line+13,smk37_pedal_contact_changes%1000000u);
        draw_text_line(8,196,224,&AF_S,line,0xffff,0,0);
        draw_text_line(0,220,240,&AF_S,"READ ONLY / E4 EXIT",0x07ff,0,1);return;
    }
    if (page < SMK37_SOUND_PAGES) {
        smk37_sound_status();
        return;
    }
    uint16_t raw[SMK37_ANALOG_INPUTS], span;
    uint8_t last;
    uint32_t valid, changed, samples, timeouts;
    /* Short copy only: formatting and display DMA run with IRQs enabled. */
    fm1_irq_off();
    for (unsigned i=0; i<SMK37_ANALOG_INPUTS; ++i)
        raw[i] = smk37_analog_watch.current[i];
    valid = smk37_analog_watch.valid;
    changed = smk37_analog_watch.changed;
    last = smk37_analog_watch.last;
    span = last ? smk37_analog_watch.high[last-1] - smk37_analog_watch.low[last-1] : 0;
    samples = smk37_adc.samples;
    timeouts = smk37_adc.timeouts;
    fm1_irq_on();
    draw_text_line(0, 40, 240, &AF_S, "TEST10 ANALOG INPUT", 0x07ff, 0, 1);
    char text[32];
    str_cpy(text, timeouts ? "ADC TIMEOUT " : "SAMPLES ", sizeof text);
    fmt_int(text+(timeouts ? 12 : 8), (int32_t)(samples % 100000000u));
    draw_text_line(8, 58, 224, &AF_S, text, timeouts ? 0xf800 : 0xffff, 0, 0);
    for (unsigned i=0; i<SMK37_ANALOG_INPUTS; ++i) {
        /* Three columns of eight, numbered down columns. No physical names
         * until hardware mapping. A sticky star preserves brief pad hits. */
        text[0] = '0' + (i+1)/10; text[1] = '0' + (i+1)%10; text[2] = ':';
        for (unsigned j=0, divisor=1000; j<4; ++j, divisor/=10)
            text[3+j] = valid & (1u<<i) ? '0' + (raw[i]/divisor)%10 : '-';
        text[7] = changed & (1u<<i) ? '*' : ' '; text[8] = 0;
        draw_text_line(6+(i/8)*78, 80+(i%8)*17, 76, &AF_S, text,
                       changed & (1u<<i) ? 0xffe0 : 0xffff, 0, 0);
    }
    str_cpy(text, "MOVE ONE CONTROL", sizeof text);
    if (last) {
        str_cpy(text, "LAST 00 RANGE ", sizeof text);
        text[5] = '0' + last/10; text[6] = '0' + last%10;
        fmt_int(text+14, span);
    }
    draw_text_line(0, 220, 240, &AF_S, text, 0x07ff, 0, 1);
}

void smk37_audio_irq(void)
{
    ++smk37_audio_irqs;
    uint8_t pending = smk37_audio_pending();
    smk37_audio_ack_aux(pending);
    if (pending & SMK37_AUDIO_HALF) {
        unsigned half = smk37_audio_free_half(), b, i;
        int32_t *out = smk37_dma + half * HALF_FRAMES * 2u;
        smk37_consume_keys();
        fm6_poll();
#if FELUCCA_UAC
        uac_render_start();
#endif
        for (b = 0; b < HALF_FRAMES; b += CTL) {
            mix_block(out + b * 2u, CTL);
#if FELUCCA_UAC
            uac_tap(out + b * 2u, CTL);
#endif
            if(fx_usb_fixed)usb_fixed_dac(out + b * 2u,CTL);
            if(!scope_mix)for(unsigned j=1;j<CTL;j+=2)scope_buf[scope_w++ & (SCOPE_N-1u)]=(int16_t)out[(b+j)*2u];
            click_render(out + b * 2u, CTL);
        }
        for (i = 0; i < HALF_FRAMES * 2u; ++i) out[i] *= 128;
        smk37_audio_ack_half();
        ++smk37_rendered;
        if (smk37_audio_free_half() != half) ++smk37_late;
    }
}
/* UAC needs one packet each millisecond even while the LCD transfers.
 * A single 2 kHz TIMER5 consumer owns every SIE access. Priority 4 allows it
 * to preempt rendering (3); SMK contact scanning keeps priority 5. The main
 * loop never polls USB concurrently with this consumer. Retry stays in main:
 * usb.up is false throughout its 25 ms reset, so usb_poll touches no SIE. */
#if FELUCCA_UAC
void smk37_usb_irq(void){fm1_timer5_ack();usb_poll();
#if SMK37_ENABLE_TRS
    smk37_trs_poll();
#endif
}
extern void smk37_isr_usb(void);
static void smk37_usb_timer_start(void){
    fm1_timer5_start(smk37_isr_usb,4);
    FM1_T5_PRD=3000u; /* OSC/4 = 6 MHz, 2 kHz */
}
static void smk37_usb_timer_stop(void){FM1_T5_CON=0x4000u;
#if SMK37_ENABLE_TRS
    if(smk37_trs_running)smk37_uart_tx_stop();
#endif
}
#endif
extern void smk37_isr_audio(void);
extern void smk37_isr_scanner(void);

static void fm1_fault(const fm1_crash_t *record)
{
    (void)record;
#if FELUCCA_UAC
    smk37_usb_timer_stop();
#endif
    smk37_audio_stop();
    smk37_scan_stop();
    smk37_adc_stop(&smk37_adc);
    /* Watchdog reset, without attempting an unverified flash repair. */
    for (;;) ;
}

#if SMK37_ENABLE_USB_UPDATE
/* Loader overwrites application BSS only after all interrupt/DMA users stop.
 * Code and source bytes remain in XIP until the RAM loader owns execution. */
static void smk37_enter_usb_update(void){
    smk37_bootguard_update(&smk37_boot_state);
    fm1_irq_off();
#if FELUCCA_UAC
    smk37_usb_timer_stop();
#endif
    smk37_audio_stop();smk37_scan_stop();smk37_adc_stop(&smk37_adc);
    smk37_audio_pc_out(6,0);
    usb_detach();fm1_delay_ms(100);
    volatile uint8_t *dst=(volatile uint8_t *)0x01c0a800u;
    for(unsigned i=0;i<sizeof smk37_loader_image;++i)dst[i]=smk37_loader_image[i];
    __asm__ volatile("csync" ::: "memory");
    void (*volatile enter)(void)=(void (*)(void))0x01c0a800u;
    enter();for(;;);
}
#endif
extern uint32_t _bss_start[], _bss_end[], _pool_start[], _pool_end[];
extern uint32_t _data_start[], _data_end[], _data_load[];
extern uint32_t _rt_start[], _rt_end[], _rt_load[];
void smk37_cstart(const uint32_t *boot_parameters)
{
    uint32_t last, remainder = 0, status_at = 0, lights_at = 0;
    uint32_t health_scan = 0, health_audio = 0, health_progress_at = 0;
    uint32_t saved_reset, saved_p33_timeouts;
    uint8_t saved_p3_reset, saved_watchdog;
    /* Shared P33 reset flags: bit 0 is power-on. Snapshot before replacing BSS.
     * Retention and fallback entry are still hardware verification gates. */
    fm1_p33_timeouts = 0; /* P33 helpers run before BSS initialization. */
    fm1_reset_reason();
    saved_reset = fm1_boot.rst_src;
    saved_p3_reset = fm1_boot.p3_rst;
    saved_watchdog = fm1_boot.wdt_con;
    fm1_wdt_arm(0x0d);
    if (smk37_bootguard_begin(&smk37_boot_state, fm1_boot.p3_rst & 1u))
        fm1_enter_uboot();
    smk37_boot_capture(boot_parameters);
    smk37_boot_state.stage = SMK37_BOOT_MEMORY;
    saved_p33_timeouts = fm1_p33_timeouts;
    smk37_memory_zero(_bss_start, _bss_end);
    smk37_memory_zero(_pool_start, _pool_end);
    smk37_memory_copy(_data_start, _data_end, _data_load);
    smk37_memory_copy(_rt_start, _rt_end, _rt_load);
    __asm__ volatile("csync" ::: "memory");
    /* Reset diagnostics live in BSS; preserve the pre-watchdog snapshot. */
    fm1_boot.rst_src = saved_reset;
    fm1_boot.p3_rst = saved_p3_reset;
    fm1_boot.wdt_con = saved_watchdog;
    fm1_p33_timeouts = saved_p33_timeouts;
    smk37_time_init();
    fm1_irq_init();
    fm1_mailbox_clear();
    smk37_synth_init();
    /* Playable startup. Diagnostics remain reachable with the page encoder. */
    smk37_page = 0;
    smk37_boot_state.stage = SMK37_BOOT_USB;
    usb_start();
    smk37_boot_state.stage = SMK37_BOOT_DISPLAY;
    smk37_lcd_hw_init(4);
    /* Failure disables the display transport; USB/audio can still run. */
    smk37_display_ok = (uint8_t)smk37_panel_init(&smk37_panel_io);
    /* One-shot branding; retain normal startup timing and USB identity. */
    draw_text_line(0, (240 - AF_M.h) / 2, 240, &AF_M, "Felucca SMK", 0xffff, 0, 1);
    draw_text_line(0, 240 - AF_S.h - 10, 240, &AF_S,
#if SMK37_REMOTE_PANEL
                   "0.9.27",
#else
                   SMK37_RELEASE_VERSION,
#endif
                   0x9cf3, 0, 1);
    smk37_boot_state.stage = SMK37_BOOT_SCANNER;
    smk37_adc_init(&smk37_adc);
    /* Stock PC10 idle is low/output. Keep it defined between RGB frames. */
    SMK37_LIGHT_REG(0x50080u) &= ~(1u<<10);
    SMK37_LIGHT_REG(0x50088u) &= ~(1u<<10);
    smk37_scan_set_analog(smk37_analog_mask(&smk37_adc.scan));
    smk37_scan_init(fm1_irq_attach, smk37_isr_scanner);
    smk37_boot_state.stage = SMK37_BOOT_AUDIO;
    (void)smk37_audio_init(smk37_dma, HALF_FRAMES * 2u,
                          smk37_isr_audio, 3, &smk37_audio_hooks);
    last = fm1_ticks();
#if FELUCCA_UAC
    smk37_usb_timer_start();
#endif
    fm1_irq_enable_all();
    smk37_boot_state.stage = SMK37_BOOT_RUNNING;
#if SMK37_ENABLE_SAVES
    /* IRQs and audio/scanner are initialized before the read helpers restore
     * interrupts. No auto-save: resume reads only and leaves transport stopped. */
    if(!smk37_storage_probe()){
        smk37_notice=smk37_restore_last?(smk37_session_resume()?"PROJECT SLOTS EMPTY":"PROJECT RESTORED"):"NEW SESSION";
        if(smk37_up_ready)smk37_prefs_restore();
    }else smk37_notice="STORAGE NEEDS RECOVERY";
    smk37_notice_at=fm1_ms;
#endif
    for (;;) {
        uint32_t now = fm1_ticks();
        remainder += now - last;
        last = now;
        while (remainder >= 24000u) { ++fm1_ms; remainder -= 24000u; }
        fm1_wdt_feed();
#if !FELUCCA_UAC
        usb_poll();
#endif
        usb_retry(fm1_ms);
        smk37_editor_service();
#if SMK37_ENABLE_USB_UPDATE
        if(usb.ota_req){usb.ota_req=0;smk37_enter_usb_update();}
#endif
        smk37_project_service();
#if SMK37_ENABLE_TRS
        smk37_trs_configure();
#endif
        smk37_slice_service();
#if SMK37_ENABLE_SAMPLES
        if(smk37_samples_owned)slc_store_poll();
#endif
        if (smk37_scan_frames != health_scan && smk37_rendered != health_audio) {
            health_scan = smk37_scan_frames;
            health_audio = smk37_rendered;
            health_progress_at = fm1_ms;
        }
        if (smk37_boot_state.pending && fm1_ms >= 30000u &&
            health_scan >= 100u && health_audio >= 100u && fm1_ms - health_progress_at < 1000u)
            smk37_bootguard_healthy(&smk37_boot_state);
        if (fm1_ms - lights_at >= 25u) { lights_at=fm1_ms; smk37_light_service(); }
        if (fm1_ms - status_at >= (smk37_animation && fm1_ms-smk37_input_at<400u ? 30u : 250u)) {
            status_at = fm1_ms;
            smk37_status();
        }
        /* Host-requested return uses the mailbox/reset sequence also found
         * in Pro016. It is not a verified forced recovery route. */
        if (usb.uboot_req) {
            smk37_bootguard_update(&smk37_boot_state);
#if FELUCCA_UAC
            smk37_usb_timer_stop();
#endif
            smk37_audio_stop();
            smk37_scan_stop();
            smk37_adc_stop(&smk37_adc);
            usb_detach();
            fm1_delay_ms(30);
            fm1_enter_uboot();
        }
    }
}
