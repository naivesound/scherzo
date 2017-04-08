#include <atomic>
#include <chrono>
#include <map>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <RtAudio.h>
#include <RtMidi.h>

#include <fluidsynth.h>

#include "m.h"

#include "scherzo.h"

#include <termios.h>
#include <unistd.h>

#define SAMPLE_RATE 44100

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

int audioCallback(void *out, void *in, unsigned int frames, double time,
		  RtAudioStreamStatus status, void *arg) {
  (void)in;
  (void)time;
  (void)status;

  scherzo_t *scherzo = static_cast<scherzo_t *>(arg);
  int16_t *buf = static_cast<int16_t *>(out);

  scherzo_write_stereo(scherzo, buf, frames);
  return 0;
}

void midiCallback(double time, std::vector<unsigned char> *msg, void *arg) {
  (void)time;
  (void)arg;

  scherzo_t *scherzo = static_cast<scherzo_t *>(arg);

  if (msg->size() == 3) {
    scherzo_midi(scherzo, msg->at(0), msg->at(1), msg->at(2));
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

  scherzo_t scherzo;

  scherzo_create(&scherzo, SAMPLE_RATE, 64, metronome_acoustic);

  std::thread midi_poller(midi_poll_thread, &scherzo, std::ref(should_exit));

  params.deviceId = audio.getDefaultOutputDevice();
  params.nChannels = 2;
  options.flags = RTAUDIO_ALSA_USE_DEFAULT;

  unsigned int nframes = 256;

  audio.openStream(&params, nullptr, RTAUDIO_SINT16, SAMPLE_RATE, &nframes,
		   &audioCallback, &scherzo, &options);
  audio.startStream();

  int bank = 0;
  int gain = 120;
  int looper_gain = 120;
  int decay = 0;
  scherzo_load_instrument(&scherzo, bank);
  scherzo_set_gain(&scherzo, gain);
#define LIMIT(x) ((x) > 127 ? 127 : ((x) < 0 ? 0 : (x)))

  while (!should_exit) {
    int c = getch();
    switch (c) {
    case ',':
      bank = LIMIT(bank - 1);
      scherzo_load_instrument(&scherzo, bank);
      printf("bank: %d\n", bank);
      break;
    case '.':
      bank = LIMIT(bank + 1);
      scherzo_load_instrument(&scherzo, bank);
      printf("bank: %d\n", bank);
      break;
    case 'v':
      gain = LIMIT(gain - 1);
      printf("gain: %d\n", gain);
      scherzo_set_gain(&scherzo, gain);
      break;
    case 'V':
      gain = LIMIT(gain + 1);
      printf("gain: %d\n", gain);
      scherzo_set_gain(&scherzo, gain);
      break;
    case 'c':
      looper_gain = LIMIT(looper_gain - 1);
      printf("looper_gain: %d\n", looper_gain);
      scherzo_set_looper_gain(&scherzo, looper_gain);
      break;
    case 'C':
      looper_gain = LIMIT(looper_gain + 1);
      printf("looper_gain: %d\n", looper_gain);
      scherzo_set_looper_gain(&scherzo, looper_gain);
      break;
    case 'm':
      decay = LIMIT(decay - 1);
      printf("decay: %d\n", decay);
      scherzo_set_decay(&scherzo, decay);
      break;
    case 'M':
      decay = LIMIT(decay + 1);
      printf("decay: %d\n", decay);
      scherzo_set_decay(&scherzo, decay);
      break;
    case 'b':
      scherzo_tap_bpm(&scherzo);
      break;
    case 'n':
      scherzo_looper(&scherzo, 0);
      printf("looper: %d\n", scherzo.looper.state);
      break;
    case 'N':
      scherzo_looper(&scherzo, 1);
      printf("looper: %d\n", scherzo.looper.state);
      break;
    case 'q':
      should_exit = true;
      break;
    }
  }

  midi_poller.join();
  audio.stopStream();
  return 0;
}
