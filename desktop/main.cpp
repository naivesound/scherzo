#include <atomic>
#include <chrono>
#include <cstring>
#include <map>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <dirent.h>
#include <errno.h>

#include <RtAudio.h>
#include <RtMidi.h>

#if !defined(_GLIBCXX_HAS_GTHREADS) && !defined(__APPLE__)
#include "vendor/mingw.mutex.h"
#include "vendor/mingw.thread.h"
#endif

#ifndef __WIN32
#include <termios.h>
#include <unistd.h>
static int getch() {
  struct termios oldattr, newattr;
  int ch;
  tcgetattr(STDIN_FILENO, &oldattr);
  newattr = oldattr;
  newattr.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newattr);
  ch = getchar();
  tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);
  return ch;
}
#else
#include <conio.h>
#endif

#include <scherzo.h>

#define SAMPLE_RATE 44100

#define SCHERZO_INCR(scherzo, cc, x)                                           \
  do {                                                                         \
    int _v = scherzo_get_cc((scherzo), (cc)) + (x);                            \
    _v = (_v > 127 ? 127 : (_v < 0 ? 0 : _v));                                 \
    scherzo_midi(scherzo, MIDI_MSG_CC, cc, _v);                                \
  } while (0);

#define KNOB(x) ((x) < 64 ? -1 : 1)

