CFLAGS += -Wall -Wextra -pedantic
CXXFLAGS += -std=c++0x -Wall -Wextra -pedantic

CXXFLAGS += -Isrc/vendor -Isrc/vendor/fluidsynth

CXXFLAGS += -D__LINUX_ALSA__
LDFLAGS += -lasound -pthread

CFLAGS += -g
CXXFLAGS += -g
LDFLAGS += -g

OBJS := src/main.o \
	src/vendor/RtAudio.o src/vendor/RtMidi.o \
	src/vendor/fluidsynth/fluid_chan.o \
	src/vendor/fluidsynth/fluid_chorus.o \
	src/vendor/fluidsynth/fluid_conv.o \
	src/vendor/fluidsynth/fluid_defsfont.o \
	src/vendor/fluidsynth/fluid_event.o \
	src/vendor/fluidsynth/fluid_gen.o \
	src/vendor/fluidsynth/fluid_hash.o \
	src/vendor/fluidsynth/fluid_io.o \
	src/vendor/fluidsynth/fluid_list.o \
	src/vendor/fluidsynth/fluid_midi.o \
	src/vendor/fluidsynth/fluid_midi_router.o \
	src/vendor/fluidsynth/fluid_mod.o \
	src/vendor/fluidsynth/fluid_rev.o \
	src/vendor/fluidsynth/fluid_settings.o \
	src/vendor/fluidsynth/fluid_synth.o \
	src/vendor/fluidsynth/fluid_sys.o \
	src/vendor/fluidsynth/fluid_tuning.o \
	src/vendor/fluidsynth/fluid_voice.o \

all: scherzo
	
scherzo: $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o $@

lint:
	clang-tidy src/*.cpp -checks='*' -- $(CXXFLAGS)

src/main.o: src/scherzo.h src/m.h

clean:
	rm -f $(OBJS)
	rm -f scherzo

.PHONY: all lint clean
