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

#include "BaseEncoder.h"

#include "Logger.h"
#include "AVWrapper.h"
#include "Muxer.h"

int ParseCodecOptionInt(const QString& key, const QString& value, int min, int max, int multiply) {
	bool parsed;
	int value_int = value.toInt(&parsed);
	if(!parsed) {
		Logger::LogError("[ParseCodecOptionInt] " + Logger::tr("Error: Option '%1' could not be parsed!").arg(key));
		throw LibavException();
	}
	return clamp(value_int, min, max) * multiply;
}
double ParseCodecOptionDouble(const QString& key, const QString& value, double min, double max, double multiply) {
	bool parsed;
	double value_double = value.toDouble(&parsed);
	if(!parsed) {
		Logger::LogError("[ParseCodecOptionDouble] " + Logger::tr("Error: Option '%1' could not be parsed!").arg(key));
		throw LibavException();
	}
	return clamp(value_double, min, max) * multiply;
}

BaseEncoder::BaseEncoder(Muxer* muxer, AVStream* stream, AVCodecContext* codec_context, AVCodec* codec, AVDictionary** options) {

	m_muxer = muxer;
	m_stream = stream;
	m_codec_context = codec_context;
	m_codec_opened = false;

	// initialize shared data
	{
		SharedLock lock(&m_shared_data);
		lock->m_total_frames = 0;
		lock->m_total_packets = 0;
		lock->m_stats_actual_frame_rate = 0.0;
		lock->m_stats_previous_pts = AV_NOPTS_VALUE;
		lock->m_stats_previous_frames = 0;
	}

	// initialize thread signals
	m_should_stop = false;
	m_should_finish = false;
	m_is_done = false;
	m_error_occurred = false;

	m_pts_cnt = 0;
	// 初始化GPU加速相关变量
	m_hw_device_ctx_ref = nullptr;
	m_hw_frames_ctx_ref = nullptr;
	m_avframe_gpu = nullptr;

	try {
		Init(codec, options);
	} catch(...) {
		Free();
		throw;
	}

}

BaseEncoder::~BaseEncoder() {
	Free();
}

// Why can't this be done in the constructor/destructor? The problem is that a real destructor gets called *after* the derived class has already been destructed.
// This means virtual function calls, like EncodeFrame, will fail. This is a problem since the encoder thread doesn't know that the derived class is gone.
// A similar thing can happen with the constructor. To fix this, the derived class should call StartThread() and StopThread() manually.

void BaseEncoder::StartThread() {

	// start encoder thread
	m_thread = std::thread(&BaseEncoder::EncoderThread, this);

}

void BaseEncoder::StopThread() {

	// tell the thread to stop
	if(m_thread.joinable()) {
		Logger::LogInfo("[BaseEncoder::~BaseEncoder] " + Logger::tr("Stopping encoder thread ..."));
		m_should_stop = true;
		m_thread.join();
	}

	// free everything
	Free();

}

double BaseEncoder::GetActualFrameRate() {
	SharedLock lock(&m_shared_data);
	return lock->m_stats_actual_frame_rate;
}

uint64_t BaseEncoder::GetTotalFrames() {
	SharedLock lock(&m_shared_data);
	return lock->m_total_frames;
}

unsigned int BaseEncoder::GetFrameLatency() {
	SharedLock lock(&m_shared_data);
	return (lock->m_total_frames > lock->m_total_packets)? lock->m_total_frames - lock->m_total_packets : 0;
}

unsigned int BaseEncoder::GetQueuedFrameCount() {
	SharedLock lock(&m_shared_data);
	return lock->m_frame_queue.size();
}

unsigned int BaseEncoder::GetQueuedPacketCount() {
	return GetMuxer()->GetQueuedPacketCount(GetStream()->index);
}

void BaseEncoder::AddFrame(std::unique_ptr<AVFrameWrapper> frame) {
	assert(frame->GetFrame()->pts != (int64_t) AV_NOPTS_VALUE);
	SharedLock lock(&m_shared_data);
	++lock->m_total_frames;
	if(lock->m_stats_previous_pts == (int64_t) AV_NOPTS_VALUE) {
		lock->m_stats_previous_pts = frame->GetFrame()->pts;
		lock->m_stats_previous_frames = lock->m_total_frames;
	}
	double timedelta = (double) (frame->GetFrame()->pts - lock->m_stats_previous_pts) * ToDouble(m_codec_context->time_base);
	if(timedelta > 0.999999) {
		lock->m_stats_actual_frame_rate = (double) (lock->m_total_frames - lock->m_stats_previous_frames) / timedelta;
		lock->m_stats_previous_pts = frame->GetFrame()->pts;
		lock->m_stats_previous_frames = lock->m_total_frames;
	}
	lock->m_frame_queue.push_back(std::move(frame));
}

void BaseEncoder::Finish() {
	m_should_finish = true;
}

void BaseEncoder::Stop() {
	m_should_stop = true;
}

void BaseEncoder::IncrementPacketCounter() {
	SharedLock lock(&m_shared_data);
	++lock->m_total_packets;
}

#define RETURN_IF_TRUE(condition) {if (condition) return;}

