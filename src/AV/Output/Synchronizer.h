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

#include "SourceSink.h"
#include "MutexDataPair.h"
#include "FastScaler.h"
#include "FastResampler.h"
#include "QueueBuffer.h"
#include "TempBuffer.h"
#include "AVWrapper.h"

#include "SampleCast.h"
#include "NvEncoderGL.h"

typedef enum recode_audio_type {
	RECORD_AUDIO_MIC,
	RECORD_AUDIO_SPEAKER,
	RECORD_AUDIO_ALL,
	RECORD_AUDIO_INVALID
} RECORD_AUDIO_TYPE;

class OutputManager;
class OutputSettings;
class OutputFormat;
class SyncDiagram;

class Synchronizer : public VideoSink, public AudioSink, public AudioSinkInput {

private:
	struct VideoData {

		FastScaler m_fast_scaler;
		int m_nvenc_inited;
		NvEncoderGL *m_nvenc;

		int m_image_cnt;
		int m_scale_cnt;
		int m_unchanged_cnt;

		int64_t m_last_timestamp; // the timestamp of the last received video frame (for gap detection)
		int64_t m_next_timestamp; // the preferred timestamp of the next frame (for rate control)

	};
	struct ChannelData {
		float m_current_peak, m_current_rms;
		float m_next_peak, m_next_rms;
		inline ChannelData() { m_current_peak = m_current_rms = m_next_peak = m_next_rms = 0.0f; }
		template<typename IN>
		inline void Analyze(IN sample) {
			float val = fabs(SampleCast<IN, float>(sample));
			m_next_peak = fmax(m_next_peak, val);
			m_next_rms += val * val;
		}
	};
	struct AudioData {

		std::unique_ptr<FastResampler> m_fast_resampler;
		TempBuffer<float> m_temp_input_buffer;
		TempBuffer<float> m_temp_output_buffer;

		int64_t m_filtered_timestamp; // the timestamp after noise filtering
		int64_t m_last_timestamp; // the timestamp of the last received audio frame (to detect non-monotonic timestamps)
		int64_t m_first_timestamp; // the timestamp of the first audio frame in the current segment (for drift correction)
		int64_t m_samples_written; // total number of samples written to the queue in the current segment (for drift correction)
		double m_average_drift; // drift averaged over time (for drift correction)
		bool m_drop_samples, m_insert_samples;

		bool m_warn_desync;

	};
	struct SharedData {

		TempBuffer<float> m_partial_audio_frame;
		unsigned int m_partial_audio_frame_samples;

		std::deque<std::unique_ptr<AVFrameWrapper> > m_video_buffer;
		QueueBuffer<float> m_audio_buffer_input;
		QueueBuffer<float> m_audio_buffer_output;
		int64_t m_video_pts, m_audio_samples; // video and audio position in the final stream (encoded frames and samples, including the partial audio frame)
		int64_t m_time_offset; // the length of all previous segments combined (in microseconds)

		bool m_segment_video_started;
		bool m_segment_audio_started_input, m_segment_audio_started_output; // whether video and audio have started (always true if the corresponding stream is disabled)
		int64_t m_segment_video_start_time;
		int64_t m_segment_audio_start_time_input, m_segment_audio_start_time_output; // the start time of video and audio (real-time, in microseconds)
		int64_t m_segment_video_stop_time;
		int64_t m_segment_audio_stop_time_input, m_segment_audio_stop_time_output; // the stop time of video and audio (real-time, in microseconds)
		bool m_segment_audio_can_drop; // whether audio samples can still be dropped (i.e. no samples have been sent to the encoder yet)
		int64_t m_segment_audio_samples_read; // the number of samples that have been read from the audio buffer (including dropped samples)
		int64_t m_segment_video_accumulated_delay; // sum of all video frame delays that were applied so far

		std::shared_ptr<AVFrameData> m_last_video_frame_data;
		// check if last read frame is encoded by nvenc
		int m_last_video_frame_encoded;
		// std::shared_ptr<std::vector<std::vector<uint8_t> > > m_last_video_frame_vpacket;;
		size_t m_last_video_frame_size;

		// to check if image changed
		std::shared_ptr<AVFrameData> m_last_video_read_data;
		// check if last read frame is encoded by nvenc
		int m_last_video_read_encoded;
		std::shared_ptr<std::vector<std::vector<uint8_t> > > m_last_video_read_vpacket;;
		size_t m_last_video_read_size;

