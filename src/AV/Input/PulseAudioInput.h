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

#if SSR_USE_PULSEAUDIO

#include "SourceSink.h"
#include "MutexDataPair.h"
#include "CommandLineOptions.h"

#include <pulse/mainloop.h>
#include <pulse/context.h>
#include <pulse/introspect.h>
#include <pulse/stream.h>
#include <pulse/error.h>
#include<pulse/volume.h>

class PulseAudioInput : public AudioSourceInput, public AudioSource {

public:
	struct Source {
		std::string m_name, m_description;
		inline Source(const std::string& name, const std::string& description) : m_name(name), m_description(description) {}
	};

private:
	static const int64_t START_DELAY;

private:
	QString m_source_name;
	unsigned int m_sample_rate, m_channels;

	pa_mainloop *m_pa_mainloop;
	pa_context *m_pa_context;
	pa_stream *m_pa_stream;
	pa_cvolume m_volume; //调整音量
	int m_last_mic_volume; //记录上一次的麦克风音量
	int m_last_speaker_volume; //记录上一次的麦克风音量
	unsigned int m_pa_period_size;

	bool m_stream_is_monitor;
	bool m_stream_suspended, m_stream_moved;

	QString m_record_audio_type;

	std::thread m_thread;
	std::atomic<bool> m_should_stop, m_error_occurred;

public:
	PulseAudioInput(const QString& source_name, unsigned int sample_rate, QString m_record_audio_type);
	~PulseAudioInput();

	// Returns whether an error has occurred in the input thread.
	// This function is thread-safe.
	inline bool HasErrorOccurred() { return m_error_occurred; }
public:
	static std::vector<Source> GetSourceList();

private:
	void Init();
	void Free();

	void DetectMonitor();
	void UpdateSourceVolume(pa_volume_t volume); //音量设置

	static void SourceInfoCallback(pa_context* context, const pa_source_info* info, int eol, void* userdata);
	static void SetSourceVolumeCallback(pa_context *c, int success, void *userdata);
	static void SuspendedCallback(pa_stream* stream, void* userdata);
	static void MovedCallback(pa_stream* stream, void* userdata);

	void InputThread();

};

#endif
