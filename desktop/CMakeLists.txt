cmake_minimum_required(VERSION 3.2)

project(scherzo-desktop)

add_subdirectory(../core ${CMAKE_CURRENT_BINARY_DIR}/libscherzo_build)

add_executable(scherzo main.cpp vendor/RtAudio.cpp vendor/RtMidi.cpp)

include(FindPkgConfig)

set(LINKLIBS)
if (WIN32)
	add_definitions(-D__WINDOWS_WASAPI__ -D__WINDOWS_MM__)
	set(LINKLIBS ole32 m ksuser winmm ws2_32)
elseif (APPLE)
	add_definitions(-D__MACOSX_CORE__)
	set(LINKLIBS CoreAudio CoreMIDI CoreFoundation)
else()
	find_package(ALSA)
	if(ALSA_FOUND)
		add_definitions(-D__LINUX_ALSA__)
		set(LINKLIBS asound)
	endif(ALSA_FOUND)

	pkg_check_modules(PULSEAUDIO libpulse)
	if(PULSEAUDIO_FOUND)
		add_definitions(-D__LINUX_PULSE__)
		list(APPEND LINKLIBS pulse pulse-simple)
	endif(PULSEAUDIO_FOUND)

	find_package(Threads REQUIRED CMAKE_THREAD_PREFER_PTHREAD)
	list(APPEND LINKLIBS ${CMAKE_THREAD_LIBS_INIT})
endif(WIN32)

set_target_properties(scherzo PROPERTIES CXX_STANDARD 11)
target_include_directories(scherzo PRIVATE vendor ${scherzo-core_SOURCE_DIR})
target_link_libraries(scherzo libscherzo ${LINKLIBS})
