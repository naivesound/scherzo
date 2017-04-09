#include <dirent.h>
#include <unistd.h>

#include "scherzo.h"

#include "m.h"

#include <fluidsynth.h>

#include <fluid_chan.c>
#include <fluid_chorus.c>
#include <fluid_conv.c>
#include <fluid_defsfont.c>
#include <fluid_event.c>
#include <fluid_gen.c>
#include <fluid_hash.c>
#include <fluid_io.c>
#include <fluid_list.c>
#include <fluid_midi.c>
#include <fluid_midi_router.c>
#include <fluid_mod.c>
#include <fluid_rev.c>
#include <fluid_settings.c>
#include <fluid_synth.c>
#include <fluid_sys.c>
#include <fluid_tuning.c>
#include <fluid_voice.c>

enum schero_looper_state {
  SCHERZO_LOOPER_STATE_CLEAR,
  SCHERZO_LOOPER_STATE_RECORDING,
  SCHERZO_LOOPER_STATE_OVERDUBBING,
  SCHERZO_LOOPER_STATE_PLAYING,
  SCHERZO_LOOPER_STATE_STOPPED
};

typedef int16_t (*metronome_fn_t)(int sample_rate, int frame);

struct scherzo {
  int sample_rate;

  struct {
    enum schero_looper_state state;

    float volume;
    float decay;
    int pos;
    int frames;
    int16_t *overdub;
    int16_t *loop;
  } looper;

  struct {
    float volume;
    int bpm;
    int frame;
    int index;
    int taps[SCHERZO_BPM_MAX_TAPS];
    metronome_fn_t f;
  } metronome;

  struct {
    fluid_synth_t *synth;
    fluid_settings_t *settings;
    int font;
  } fluid;
};

static int scherzo_sort(const struct dirent **a, const struct dirent **b) {
  return strcoll((*a)->d_name, (*b)->d_name);
}

static int scherzo_scandir(const char *dir, struct dirent ***namelist,
			   int (*select)(const struct dirent *),
			   int (*compar)(const struct dirent **,
					 const struct dirent **)) {
  DIR *dirp;
  struct dirent *ent, *etmp, **nl = NULL, **ntmp;
  int count = 0;
  int allocated = 0;

  if (!(dirp = opendir(dir)))
    return -1;

  int prior_errno = errno;
  errno = 0;

  while ((ent = readdir(dirp))) {
    if (!select || select(ent)) {

      /* Ignore error from readdir/select. See POSIX specs. */
      errno = 0;

      if (count == allocated) {

	if (allocated == 0)
	  allocated = 10;
	else
	  allocated *= 2;

	ntmp = (struct dirent **)realloc(nl, allocated * sizeof *nl);
	if (!ntmp) {
	  errno = ENOMEM;
	  break;
	}
	nl = ntmp;
      }

      if (!(etmp = (struct dirent *)malloc(sizeof *ent))) {
	errno = ENOMEM;
	break;
      }
      *etmp = *ent;
      nl[count++] = etmp;
    }
  }

  if ((prior_errno = errno) != 0) {
    closedir(dirp);
    if (nl) {
      while (count > 0)
	free(nl[--count]);
      free(nl);
    }
    /* Ignore errors from closedir() and what not else. */
    errno = prior_errno;
    return -1;
  }

  closedir(dirp);
  errno = prior_errno;

  qsort(nl, count, sizeof *nl, (int (*)(const void *, const void *))compar);
  if (namelist)
    *namelist = nl;
  return count;
}

#ifdef __ANDROID__
#define SCHERZO_SF2DIR "/mnt/sdcard/sf2"
#else
#define SCHERZO_SF2DIR "./sf2"
#endif

