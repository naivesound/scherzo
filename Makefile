VERSION = 0.0.0

SCHERZO_BIN ?= scherzo

CPPFLAGS = -DVERSION=\"${VERSION}\"
CFLAGS += -Wall -Wextra -pedantic -std=c99 -g
CXXFLAGS += -Wall -Wextra -pedantic -std=c++0x -g

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

INCS = -Isrc/vendor -Isrc/vendor/fluidsynth

OBJS := src/main.o src/scherzo.o \
	src/vendor/RtAudio.o src/vendor/RtMidi.o \

CPPFLAGS += $(INCS)

all: $(SCHERZO_BIN)
	
$(SCHERZO_BIN): $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o $@

lint:
	clang-tidy src/*.cpp -checks='*' -- $(CXXFLAGS)

src/main.o: src/scherzo.h src/m.h

android: src/scherzo.h src/scherzo.c src/m.h
	mkdir -p android/app/src/main/cpp/fluidsynth/fluidsynth
	cp $^ android/app/src/main/cpp
	cp src/vendor/fluidsynth/*.[ch] android/app/src/main/cpp/fluidsynth
	cp src/vendor/fluidsynth/fluidsynth/*.h android/app/src/main/cpp/fluidsynth/fluidsynth
	cd android && ./gradlew build

EMCC_EXPORT := "['_scherzo_create','_scherzo_destroy','_scherzo_write_stereo','_scherzo_midi','_scherzo_load_instrument','_scherzo_tap_bpm','_scherzo_looper']"

web: src/scherzo.h src/m.h
	mkdir -p _tmp/js _tmp/wasm
	docker run --rm -v $(shell pwd):/src naivesound/emcc \
		emcc src/scherzo.c -o _tmp/js/scherzo-asm.js $(INCS) \
		--use-preload-plugins \
		-s ALLOW_MEMORY_GROWTH=1 \
		-s EXPORTED_FUNCTIONS=$(EMCC_EXPORT) -O3
	docker run --rm -v $(shell pwd):/src naivesound/emcc \
		emcc src/scherzo.c -o _tmp/wasm/scherzo.html $(INCS) \
		--use-preload-plugins \
		-s WASM=1 \
		-s ALLOW_MEMORY_GROWTH=1 \
		-s EXPORTED_FUNCTIONS=$(EMCC_EXPORT) -O3
	mv -f _tmp/js/scherzo-asm.js _tmp/js/scherzo-asm.js.mem web
	mv -f _tmp/wasm/scherzo.wasm web
	mv -f _tmp/wasm/scherzo.js web/scherzo-loader.js
	rm -f _tmp/wasm/scherzo.html
	rmdir _tmp/js _tmp/wasm
	rmdir  _tmp

clean:
	rm -f $(OBJS)
	rm -f $(SCHERZO_BIN)

.PHONY: all android lint clean web
