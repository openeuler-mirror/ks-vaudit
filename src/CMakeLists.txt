# 'ks-vaudit' executable

set(CMAKE_AUTOMOC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Threads REQUIRED)
find_package(AVFormat REQUIRED)
find_package(AVCodec REQUIRED)
find_package(AVUtil REQUIRED)
find_package(SWScale REQUIRED)
find_package(X11 REQUIRED)

if(WITH_V4L2)
	find_package(V4L2 REQUIRED)
endif()
if(WITH_ALSA)
	find_package(ALSA REQUIRED)
endif()
if(WITH_PULSEAUDIO)
	find_package(PulseAudio REQUIRED)
endif()
if(WITH_JACK)
	find_package(Jack REQUIRED)
endif()

find_package(Qt5 COMPONENTS Core Gui Widgets X11Extras Multimedia REQUIRED LinguistTools)

pkg_search_module(X11_XCB REQUIRED x11-xcb)
pkg_search_module(XET REQUIRED xext)
pkg_search_module(XCB_SYNC REQUIRED xcb-sync)

IF(HIGH_VERSION)
	ADD_DEFINITIONS(-DHIGH_VERSION)
ENDIF(HIGH_VERSION)

set(sources
	AV/Input/ALSAInput.cpp
	AV/Input/ALSAInput.h
	AV/Input/GLInjectInput.cpp
	AV/Input/GLInjectInput.h
	AV/Input/JACKInput.cpp
	AV/Input/JACKInput.h
	AV/Input/PulseAudioInput.cpp
	AV/Input/PulseAudioInput.h
	AV/Input/SSRVideoStream.h
	AV/Input/SSRVideoStreamReader.cpp
	AV/Input/SSRVideoStreamReader.h
	AV/Input/SSRVideoStreamWatcher.cpp
	AV/Input/SSRVideoStreamWatcher.h
	AV/Input/V4L2Input.cpp
	AV/Input/V4L2Input.h
	AV/Input/X11Input.cpp
	AV/Input/X11Input.h
	AV/Output/NvEncoder.h
	AV/Output/NvEncoder.cpp
	AV/Output/NvEncoderGL.h
	AV/Output/NvEncoderGL.cpp
	AV/Output/AudioEncoder.cpp
	AV/Output/AudioEncoder.h
	AV/Output/BaseEncoder.cpp
	AV/Output/BaseEncoder.h
	AV/Output/Muxer.cpp
	AV/Output/Muxer.h
	AV/Output/OutputManager.cpp
	AV/Output/OutputManager.h
	AV/Output/OutputSettings.h
	AV/Output/SyncDiagram.cpp
	AV/Output/SyncDiagram.h
	AV/Output/Synchronizer.cpp
	AV/Output/Synchronizer.h
	AV/Output/VideoEncoder.cpp
	AV/Output/VideoEncoder.h
	AV/Output/X264Presets.cpp
	AV/Output/X264Presets.h
	AV/AVWrapper.cpp
	AV/AVWrapper.h
	AV/FastResampler.cpp
	AV/FastResampler.h
	AV/FastResampler_FirFilter.h
	AV/FastResampler_FirFilter_Fallback.cpp
	AV/FastScaler.cpp
	AV/FastScaler.h
	AV/FastScaler_Convert.h
	AV/FastScaler_Convert_Fallback.cpp
	AV/FastScaler_Scale.h
	AV/FastScaler_Scale_Fallback.cpp
	AV/FastScaler_Scale_Generic.cpp
	AV/FastScaler_Scale_Generic.h
	AV/SampleCast.h
	AV/SimpleSynth.cpp
	AV/SimpleSynth.h
	AV/SourceSink.cpp
	AV/SourceSink.h
	common/CommandLineOptions.cpp
	common/CommandLineOptions.h
	common/CPUFeatures.cpp
	common/CPUFeatures.h
	common/Dialogs.cpp
	common/Dialogs.h
	common/EnumStrings.h
	common/LockFreeMessageQueue.h
	common/Logger.cpp
	common/Logger.h
	common/MutexDataPair.h
	common/NVidia.cpp
	common/NVidia.h
	common/QueueBuffer.h
	common/ScreenScaling.cpp
	common/ScreenScaling.h
	common/TempBuffer.h
	common/common-definition.h
	kidletime.cpp
	xsyncbasedpoller.cpp
	fixx11h.h
	Recording.cpp
	Recording.h
	Benchmark.cpp
	Benchmark.h
	Global.h
	Main.cpp
)

if(ENABLE_X86_ASM)

	list(APPEND sources
		AV/FastResampler_FirFilter_SSE2.cpp
		AV/FastScaler_Convert_SSSE3.cpp
		AV/FastScaler_Scale_SSSE3.cpp
	)

	set_source_files_properties(
		AV/FastResampler_FirFilter_SSE2.cpp
		PROPERTIES COMPILE_FLAGS -msse2
	)

	set_source_files_properties(
		AV/FastScaler_Convert_SSSE3.cpp
		AV/FastScaler_Scale_SSSE3.cpp
		PROPERTIES COMPILE_FLAGS -mssse3
	)

endif()