void BaseEncoder::InitGpuEncode(AVCodec* codec) {

	// only for video
	RETURN_IF_TRUE(GetCodecContext()->width <= 0 || GetCodecContext()->height <= 0)

	if (m_hw_device_ctx_ref) {
		return;
	}

	// 函数已过期，不需要调用
	// avcodec_register_all();

	EncodeType enc_type = EncodeTypeCpu;
	int ret = 0;
	if (strstr(codec->name, "vaapi")) {
		enc_type = EncodeTypeVaapi;
		ret = av_hwdevice_ctx_create(&m_hw_device_ctx_ref, AV_HWDEVICE_TYPE_VAAPI, "/dev/dri/renderD128", nullptr, 0);
	} else if (strstr(codec->name, "qsv")) {
		enc_type = EncodeTypeQsv;
		ret = av_hwdevice_ctx_create(&m_hw_device_ctx_ref, AV_HWDEVICE_TYPE_QSV, "auto", nullptr, 0);
	} else {
		Logger::LogError(QString("[BaseEncoder::InitGpuEncode] invalid codec name: ") + codec->name);
		return;
	}

	if (ret < 0) {
		Logger::LogError(QString("[BaseEncoder::InitGpuEncode] av_hwdevice_ctx_create failed, codec name: ") + codec->name);
		return;
	}
	
	if (enc_type == EncodeTypeVaapi) {
		m_codec_context->pix_fmt = AV_PIX_FMT_VAAPI;

		m_hw_frames_ctx_ref = av_hwframe_ctx_alloc(m_hw_device_ctx_ref);
		if (!m_hw_frames_ctx_ref) {
			Logger::LogError(QString("[BaseEncoder::InitGpuEncode] av_hwframe_ctx_alloc failed, codec name: ") + codec->name);
			return;
		}
		AVHWFramesContext *hw_frames_ctx = (AVHWFramesContext*)m_hw_frames_ctx_ref->data;
		hw_frames_ctx->format    = AV_PIX_FMT_VAAPI;
		hw_frames_ctx->sw_format = AV_PIX_FMT_NV12;
		hw_frames_ctx->initial_pool_size = 0;
		hw_frames_ctx->width     = GetCodecContext()->width;
		hw_frames_ctx->height    = GetCodecContext()->height;

		// vaapi编码时需要做转码，需要初始化 frames_ctx
		ret = av_hwframe_ctx_init(m_hw_frames_ctx_ref);
		if (ret < 0) {
			Logger::LogError(QString("[BaseEncoder::InitGpuEncode] av_hwframe_ctx_init failed, codec name: ") + codec->name);
			return;
		}

		m_codec_context->hw_frames_ctx = av_buffer_ref(m_hw_frames_ctx_ref);
	} else if (enc_type == EncodeTypeQsv) {
		m_codec_context->pix_fmt = AV_PIX_FMT_NV12;
	
		// 将硬件设备关联到编码器
		m_codec_context->hw_device_ctx = av_buffer_ref(m_hw_device_ctx_ref);
	}

}

void BaseEncoder::Init(AVCodec* codec, AVDictionary** options) {

	InitGpuEncode(codec);

	// open codec
	if(avcodec_open2(m_codec_context, codec, options) < 0) {
		Logger::LogError("[BaseEncoder::Init] " + Logger::tr("Error: Can't open codec!"));
		throw LibavException();
	}

	m_codec_opened = true;

	// show a warning for every option that wasn't recognized
	AVDictionaryEntry *t = nullptr;
	while((t = av_dict_get(*options, "", t, AV_DICT_IGNORE_SUFFIX))) {
		Logger::LogWarning("[BaseEncoder::Init] " + Logger::tr("Warning: Codec option '%1' was not recognised!").arg(t->key));
	}

}

void BaseEncoder::Free() {
	if(m_codec_opened) {
		avcodec_close(m_codec_context);
		m_codec_opened = false;
	}
}

void BaseEncoder::EncoderThread() {

	try {

		pid_t tid = gettid();
		Logger::LogInfo("[BaseEncoder::EncoderThread] " + Logger::tr("Encoder thread started. tid: ") + QString::number(tid));

		// normal encoding
		while(!m_should_stop) {

			// get a frame
			std::unique_ptr<AVFrameWrapper> frame;
			{
				SharedLock lock(&m_shared_data);
				if(!lock->m_frame_queue.empty()) {
					frame = std::move(lock->m_frame_queue.front());
					lock->m_frame_queue.pop_front();
				}
			}
			if(!frame) {
				if(m_should_finish) {
					break;
				}
				usleep(20000);
				continue;
			}

			// encode the frame
			EncodeFrame(frame.get());

		}

		// flush the encoder
		if(!m_should_stop && (m_codec_context->codec->capabilities & AV_CODEC_CAP_DELAY)) {
			Logger::LogInfo("[BaseEncoder::EncoderThread] " + Logger::tr("Flushing encoder ..."));
			while(!m_should_stop) {
				if(!EncodeFrame(nullptr)) {
					break;
				}
			}
		}

		// tell the others that we're done
		m_is_done = true;

		Logger::LogInfo("[BaseEncoder::EncoderThread] " + Logger::tr("Encoder thread stopped."));

	} catch(const std::exception& e) {
		m_error_occurred = true;
		Logger::LogError("[BaseEncoder::EncoderThread] " + Logger::tr("Exception '%1' in encoder thread.").arg(e.what()));
	} catch(...) {
		m_error_occurred = true;
		Logger::LogError("[BaseEncoder::EncoderThread] " + Logger::tr("Unknown exception in encoder thread."));
	}

	// always end the stream, even if there was an error, otherwise the muxer will wait forever
	m_muxer->EndStream(m_stream->index);

}