scherzo_t *scherzo_create(int sample_rate, int max_polyphony) {
  scherzo_t *scherzo = (scherzo_t *)malloc(sizeof(*scherzo));
  if (scherzo == NULL) {
    return NULL;
  }
  scherzo->sample_rate = sample_rate;

  scherzo->metronome.bpm = 0;
  scherzo->metronome.f = metronome_acoustic;
  scherzo->metronome.frame = 0;
  scherzo->metronome.index = -1;
  scherzo->metronome.volume = 1;
  memset(scherzo->metronome.taps, 0, sizeof(scherzo->metronome.taps));

  scherzo->looper.state = SCHERZO_LOOPER_STATE_CLEAR;
  scherzo->looper.pos = 0;
  scherzo->looper.frames = 0;
  scherzo->looper.overdub = NULL;
  scherzo->looper.loop = NULL;
  scherzo->looper.decay = 0.5;
  scherzo->looper.volume = 1;

  scherzo->fluid.font = -1;
  scherzo->fluid.settings = new_fluid_settings();
  if (scherzo->fluid.settings == NULL) {
    scherzo_destroy(scherzo);
    return NULL;
  }
  char name_sample_rate[] = "synth.sample-rate";
  char name_polyphony[] = "synth.polyphony";
  fluid_settings_setnum(scherzo->fluid.settings, name_sample_rate, sample_rate);
  fluid_settings_setint(scherzo->fluid.settings, name_polyphony, max_polyphony);
  scherzo->fluid.synth = new_fluid_synth(scherzo->fluid.settings);
  if (scherzo->fluid.synth == NULL) {
    scherzo_destroy(scherzo);
    return NULL;
  }
  return scherzo;
}

int scherzo_load_instrument(scherzo_t *scherzo, int index) {
  if (scherzo->fluid.font > 0) {
    fluid_synth_sfunload(scherzo->fluid.synth, scherzo->fluid.font, 1);
    scherzo->fluid.font = -1;
  }
  if (index >= 0) {
    int r;
    struct dirent **entries;
    r = scherzo_scandir(SCHERZO_SF2DIR, &entries, 0, scherzo_sort);
    if (r < 0) {
      printf("scandir: %d, %d\n", r, errno);
      return -1;
    }
    for (; r > 0; r--, index--) {
      if (index == 0) {
	char path[4096];
	snprintf(path, sizeof(path) - 1, "%s/%s", SCHERZO_SF2DIR,
		 entries[r - 1]->d_name);
	printf("loading sf2 at %s\n", path);
	scherzo->fluid.font = fluid_synth_sfload(scherzo->fluid.synth, path, 1);
      }
      free(entries[r - 1]);
    }
    free(entries);
  }
  return scherzo->fluid.font;
}

void scherzo_destroy(scherzo_t *scherzo) {
  if (scherzo != NULL) {
    scherzo_load_instrument(scherzo, -1);
    if (scherzo->fluid.synth != NULL) {
      delete_fluid_synth(scherzo->fluid.synth);
      scherzo->fluid.synth = NULL;
    }
    if (scherzo->fluid.settings != NULL) {
      delete_fluid_settings(scherzo->fluid.settings);
      scherzo->fluid.settings = NULL;
    }
    free(scherzo);
  }
}

// void fluid_synth_set_reverb(fluid_synth_t* synth, double roomsize, double
// damping, double width, double level)
//
// void fluid_synth_set_chorus(fluid_synth_t* synth, int nr, double level,
// double speed, double depth_ms, int type)
//

static int16_t scherzo_mix(int16_t a, int16_t b) {
  float v = (1.f * ((int)a + (int)b)) / 0x7fff;
  if (v <= -1.25f) {
    v = -0.984375;
  } else if (v >= 1.25f) {
    v = 0.984375;
  } else {
    v = 1.1f * v - 0.2f * v * v * v;
  }
  return (int16_t)(v * 0x7fff);
}

static void scherzo_merge_loop(scherzo_t *scherzo) {
  for (int i = 0; i < scherzo->looper.frames; i++) {
    scherzo->looper.loop[i * 2] = scherzo_mix(scherzo->looper.loop[i * 2],
					      scherzo->looper.overdub[i * 2]);
    scherzo->looper.loop[i * 2 + 1] = scherzo_mix(
	scherzo->looper.loop[i * 2 + 1], scherzo->looper.overdub[i * 2 + 1]);
    scherzo->looper.overdub[i * 2] = 0;
    scherzo->looper.overdub[i * 2 + 1] = 0;
  }
}

