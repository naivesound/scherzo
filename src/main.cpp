#include <RtAudio.h>
#include <RtMidi.h>

#include <fluidsynth.h>

#include <vector>

//#include "synth.h"

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

// void handleMIDI(scherzo::Synth &synth, unsigned char cmd, unsigned char a,
// unsigned char b) {
// int chan = cmd & 0xf;
// cmd = cmd >> 4;
// if (cmd == 0x9 && b > 0) {
// synth.noteOn(chan, a, b);
//} else if ((cmd == 0x9 && b == 0) || cmd == 0x8) {
// synth.noteOff(chan, a);
//} else if (cmd == 0xe) {
// int bend = ((b & 0x7f) << 7) | (a & 0x7f);
// synth.pitchBend(chan, bend);
//} else if (cmd == 0xb) {
// synth.cc(chan, a, b);
//}
//}

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

  midi.openPort();
  midi.setCallback(midiCallback, &scherzo);

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
