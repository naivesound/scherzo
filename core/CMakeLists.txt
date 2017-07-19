cmake_minimum_required(VERSION 3.2)

project(scherzo-core)

add_library(libfluid STATIC 
	fluidsynth/fluid_chan.c
	fluidsynth/fluid_chorus.c
	fluidsynth/fluid_conv.c
	fluidsynth/fluid_defsfont.c
	fluidsynth/fluid_dsp_float.c
	fluidsynth/fluid_gen.c
	fluidsynth/fluid_hash.c
	fluidsynth/fluid_list.c
	fluidsynth/fluid_mod.c
	fluidsynth/fluid_rev.c
	fluidsynth/fluid_settings.c
	fluidsynth/fluid_synth.c
	fluidsynth/fluid_sys.c
	fluidsynth/fluid_tuning.c
	fluidsynth/fluid_voice.c)

add_library(libscherzo STATIC scherzo.c)
set_target_properties(libscherzo PROPERTIES C_STANDARD 99)
target_include_directories(libscherzo PRIVATE fluidsynth)
target_link_libraries(libscherzo libfluid)
