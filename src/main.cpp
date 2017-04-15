#include <atomic>
#include <chrono>
#include <map>
#include <string>
#include <thread>
#include <utility>
#include <vector>

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

#include "scherzo.h"

#define SAMPLE_RATE 44100

#define SCHERZO_INCR(scherzo, cc, x)                                           \
  do {                                                                         \
    int _v = scherzo_get_cc((scherzo), (cc)) + (x);                            \
    _v = (_v > 127 ? 127 : (_v < 0 ? 0 : _v));                                 \
    scherzo_midi(scherzo, MIDI_MSG_CC, cc, _v);                                \
  } while (0);

#define KNOB(x) ((x) < 64 ? -1 : 1)

int audioCallback(void *out, void *in, unsigned int frames, double time,
		  RtAudioStreamStatus status, void *arg) {
  (void)in;
  (void)time;
  (void)status;

  scherzo_t *scherzo = static_cast<scherzo_t *>(arg);
  int16_t *buf = static_cast<int16_t *>(out);

  int events = scherzo_write_stereo(scherzo, buf, frames);
  if (events & SCHERZO_EVENT_BEAT) {
    printf("tick\n");
  }
  if (events & SCHERZO_EVENT_LOOP) {
    printf("loop\n");
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

  int bank = 0;
  scherzo_load_instrument(scherzo, bank);
  scherzo_midi(scherzo, MIDI_MSG_CC, MIDI_CC_VOL, 80);
  while (!should_exit) {
    int c = getch();
    switch (c) {
    case ',':
      bank = bank - 1;
      if (bank < 0) {
	bank = 0;
      }
      scherzo_load_instrument(scherzo, bank);
      printf("bank: %d\n", bank);
      break;
    case '.':
      bank = bank + 1;
      scherzo_load_instrument(scherzo, bank);
      printf("bank: %d\n", bank);
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
    case 'b':
      scherzo_midi(scherzo, MIDI_MSG_CC, MIDI_CC_TAP, 1);
      break;
    case 'n':
      scherzo_midi(scherzo, MIDI_MSG_CC, MIDI_CC_LOOP, 1);
      break;
    case 'N':
      scherzo_midi(scherzo, MIDI_MSG_CC, MIDI_CC_CANCEL, 1);
      break;
    case 'q':
      should_exit = true;
      break;
    }
    printf("V:%03d | M:%03d | L:%03d D:%03d | R:%03d C:%03d\n",
	   scherzo_get_cc(scherzo, MIDI_CC_VOL),
	   scherzo_get_cc(scherzo, MIDI_CC_METRONOME_GAIN),
	   scherzo_get_cc(scherzo, MIDI_CC_LOOPER_GAIN),
	   scherzo_get_cc(scherzo, MIDI_CC_LOOPER_DECAY),
	   scherzo_get_cc(scherzo, MIDI_CC_REVERB),
	   scherzo_get_cc(scherzo, MIDI_CC_CHORUS));
  }

  midi_poller.join();
  audio.stopStream();
  scherzo_destroy(scherzo);
  return 0;
}
