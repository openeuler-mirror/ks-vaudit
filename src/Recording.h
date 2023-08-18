/*
Copyright (c) 2012-2020 Maarten Baert <maarten-baert@hotmail.com>

This file is part of SimpleScreenRecorder.

SimpleScreenRecorder is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

SimpleScreenRecorder is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with SimpleScreenRecorder.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once
#include "Global.h"
#include "Logger.h"
#include "OutputSettings.h"
#include "OutputManager.h"
#include "PulseAudioInput.h"

class MainWindow;
class Muxer;
class VideoEncoder;
class AudioEncoder;
class Synchronizer;
class X11Input;
class PulseAudioInput;
class VideoPreviewer;
class AudioPreviewer;

class Recording : public QObject {
	Q_OBJECT
public:
	enum enum_audio_backend {
#if SSR_USE_ALSA
		AUDIO_BACKEND_ALSA,
#endif
		AUDIO_BACKEND_PULSEAUDIO,
#if SSR_USE_JACK
		AUDIO_BACKEND_JACK,
#endif
		AUDIO_BACKEND_COUNT // must be last
	};

	enum enum_video_area {
		VIDEO_AREA_SCREEN,
		VIDEO_AREA_FIXED,
		VIDEO_AREA_CURSOR,
#if SSR_USE_OPENGL_RECORDING
		VIDEO_AREA_GLINJECT,
#endif
#if SSR_USE_V4L2
		VIDEO_AREA_V4L2,
#endif
		VIDEO_AREA_COUNT // must be last
	};    
    
	enum enum_h264_preset {
		H264_PRESET_ULTRAFAST,
		H264_PRESET_SUPERFAST,
		H264_PRESET_VERYFAST,
		H264_PRESET_FASTER,
		H264_PRESET_FAST,
		H264_PRESET_MEDIUM,
		H264_PRESET_SLOW,
		H264_PRESET_SLOWER,
		H264_PRESET_VERYSLOW,
		H264_PRESET_PLACEBO,
		H264_PRESET_COUNT // must be last
	};

	enum enum_container {
		CONTAINER_MKV,
		CONTAINER_MP4,
		CONTAINER_WEBM,
		CONTAINER_OGG,
		CONTAINER_OTHER,
		CONTAINER_COUNT // must be last
	};

	enum enum_video_codec {
		VIDEO_CODEC_H264,
		VIDEO_CODEC_VP8,
		VIDEO_CODEC_THEORA,
		VIDEO_CODEC_OTHER,
		VIDEO_CODEC_COUNT // must be last
	};

	enum enum_audio_codec {
		AUDIO_CODEC_VORBIS,
		AUDIO_CODEC_MP3,
		AUDIO_CODEC_AAC,
		AUDIO_CODEC_UNCOMPRESSED,
		AUDIO_CODEC_OTHER,
		AUDIO_CODEC_COUNT // must be last
	};

public:
	struct ContainerData {
		QString name, avname;
		QStringList suffixes;
		QString filter;
		std::set<enum_video_codec> supported_video_codecs;
		std::set<enum_audio_codec> supported_audio_codecs;
		inline bool operator<(const ContainerData& other) const { return (avname < other.avname); }
	};

	struct VideoCodecData {
		QString name, avname;
		inline bool operator<(const VideoCodecData& other) const { return (avname < other.avname); }
	};

	struct AudioCodecData {
		QString name, avname;
		inline bool operator<(const AudioCodecData& other) const { return (avname < other.avname); }
	};

private:
	static constexpr int PRIORITY_RECORD = 0, PRIORITY_PREVIEW = -1;

private:
	bool m_page_started, m_input_started, m_output_started;
	bool m_recorded_something, m_wait_saving, m_error_occurred;

	enum_video_area m_video_area;
	bool m_video_area_follow_fullscreen;
	unsigned int m_video_x, m_video_y, m_video_in_width, m_video_in_height;
	unsigned int m_video_frame_rate;
	bool m_video_scaling;

	bool m_audio_enabled; //是否录制音频
	std::vector<PulseAudioInput::Source> m_pulseaudio_sources;
	
	unsigned int m_audio_channels, m_audio_sample_rate;
	enum_audio_backend m_audio_backend;

	QString m_pulseaudio_source;

	OutputSettings m_output_settings;
	std::unique_ptr<OutputManager> m_output_manager;

	QString m_file_base;
	QString m_file_protocol;
	bool m_separate_files, m_add_timestamp;

	std::unique_ptr<X11Input> m_x11_input;
	std::unique_ptr<PulseAudioInput> m_pulseaudio_input;
	QSettings* settings;

public:
	Recording(QSettings* qsettings);
	~Recording();

	// Called when the user tries to close the program. If this function returns true, the command will be blocked.
	// This is used to display a warning if the user is about to close the program during a recording.
	bool TryStartPage();
	void StartPage();
	void StopPage(bool save);
	void StartOutput();
	void StopOutput(bool final);
	void StartInput();
	void StopInput();
	void OnRecordStartPause();
	void OnRecordStart(); //record-start
	void OnRecordPause(); //record-pause
	void OnRecordCancel(bool confirm = true); //record-cancel
	void OnRecordSave(bool confirm = true); //record-save
	QString GetPulseAudioSourceName() {	return QString::fromStdString(m_pulseaudio_sources[0].m_name);}
	static std::vector<QRect> GetScreenGeometries();
	static QRect CombineScreenGeometries(const std::vector<QRect>& screen_geometries);
	static void SaveSettings(QSettings* settings);

private:
	void FinishOutput();
	void UpdateInput();
};
