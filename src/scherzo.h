#ifndef SCHERZO_H
#define SCHERZO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <fluidsynth.h>

#define MIDI_MSG_NOTE_ON 0x90
#define MIDI_MSG_NOTE_OFF 0x80
#define MIDI_MSG_CC 0xb0
#define MIDI_MSG_PITCH_BEND 0xe0

#define MIDI_MSG_CLOCK 0xf8
#define MIDI_MSG_START 0xfa
#define MIDI_MSG_STOP 0xfc

#define MIDI_CC_BANK_MSB 0x00 /* Used to select instrument */
#define MIDI_CC_MOD 0x01
#define MIDI_CC_VOL 0x07 /* Synthesizer gain */
#define MIDI_CC_GPC1 0x10
#define MIDI_CC_GPC2 0x11
#define MIDI_CC_GPC3 0x12
#define MIDI_CC_GPC4 0x13
#define MIDI_CC_GPC5 0x50
#define MIDI_CC_GPC6 0x51
#define MIDI_CC_GPC7 0x52
#define MIDI_CC_GPC8 0x53

#define MIDI_CC_SUSTAIN 0x40

#define MIDI_CC_REVERB 91
#define MIDI_CC_CHORUS 93

#define SCHERZO_BPM_MAX_TAPS 8
#define SCHERZO_BPM_MAX_IDLE_INTERVAL 2 /* seconds */

typedef struct scherzo scherzo_t;

scherzo_t *scherzo_create(int sample_rate, int max_polyphony);
void scherzo_destroy(scherzo_t *scherzo);

void scherzo_write_stereo(scherzo_t *scherzo, int16_t *buf, int frames);
int scherzo_midi(scherzo_t *scherzo, int msg, int a, int b);

int scherzo_load_instrument(scherzo_t *scherzo, int index);

int scherzo_tap_bpm(scherzo_t *scherzo);
void scherzo_looper(scherzo_t *scherzo, int mode);
void scherzo_set_gain(scherzo_t *scherzo, int gain);
void scherzo_set_looper_gain(scherzo_t *scherzo, int gain);
void scherzo_set_decay(scherzo_t *scherzo, int decay);

#ifdef __cplusplus
}
#endif

#endif /* SCHERZO_H */