set(res_input
	../data/resources/resources.qrc
)

qt5_add_resources(res_output ${res_input})
set(QT_LIBS
	Qt5::Core
	Qt5::Gui
	Qt5::Widgets
	Qt5::X11Extras
	Qt5::Multimedia
)

set_property(SOURCE ${res_output} PROPERTY SKIP_AUTOMOC ON)
find_package(Qt5 COMPONENTS Widgets DBus Core Sql Xml REQUIRED)

set(VAUDITCONFIGURE_DBUS_XML ${CMAKE_CURRENT_SOURCE_DIR}/daemon/configure/data/com.kylinsec.Kiran.VauditConfigure.xml)

set_source_files_properties(${VAUDITCONFIGURE_DBUS_XML}
        PROPERTIES
        CLASSNAME ConfigureInterface
        NO_NAMESPACE true)
qt5_add_dbus_interface(VAUDITCONFIGURE_SRC ${VAUDITCONFIGURE_DBUS_XML} configure_interface)
foreach (interface_item ${VAUDITCONFIGURE_SRC})
    set_property(SOURCE ${interface_item} PROPERTY SKIP_AUTOGEN ON)
endforeach ()

add_executable(ks-vaudit
	${sources}
	${res_output}
	${VAUDITCONFIGURE_SRC}
)

target_include_directories(ks-vaudit PRIVATE
	${AVFORMAT_INCLUDE_DIRS}
	${AVCODEC_INCLUDE_DIRS}
	${AVUTIL_INCLUDE_DIRS}
	${SWSCALE_INCLUDE_DIRS}
	${X11_X11_INCLUDE_PATH}
	${X11_Xext_INCLUDE_PATH}
	${X11_Xfixes_INCLUDE_PATH}
	${X11_Xi_INCLUDE_PATH}
	${X11_Xinerama_INCLUDE_PATH}
	${XET_INCLUDE_DIRS}
	${X11_XCB_INCLUDE_DIRS}
	${XCB_SYNC_INCLUDE_DIRS}
	$<$<BOOL:${WITH_V4L2}>:${V4L2_INCLUDE_DIRS}>
	$<$<BOOL:${WITH_ALSA}>:${ALSA_INCLUDE_DIRS}>
	$<$<BOOL:${WITH_PULSEAUDIO}>:${PULSEAUDIO_INCLUDE_DIRS}>
	$<$<BOOL:${WITH_JACK}>:${JACK_INCLUDE_DIRS}>
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/AV
	${CMAKE_CURRENT_SOURCE_DIR}/AV/Input
	${CMAKE_CURRENT_SOURCE_DIR}/AV/Output
	${CMAKE_CURRENT_SOURCE_DIR}/common
	${CMAKE_CURRENT_SOURCE_DIR}/GUI
	${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(ks-vaudit PRIVATE
	-lcairo
	-lrt
	-lGL
	-lglut
	-ldl
	Qt5::DBus
	Qt5::Core
	klog-qt5
	${AVFORMAT_LIBRARIES}
	${AVCODEC_LIBRARIES}
	${AVUTIL_LIBRARIES}
	${SWSCALE_LIBRARIES}
	${QT_LIBS}
	${CMAKE_THREAD_LIBS_INIT}
	${X11_X11_LIB}
	${X11_Xext_LIB}
	${X11_Xfixes_LIB}
	${X11_Xi_LIB}
	${X11_Xinerama_LIB}
	${XET_LIBRARIES}
	${X11_XCB_LIBRARIES}
	${XCB_SYNC_LIBRARIES}
	$<$<BOOL:${WITH_V4L2}>:${V4L2_LIBRARIES}>
	$<$<BOOL:${WITH_ALSA}>:${ALSA_LIBRARIES}>
	$<$<BOOL:${WITH_PULSEAUDIO}>:${PULSEAUDIO_LIBRARIES}>
	$<$<BOOL:${WITH_JACK}>:${JACK_LIBRARIES}>
)

target_compile_definitions(ks-vaudit PRIVATE
	-DSSR_USE_X86_ASM=$<BOOL:${ENABLE_X86_ASM}>
	-DSSR_USE_FFMPEG_VERSIONS=$<BOOL:${ENABLE_FFMPEG_VERSIONS}>
	-DSSR_USE_JACK_METADATA=$<BOOL:${ENABLE_JACK_METADATA}>
	-DSSR_USE_OPENGL_RECORDING=$<BOOL:${WITH_OPENGL_RECORDING}>
	-DSSR_USE_V4L2=$<BOOL:${WITH_V4L2}>
	-DSSR_USE_ALSA=$<BOOL:${WITH_ALSA}>
	-DSSR_USE_PULSEAUDIO=$<BOOL:${WITH_PULSEAUDIO}>
	-DSSR_USE_JACK=$<BOOL:${WITH_JACK}>
	-DSSR_SYSTEM_DIR="${CMAKE_INSTALL_FULL_DATADIR}/ks-vaudit"
	-DSSR_VERSION="${PROJECT_VERSION}"
)

install(TARGETS ks-vaudit RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})

add_subdirectory(daemon)
add_subdirectory(ui)