		std::vector<ChannelData> m_channel_data;
		int m_has_voice;

		bool m_warn_drop_video;

	};
	typedef MutexDataPair<VideoData>::Lock VideoLock;
	typedef MutexDataPair<AudioData>::Lock AudioLock;
	typedef MutexDataPair<SharedData>::Lock SharedLock;

private:
	static const int64_t AUDIO_TIMESTAMP_FILTER;
	static const double DRIFT_CORRECTION_P, DRIFT_CORRECTION_I;
	static const double DRIFT_ERROR_THRESHOLD, DRIFT_MAX_BLOCK;
	static const size_t MAX_VIDEO_FRAMES_BUFFERED, MAX_AUDIO_SAMPLES_BUFFERED;
	static const int64_t MAX_FRAME_DELAY;

private:
	OutputManager *m_output_manager;
	const OutputSettings *m_output_settings;
	const OutputFormat *m_output_format;

	int64_t m_max_frames_skipped;
	std::unique_ptr<SyncDiagram> m_sync_diagram;

	std::thread m_thread;
	MutexDataPair<VideoData> m_video_data;
	MutexDataPair<AudioData> m_audio_data_input;
	MutexDataPair<AudioData> m_audio_data_output;
	int m_audio_input_received;
	int m_audio_output_received;
	MutexDataPair<SharedData> m_shared_data;
	std::atomic<bool> m_should_stop, m_error_occurred;

public:
	// The arguments 'video_encoder' and 'audio_encoder' can be NULL to disable video or audio.
	Synchronizer(OutputManager* output_manager);
	~Synchronizer();

private:
	//void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
	int EncodeFrameByNvenc(VideoLock &videolock, const uint8_t* data, std::unique_ptr<AVFrameWrapper> &converted_frame);
	void InitNvenc(VideoLock &videolock);
	void Init();
	void Free();

public:

	// This function tells the synchronizer to end the current segment and reset the synchronization system
	// in preparation for a new segment. This is required for pausing and continuing a recording.
	// This function has no effect if there are no frames in the current segment, so it is safe to call this multiple times.
	// This function is thread-safe, but for best results you should still make sure that no input is running
	// while this function is called, because otherwise frames may end up in the wrong segment.
	void NewSegment();

	// Returns the total recording time (in microseconds).
	// This function is thread-safe.
	int64_t GetTotalTime();

	// Returns whether an error has occurred in the synchronizer thread.
	// This function is thread-safe.
	inline bool HasErrorOccurred() { return m_error_occurred; }

	//inline VideoEncoder* GetVideoEncoder() { return m_video_encoder; }
	//inline AudioEncoder* GetAudioEncoder() { return m_audio_encoder; }

public: // internal
	virtual int64_t GetNextVideoTimestamp() override;
	virtual void ReadVideoFrame(unsigned int width, unsigned int height, const uint8_t* data, int stride, AVPixelFormat format, int colorspace, int64_t timestamp, int changed) override;
	virtual void ReadVideoPing(int64_t timestamp) override;
	void ReadAudioSamplesReal(unsigned int channels, unsigned int sample_rate, AVSampleFormat format, unsigned int sample_count, const uint8_t* data, int64_t timestamp, QString &type);
	void ReadAudioHoleReal(QString &type);
	virtual void ReadAudioSamples(unsigned int channels, unsigned int sample_rate, AVSampleFormat format, unsigned int sample_count, const uint8_t* data, int64_t timestamp) override;
	virtual void ReadAudioHole() override;
	virtual void ReadAudioSamplesInput(unsigned int channels, unsigned int sample_rate, AVSampleFormat format, unsigned int sample_count, const uint8_t* data, int64_t timestamp) override;
	virtual void ReadAudioHoleInput() override;

private:
	void InitAudioSegment(AudioData* audiolock);
	double GetAudioDrift(AudioData* audiolock, unsigned int extra_samples = 0);
	void NewSegment(SharedData* lock);
	void InitSegment(SharedData* lock);
	int64_t GetTotalTime(SharedData* lock);
	void GetSegmentStartStop(SharedData* lock, int64_t* segment_start_time, int64_t* segment_stop_time);
	void FlushBuffers(SharedData* lock);
	void FlushVideoBuffer(SharedData* lock, int64_t segment_start_time, int64_t segment_stop_time);
	void FlushAudioBuffer(SharedData* lock, int64_t segment_start_time, int64_t segment_stop_time);

private:
	void SynchronizerThread();

};
