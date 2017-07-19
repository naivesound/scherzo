#ifndef SCHERZO_H
#define SCHERZO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define MIDI_MSG_NOTE_ON 0x90
#define MIDI_MSG_NOTE_OFF 0x80
#define MIDI_MSG_CC 0xb0
#define MIDI_MSG_PITCH_BEND 0xe0

#define MIDI_CC_BANK_MSB 0x00       /* Select instrument */
#define MIDI_CC_VOL 0x07	    /* Master gain */
#define MIDI_CC_LOOPER_DECAY 0x9    /* Looper decay */
#define MIDI_CC_LOOPER_GAIN 0x0c    /* Looper gain */
#define MIDI_CC_METRONOME_GAIN 0x0d /* Metronome gain */

#define MIDI_CC_TAP 0x50    /* Metronome tap button >=0 */
#define MIDI_CC_LOOP 0x51   /* Looper rec/play/overdub button >=0 */
#define MIDI_CC_CANCEL 0x52 /* Looper undo/stop/erase button >=0 */

#define MIDI_CC_REVERB 0x5b /* Reverb amount 0..127 */
#define MIDI_CC_CHORUS 0x5d /* Chorus amount 0..127 */

#define MIDI_CC_SUSTAIN 0x40 /* Sustain pedal */

#define MIDI_CC_GPC1 0x10
#define MIDI_CC_GPC2 0x11
#define MIDI_CC_GPC3 0x12
#define MIDI_CC_GPC4 0x13
#define MIDI_CC_GPC5 0x50
#define MIDI_CC_GPC6 0x51
#define MIDI_CC_GPC7 0x52
#define MIDI_CC_GPC8 0x53

#define SCHERZO_EVENT_BEAT (1 << 1)
#define SCHERZO_EVENT_LOOP (1 << 2)
#define SCHERZO_EVENT_NOTE (1 << 3)
#define SCHERZO_EVENT_CC (1 << 4)

typedef struct scherzo scherzo_t;

scherzo_t *scherzo_create(int sample_rate, int max_polyphony);
void scherzo_destroy(scherzo_t *scherzo);

int scherzo_write_stereo(scherzo_t *scherzo, int16_t *buf, int frames);
int scherzo_midi(scherzo_t *scherzo, int msg, int a, int b);

int scherzo_get_note(scherzo_t *scherzo_t, int note);
int scherzo_get_cc(scherzo_t *scherzo_t, int cc);

int scherzo_load_instrument(scherzo_t *scherzo, const char *dir, int index);

#ifdef __cplusplus
}
#endif

#endif /* SCHERZO_H */