void scherzo_looper(scherzo_t *scherzo, int mode) {
  switch (scherzo->looper.state) {
  case SCHERZO_LOOPER_STATE_CLEAR:
    if (mode == 0) {
      scherzo->looper.state = SCHERZO_LOOPER_STATE_RECORDING;
    }
    break;
  case SCHERZO_LOOPER_STATE_STOPPED:
    if (mode == 0) {
      scherzo->looper.state = SCHERZO_LOOPER_STATE_PLAYING;
    } else {
      scherzo->looper.state = SCHERZO_LOOPER_STATE_CLEAR;
      scherzo->looper.frames = 0;
      scherzo->looper.pos = 0;
      free(scherzo->looper.overdub);
      scherzo->looper.overdub = NULL;
      free(scherzo->looper.loop);
      scherzo->looper.loop = NULL;
    }
    break;
  case SCHERZO_LOOPER_STATE_PLAYING:
    if (mode == 0) {
      scherzo->looper.state = SCHERZO_LOOPER_STATE_OVERDUBBING;
    } else {
      scherzo->looper.state = SCHERZO_LOOPER_STATE_STOPPED;
    }
    break;
  case SCHERZO_LOOPER_STATE_RECORDING:
    if (mode == 0) {
      scherzo_merge_loop(scherzo);
      scherzo->looper.state = SCHERZO_LOOPER_STATE_PLAYING;
    } else {
      scherzo->looper.state = SCHERZO_LOOPER_STATE_STOPPED;
      scherzo_looper(scherzo, mode);
    }
    break;
  case SCHERZO_LOOPER_STATE_OVERDUBBING:
    if (mode == 0) {
      scherzo_merge_loop(scherzo);
      scherzo->looper.state = SCHERZO_LOOPER_STATE_PLAYING;
    } else {
      memset(scherzo->looper.overdub, 0,
	     scherzo->looper.frames * 2 * sizeof(int16_t));
      scherzo->looper.state = SCHERZO_LOOPER_STATE_PLAYING;
    }
    break;
  }
}

int scherzo_tap_bpm(scherzo_t *scherzo) {
  int max_interval;
  if (scherzo->metronome.bpm > 0) {
    max_interval = scherzo->sample_rate * 2 * 60 / scherzo->metronome.bpm;
  } else {
    max_interval = scherzo->sample_rate * SCHERZO_BPM_MAX_IDLE_INTERVAL;
  }

  int last = scherzo->metronome.taps[scherzo->metronome.index];
  if (last >= max_interval) {
    scherzo->metronome.bpm = 0;
    scherzo->metronome.index = -1;
    for (int i = 0; i < SCHERZO_BPM_MAX_TAPS; i++) {
      scherzo->metronome.taps[i] = 0;
    }
  }
  if (scherzo->metronome.index < SCHERZO_BPM_MAX_TAPS - 1) {
    scherzo->metronome.index++;
  } else {
    scherzo->metronome.index = SCHERZO_BPM_MAX_TAPS - 1;
    for (int i = 0; i < SCHERZO_BPM_MAX_TAPS - 1; i++) {
      scherzo->metronome.taps[i] = scherzo->metronome.taps[i + 1];
    }
  }

  scherzo->metronome.taps[scherzo->metronome.index] = 0;

  int sum = 0;
  for (int i = 0; i < scherzo->metronome.index; i++) {
    sum = sum + scherzo->metronome.taps[i];
  }
  if (scherzo->metronome.index > 1) {
    int spb = sum / (scherzo->metronome.index);
    scherzo->metronome.bpm = 60 * scherzo->sample_rate / spb;
  }
  return scherzo->metronome.bpm;
}

void scherzo_note_on(scherzo_t *scherzo, int chan, int key, int velocity) {
  if (scherzo->fluid.synth != NULL) {
    fluid_synth_noteon(scherzo->fluid.synth, chan, key, velocity);
  }
}

void scherzo_note_off(scherzo_t *scherzo, int chan, int key) {
  if (scherzo->fluid.synth != NULL) {
    fluid_synth_noteoff(scherzo->fluid.synth, chan, key);
  }
}

void scherzo_pitch_bend(scherzo_t *scherzo, int chan, int bend) {
  if (scherzo->fluid.synth != NULL) {
    fluid_synth_pitch_bend(scherzo->fluid.synth, chan, bend);
  }
}

void scherzo_set_gain(scherzo_t *scherzo, int gain) {
  if (scherzo->fluid.synth != NULL) {
    fluid_synth_set_gain(scherzo->fluid.synth, gain / 127.f);
  }
}

void scherzo_set_looper_gain(scherzo_t *scherzo, int gain) {
  scherzo->looper.volume = gain / 127.f;
}

void scherzo_set_decay(scherzo_t *scherzo, int decay) {
  scherzo->looper.decay = decay / 127.f;
}

void scherzo_cc(scherzo_t *scherzo, int chan, int ctrl, int value) {
  if (scherzo->fluid.synth != NULL) {
    switch (ctrl) {
    case MIDI_CC_BANK_MSB:
      scherzo_load_instrument(scherzo, value);
      break;
    case MIDI_CC_VOL:
      scherzo_set_gain(scherzo, value);
      break;
    default:
      if (scherzo->fluid.synth != NULL) {
	fluid_synth_cc(scherzo->fluid.synth, chan, ctrl, value);
      }
    }
  }
}

