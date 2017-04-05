cmake_minimum_required(VERSION 3.4.1)
include_directories(src/main/cpp/fluidsynth)
add_library(scherzo-lib SHARED
             src/main/cpp/scherzo-lib.c)
add_library(fluidsynth-lib SHARED
             src/main/cpp/fluidsynth/fluid_chan.c
             src/main/cpp/fluidsynth/fluid_chorus.c
             src/main/cpp/fluidsynth/fluid_conv.c
             src/main/cpp/fluidsynth/fluid_defsfont.c
             src/main/cpp/fluidsynth/fluid_event.c
             src/main/cpp/fluidsynth/fluid_gen.c
             src/main/cpp/fluidsynth/fluid_hash.c
             src/main/cpp/fluidsynth/fluid_io.c
             src/main/cpp/fluidsynth/fluid_list.c
             src/main/cpp/fluidsynth/fluid_midi.c
             src/main/cpp/fluidsynth/fluid_midi_router.c
             src/main/cpp/fluidsynth/fluid_mod.c
             src/main/cpp/fluidsynth/fluid_rev.c
             src/main/cpp/fluidsynth/fluid_settings.c
             src/main/cpp/fluidsynth/fluid_synth.c
             src/main/cpp/fluidsynth/fluid_sys.c
             src/main/cpp/fluidsynth/fluid_tuning.c
             src/main/cpp/fluidsynth/fluid_voice.c)

find_library(log-lib log)
find_library(opensles-lib OpenSLES)
target_link_libraries(scherzo-lib fluidsynth-lib ${log-lib} ${opensles-lib})
