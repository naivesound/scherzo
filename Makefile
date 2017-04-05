VERSION = 0.0.0

SCHERZO_BIN ?= scherzo

CPPFLAGS = -DVERSION=\"${VERSION}\"
CFLAGS += -Wall -Wextra -pedantic -std=c99
CXXFLAGS += -Wall -Wextra -pedantic -std=c++0x

ifeq ($(alsa),1)
	CXXFLAGS += -D__LINUX_ALSA__
	LDFLAGS += -lasound -pthread
endif
ifeq ($(pulse),1)
	CXXFLAGS += -D__LINUX_PULSE__
	LDFLAGS += -lpulse -lpulse-simple -pthread
endif
ifeq ($(jack),1)
	CXXFLAGS += -D__UNIX_JACK__
	LDFLAGS += -ljack -pthread
endif

ifeq ($(macos),1)
	CXXFLAGS += -D__MACOSX_CORE__
	LDFLAGS += -framework CoreAudio -framework CoreMIDI -framework CoreFoundation
endif
ifeq ($(windows),1)
	CXXFLAGS += -D__WINDOWS_WASAPI__ -D__WINDOWS_MM__
	LDFLAGS += -lole32 -lm -lksuser -lwinmm -lws2_32 -mwindows -static
endif

CXXFLAGS += -Isrc/vendor -Isrc/vendor/fluidsynth

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

all: $(SCHERZO_BIN)
	
$(SCHERZO_BIN): $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o $@

lint:
	clang-tidy src/*.cpp -checks='*' -- $(CXXFLAGS)

src/main.o: src/scherzo.h src/m.h

android: src/scherzo.h src/m.h
	mkdir -p android/app/src/main/cpp/fluidsynth
	cp $^ android/app/src/main/cpp
	cp src/vendor/fluidsynth/*.[ch] android/app/src/main/cpp/fluidsynth
	cd android && ./gradlew build

clean:
	rm -f $(OBJS)
	rm -f $(SCHERZO_BIN)

.PHONY: all android lint clean
