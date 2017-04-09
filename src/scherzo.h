#ifndef SCHERZO_H
#define SCHERZO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <fluidsynth.h>

enum {
  SCHERZO_EVENT_BEAT, /* next beat started */
  SCHERZO_EVENT_LOOP, /* next loop started */

  SCHERZO_EVENT_NOTE_ON,
  SCHERZO_EVENT_NOTE_OFF,

  SCHERZO_EVENT_INSTRUMENT_UNLOAD,
  SCHERZO_EVENT_INSTRUMENT_LOAD,
};

typedef struct scherzo scherzo_t;

typedef void (*scherzo_notify_fn)(scherzo_t *scherzo, int event, int value,
				  void *context);

scherzo_t *scherzo_create(int sample_rate, int max_polyphony,
			  scherzo_notify_fn fn, void *context);
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