static int scherzo_sort(const struct dirent **a, const struct dirent **b) {
  return -strcoll((*a)->d_name, (*b)->d_name);
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

static int load_instruments(scherzo_t *scherzo, const char *dir) {
  int r;
  struct dirent **entries;
  r = scherzo_scandir(dir, &entries, 0, scherzo_sort);
  if (r < 0) {
    printf("scandir: %d, %d\n", r, errno);
    return -1;
  }
  int i = 0;
  for (; r > 0; r--) {
    if (strcmp(entries[r - 1]->d_name, ".") == 0 ||
	strcmp(entries[r - 1]->d_name, "..") == 0) {
      free(entries[r - 1]);
      continue;
    }
    char path[4096];
    snprintf(path, sizeof(path) - 1, "%s/%s", dir, entries[r - 1]->d_name);
    scherzo_set_instrument_path(scherzo, i, path);
    i++;
    free(entries[r - 1]);
  }
  free(entries);
}

int audioCallback(void *out, void *in, unsigned int frames, double time,
		  RtAudioStreamStatus status, void *arg) {
  (void)in;
  (void)time;
  (void)status;

  scherzo_t *scherzo = static_cast<scherzo_t *>(arg);
  int16_t *buf = static_cast<int16_t *>(out);

  static int events;
  static int notes_buf[129];
  static int every = 4;
  static int counter = 0;

  int e = scherzo_write_stereo(scherzo, buf, frames);
  events = events | e;

  if (counter == 0) {
    for (int i = 0; i < 128; i++) {
      notes_buf[i] = 0;
    }
  }

  for (int i = 0; i < 128; i++) {
    int vol = scherzo_get_note(scherzo, i);
    if (vol > 0) {
      notes_buf[i] = vol;
    }
  }
  if (counter++ == every) {
    char *fill = " ";
    if (events & SCHERZO_EVENT_LOOP) {
      fill = "\x1b[31m=";
    }
    if (events & SCHERZO_EVENT_BEAT) {
      fill = "\x1b[31m.";
    }
    for (int i = 20; i < 128 - 20; i++) {
      char *vol = "█";
      if (notes_buf[i] == 0) {
	vol = fill;
      }
      char *color = "\x1b[37m";
      int note = i % 12;
      if (note == 0) {
	// printf("\x1b[31m·");
	printf("\x1b[31m.");
      }
      if (note == 1 || note == 3 || note == 6 || note == 8 || note == 10) {
	color = "\x1b[31m";
      }
      printf("%s%s", color, vol);
    }
    printf("\x1b[0m\n");
    counter = 0;
    events = 0;
  }
  return 0;
}

void midiCallback(double time, std::vector<unsigned char> *msg, void *arg) {
  (void)time;
  (void)arg;

  scherzo_t *scherzo = static_cast<scherzo_t *>(arg);

  if (msg->size() == 3) {
    int cmd = msg->at(0);
    int a = msg->at(1);
    int b = msg->at(2);
    if ((cmd & 0xf0) == MIDI_MSG_CC) {
      switch (a) {
      case 0x10:
	SCHERZO_INCR(scherzo, MIDI_CC_LOOPER_GAIN, KNOB(b));
	break;
      case 0x1c:
	SCHERZO_INCR(scherzo, MIDI_CC_LOOPER_DECAY, KNOB(b));
	break;
      default:
	scherzo_midi(scherzo, cmd, a, b);
      }
    } else if ((cmd & 0xf0) == MIDI_MSG_NOTE_ON) {
      if (a >= 0 && a < 8) {
	scherzo_midi(scherzo, MIDI_MSG_CC, MIDI_CC_LOOP, b);
      } else if (a >= 8 && a < 16) {
	scherzo_midi(scherzo, MIDI_MSG_CC, MIDI_CC_CANCEL, b);
      } else if (a >= 16 && a < 24) {
	scherzo_midi(scherzo, MIDI_MSG_CC, MIDI_CC_TAP, b);
      } else {
	scherzo_midi(scherzo, cmd, a, b);
      }
    }
    if ((cmd & 0xf0) == MIDI_MSG_NOTE_OFF) {
      scherzo_midi(scherzo, cmd, a, b);
    }
    if ((cmd & 0xf0) == MIDI_MSG_PITCH_BEND) {
      scherzo_midi(scherzo, cmd, a, b);
    }
  }
}

static void midi_poll_thread(scherzo_t *scherzo,
			     std::atomic<bool> &should_exit) {
  std::map<std::pair<unsigned int, std::string>, RtMidiIn *> ports;
  while (!should_exit) {
    RtMidiIn midi_inputs;
    for (auto it = ports.begin(); it != ports.end();) {
      auto k = it->first;
      if (k.first >= midi_inputs.getPortCount() ||
	  k.second != midi_inputs.getPortName(k.first)) {
	std::cout << "  close " << k.first << " " << k.second << std::endl;
	it->second->closePort();
	it = ports.erase(it);
      } else {
	++it;
      }
    }
    for (unsigned int i = 0; i < midi_inputs.getPortCount(); i++) {
      std::pair<int, std::string> k(i, midi_inputs.getPortName(i));
      if (ports.find(k) == ports.end()) {
	std::cout << "  open " << k.first << " " << k.second << std::endl;
	RtMidiIn *midi_in = new RtMidiIn();
	midi_in->openPort(i);
	midi_in->setCallback(midiCallback, scherzo);
	ports[k] = midi_in;
      }
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  for (auto &kv : ports) {
    kv.second->closePort();
  }
}

static inline const char *sf2dir() {
  const char *dir = getenv("SF2DIR");
  if (dir == NULL) {
    return "./sf2";
  }
  return dir;
}

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  std::atomic<bool> should_exit(false);
  RtAudio audio;
  RtAudio::StreamOptions options;
  RtAudio::StreamParameters params;

  scherzo_t *scherzo = scherzo_create(SAMPLE_RATE, 64);

  std::thread midi_poller(midi_poll_thread, scherzo, std::ref(should_exit));

  params.deviceId = audio.getDefaultOutputDevice();
  params.nChannels = 2;
  options.flags = RTAUDIO_ALSA_USE_DEFAULT;

  unsigned int nframes = 512;

  audio.openStream(&params, nullptr, RTAUDIO_SINT16, SAMPLE_RATE, &nframes,
		   &audioCallback, scherzo, &options);
  audio.startStream();

  int program = 0;
  load_instruments(scherzo, sf2dir());
  scherzo_load_instrument(scherzo, program);

  scherzo_midi(scherzo, MIDI_MSG_CC, MIDI_CC_VOL, 80);
  while (!should_exit) {
    int c = getch();
    switch (c) {
    case ',':
      program = program - 1;
      if (program < 0) {
	program = 0;
      }
      scherzo_load_instrument(scherzo, program);
      printf("instrument program: %d\n", program);
      break;
    case '.':
      program = program + 1;
      scherzo_load_instrument(scherzo, program);
      printf("instrument program: %d\n", program);
      break;
    case 'v':
      SCHERZO_INCR(scherzo, MIDI_CC_VOL, -1);
      printf("gain: %d\n", scherzo_get_cc(scherzo, MIDI_CC_VOL));
      break;
    case 'V':
      SCHERZO_INCR(scherzo, MIDI_CC_VOL, 1);
      printf("gain: %d\n", scherzo_get_cc(scherzo, MIDI_CC_VOL));
      break;
    case 'c':
      SCHERZO_INCR(scherzo, MIDI_CC_LOOPER_GAIN, -1);
      printf("looper gain: %d\n", scherzo_get_cc(scherzo, MIDI_CC_LOOPER_GAIN));
      break;
    case 'C':
      SCHERZO_INCR(scherzo, MIDI_CC_LOOPER_GAIN, 1);
      printf("looper gain: %d\n", scherzo_get_cc(scherzo, MIDI_CC_LOOPER_GAIN));
      break;
    case 'm':
      SCHERZO_INCR(scherzo, MIDI_CC_LOOPER_DECAY, -1);
      printf("looper decay: %d\n",
	     scherzo_get_cc(scherzo, MIDI_CC_LOOPER_DECAY));
      break;
    case 'M':
      SCHERZO_INCR(scherzo, MIDI_CC_LOOPER_DECAY, -1);
      printf("looper decay: %d\n",
	     scherzo_get_cc(scherzo, MIDI_CC_LOOPER_DECAY));
      break;
    case ' ':
      scherzo_midi(scherzo, MIDI_MSG_CC, MIDI_CC_TAP, 1);
      break;
    case '\n':
      scherzo_midi(scherzo, MIDI_MSG_CC, MIDI_CC_LOOP, 1);
      break;
    case '\b': // BS or DEL
    case 127:
      scherzo_midi(scherzo, MIDI_MSG_CC, MIDI_CC_CANCEL, 1);
      break;
    case 'q':
      should_exit = true;
      break;
    }
#if 0
    printf("V:%03d | M:%03d | L:%03d D:%03d | R:%03d C:%03d\n",
	   scherzo_get_cc(scherzo, MIDI_CC_VOL),
	   scherzo_get_cc(scherzo, MIDI_CC_METRONOME_GAIN),
	   scherzo_get_cc(scherzo, MIDI_CC_LOOPER_GAIN),
	   scherzo_get_cc(scherzo, MIDI_CC_LOOPER_DECAY),
	   scherzo_get_cc(scherzo, MIDI_CC_REVERB),
	   scherzo_get_cc(scherzo, MIDI_CC_CHORUS));
#endif
  }

  midi_poller.join();
  audio.stopStream();
  scherzo_destroy(scherzo);
  return 0;
}
