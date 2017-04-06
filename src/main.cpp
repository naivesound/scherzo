#include <RtAudio.h>
#include <RtMidi.h>

#include <fluidsynth.h>

#include <vector>

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

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  RtMidiIn midi;
  RtAudio audio;
  RtAudio::StreamOptions options;
  RtAudio::StreamParameters params;

  scherzo_t scherzo;

  scherzo_create(&scherzo, SAMPLE_RATE, 64, metronome_acoustic);

  for (unsigned int i = 0; i < midi.getPortCount(); i++) {
    RtMidiIn *midi_in = new RtMidiIn;
    midi_in->openPort(i);
    midi_in->setCallback(midiCallback, &scherzo);
  }

  params.deviceId = audio.getDefaultOutputDevice();
  params.nChannels = 2;
  options.flags = RTAUDIO_ALSA_USE_DEFAULT;

  unsigned int nframes = 64;

  audio.openStream(&params, nullptr, RTAUDIO_SINT16, SAMPLE_RATE, &nframes,
		   &audioCallback, &scherzo, &options);
  audio.startStream();

  int bank = 0;
  int gain = 120;
  scherzo_load_instrument(&scherzo, bank);
  scherzo_set_gain(&scherzo, gain);
#define LIMIT(x) ((x) > 127 ? 127 : ((x) < 0 ? 0 : (x)))
  for (bool should_exit = false; !should_exit;) {
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
    case 'b':
      scherzo_set_bpm(&scherzo, 0);
      break;
    case 'B':
      scherzo_set_bpm(&scherzo, -1);
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

  midi.closePort();
  audio.stopStream();
  return 0;
}