int scherzo_midi(scherzo_t *scherzo, int msg, int a, int b) {
  int chan = msg & 0xf;
  int cmd = msg >> 4;
  if (cmd == 0x9 && b > 0) {
    scherzo_note_on(scherzo, chan, a, b);
  } else if ((cmd == 0x9 && b == 0) || cmd == 0x8) {
    scherzo_note_off(scherzo, chan, a);
  } else if (cmd == 0xe) {
    int bend = ((b & 0x7f) << 7) | (a & 0x7f);
    scherzo_pitch_bend(scherzo, chan, bend);
  } else if (cmd == 0xb) {
    scherzo_cc(scherzo, chan, a, b);
  }
  return -1;
}

void scherzo_write_stereo(scherzo_t *scherzo, int16_t *buf, int frames) {
  /* Instrument */
  if (scherzo->fluid.synth != NULL) {
    fluid_synth_write_s16(scherzo->fluid.synth, frames, buf, 0, 2, buf, 1, 2);
  }

  /* Looper */
  switch (scherzo->looper.state) {
  case SCHERZO_LOOPER_STATE_CLEAR:
  case SCHERZO_LOOPER_STATE_STOPPED:
    break;
  case SCHERZO_LOOPER_STATE_RECORDING:
    scherzo->looper.frames = scherzo->looper.frames + frames;
    scherzo->looper.overdub = (int16_t *)realloc(
	scherzo->looper.overdub, sizeof(int16_t) * 2 * scherzo->looper.frames);
    scherzo->looper.loop = (int16_t *)realloc(
	scherzo->looper.loop, sizeof(int16_t) * 2 * scherzo->looper.frames);
    for (int i = 0; i < frames; i++) {
      int j = scherzo->looper.pos + i;
      scherzo->looper.overdub[j * 2] = buf[i * 2];
      scherzo->looper.overdub[j * 2 + 1] = buf[i * 2 + 1];
      scherzo->looper.loop[j * 2] = 0;
      scherzo->looper.loop[j * 2 + 1] = 0;
    }
    scherzo->looper.pos = scherzo->looper.pos + frames;
    break;
  case SCHERZO_LOOPER_STATE_OVERDUBBING:
    for (int i = 0; i < frames; i++) {
      int j = (scherzo->looper.pos + i) % scherzo->looper.frames;
      scherzo->looper.overdub[j * 2] =
	  scherzo_mix(scherzo->looper.overdub[j * 2], buf[i * 2]);
      scherzo->looper.overdub[j * 2 + 1] =
	  scherzo_mix(scherzo->looper.overdub[j * 2 + 1], buf[i * 2 + 1]);
    } // fallthrough
  case SCHERZO_LOOPER_STATE_PLAYING:
    for (int i = 0; i < frames; i++) {
      int j = (scherzo->looper.pos + i) % scherzo->looper.frames;
      buf[i * 2] = scherzo_mix(buf[i * 2], scherzo->looper.overdub[j * 2] +
					       scherzo->looper.loop[j * 2] *
						   scherzo->looper.volume);
      buf[i * 2 + 1] =
	  scherzo_mix(buf[i * 2 + 1], scherzo->looper.overdub[j * 2 + 1] +
					  scherzo->looper.loop[j * 2 + 1] *
					      scherzo->looper.volume);
      scherzo->looper.overdub[j * 2] *= (1 - scherzo->looper.decay);
      scherzo->looper.overdub[j * 2 + 1] *= (1 - scherzo->looper.decay);
    }
    scherzo->looper.pos =
	(scherzo->looper.pos + frames) % scherzo->looper.frames;
    break;
  }

  /* Metronome */
  if (scherzo->metronome.index >= 0) {
    scherzo->metronome.taps[scherzo->metronome.index] += frames;
  }
  if (scherzo->metronome.bpm > 0) {
    float bps = scherzo->metronome.bpm / 60.f;
    int duration = scherzo->sample_rate / bps;
    for (int i = 0; i < frames; i++) {
      int16_t v =
	  scherzo->metronome.f(scherzo->sample_rate, scherzo->metronome.frame);
      scherzo->metronome.frame = (scherzo->metronome.frame + 1) % duration;
      v = v * scherzo->metronome.volume;
      buf[i * 2] = scherzo_mix(buf[i * 2], v);
      buf[i * 2 + 1] = scherzo_mix(buf[i * 2 + 1], v);
    }
  }
}