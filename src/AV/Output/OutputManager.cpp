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

#include "OutputManager.h"
#include "X264Presets.h"

#include "Logger.h"

const size_t OutputManager::THROTTLE_THRESHOLD_FRAMES = 20;
const size_t OutputManager::THROTTLE_THRESHOLD_PACKETS = 100;

static QString GetNewFragmentFile(const QString& file, unsigned int fragment_number) {
	QFileInfo fi(file);
	QString newfile;
	newfile = fi.path() + "/" + fi.completeBaseName() + QString("-%1").arg(fragment_number, 8, 10, QLatin1Char('0'));
	if(!fi.suffix().isEmpty())
		newfile += "." + fi.suffix();
	return newfile;
}

OutputManager::OutputManager(const OutputSettings& output_settings) {

	m_output_settings = output_settings;

	m_fragmented = false;
	m_fragment_length = 5;

	// initialize shared data
	{
		SharedLock lock(&m_shared_data);
		lock->m_fragment_number = 0;
		lock->m_video_encoder = NULL;
		lock->m_audio_encoder = NULL;
	}

	// initialize thread signals
	m_should_stop = false;
	m_should_finish = false;
	m_is_done = false;
	m_error_occurred = false;

	try {
		Init();
	} catch(...) {
		Free();
		throw;
	}

}

OutputManager::~OutputManager() {

	// tell the thread to stop
	if(m_thread.joinable()) {
		Logger::LogInfo("[OutputManager::~OutputManager] " + Logger::tr("Stopping fragment thread ..."));
		m_should_stop = true;
		m_thread.join();
	}

	// free everything
	Free();

}

void OutputManager::Finish() {

	// stop the synchronizer
	if(m_synchronizer != NULL) {
		m_synchronizer->NewSegment(); // needed to make sure that all data is sent to the encoders
		m_synchronizer.reset();
	}

	// after this, we still have to wait until IsFinished() returns true or else the file will be corrupted.
	if(m_fragmented) {
		m_should_finish = true;
	} else {
		SharedLock lock(&m_shared_data);
		assert(lock->m_muxer != NULL);
		lock->m_muxer->Finish();
	}

}

bool OutputManager::IsFinished() {
	SharedLock lock(&m_shared_data);
	if(m_fragmented) {
		return (m_is_done || m_error_occurred);
	} else {
		assert(lock->m_muxer != NULL);
		return (lock->m_muxer->IsDone() || lock->m_muxer->HasErrorOccurred());
	}
}

void OutputManager::AddVideoFrame(std::unique_ptr<AVFrameWrapper> frame) {
	assert(frame->GetFrame()->pts != (int64_t) AV_NOPTS_VALUE);
	SharedLock lock(&m_shared_data);
	if(m_fragmented) {
		int64_t fragment_begin = m_fragment_length * m_output_format.m_video_frame_rate * (lock->m_fragment_number - 1);
		int64_t fragment_end = m_fragment_length * m_output_format.m_video_frame_rate * lock->m_fragment_number;
		if(frame->GetFrame()->pts < fragment_end) {
			frame->GetFrame()->pts -= fragment_begin;
			assert(lock->m_video_encoder != NULL);
			lock->m_video_encoder->AddFrame(std::move(frame));
		} else {
			lock->m_video_frame_queue.push_back(std::move(frame));
		}
	} else {
		assert(lock->m_video_encoder != NULL);
		lock->m_video_encoder->AddFrame(std::move(frame));
	}
}

void OutputManager::AddAudioFrame(std::unique_ptr<AVFrameWrapper> frame) {
	assert(frame->GetFrame()->pts != (int64_t) AV_NOPTS_VALUE);
	SharedLock lock(&m_shared_data);
	if(m_fragmented) {
		int64_t fragment_begin = m_fragment_length * m_output_format.m_audio_sample_rate * (lock->m_fragment_number - 1);
		int64_t fragment_end = m_fragment_length * m_output_format.m_audio_sample_rate * lock->m_fragment_number;
		if(frame->GetFrame()->pts < fragment_end) {
			frame->GetFrame()->pts -= fragment_begin;
			assert(lock->m_audio_encoder != NULL);
			lock->m_audio_encoder->AddFrame(std::move(frame));
		} else {
			lock->m_audio_frame_queue.push_back(std::move(frame));
		}
	} else {
		assert(lock->m_audio_encoder != NULL);
		lock->m_audio_encoder->AddFrame(std::move(frame));
	}
}

int64_t OutputManager::GetVideoFrameDelay() {
	unsigned int frames = 0, packets = 0;
	{
		SharedLock lock(&m_shared_data);
		frames += lock->m_video_frame_queue.size();
		if(lock->m_video_encoder != NULL) {
			frames += lock->m_video_encoder->GetQueuedFrameCount();
			packets += lock->m_video_encoder->GetQueuedPacketCount();
		}
	}
	int64_t interval = 0;
	if(frames > THROTTLE_THRESHOLD_FRAMES) {
		int64_t n = (frames - THROTTLE_THRESHOLD_FRAMES) * 200 / THROTTLE_THRESHOLD_FRAMES;
		interval += n * n;
	}
	if(packets > THROTTLE_THRESHOLD_PACKETS) {
		int64_t n = (packets - THROTTLE_THRESHOLD_PACKETS) * 200 / THROTTLE_THRESHOLD_PACKETS;
		interval += n * n;
	}
	if(interval > 1000000)
		interval = 1000000;
	return interval;
}

unsigned int OutputManager::GetTotalQueuedFrameCount() {
	SharedLock lock(&m_shared_data);
	unsigned int frames = lock->m_video_frame_queue.size();
	if(lock->m_video_encoder != NULL)
		frames += lock->m_video_encoder->GetQueuedFrameCount() + lock->m_video_encoder->GetFrameLatency();
	return frames;
}

double OutputManager::GetActualFrameRate() {
	SharedLock lock(&m_shared_data);
	if(lock->m_video_encoder == NULL)
		return 0.0;
	return lock->m_video_encoder->GetActualFrameRate();
}

double OutputManager::GetActualBitRate() {
	SharedLock lock(&m_shared_data);
	if(lock->m_muxer == NULL)
		return 0.0;
	return lock->m_muxer->GetActualBitRate();
}

uint64_t OutputManager::GetTotalBytes() {
	SharedLock lock(&m_shared_data);
	if(lock->m_muxer == NULL)
		return 0;
	return lock->m_muxer->GetTotalBytes();
}

void OutputManager::Init() {

	// start muxer and encoders
	StartFragment();

	// save output format for synchronizer (we assume that this will always be the same)
	{
		SharedLock lock(&m_shared_data);
		if(lock->m_video_encoder != NULL) {
			m_output_format.m_video_enabled = true;
			m_output_format.m_video_width = lock->m_video_encoder->GetWidth();
			m_output_format.m_video_height = lock->m_video_encoder->GetHeight();
			m_output_format.m_video_frame_rate = lock->m_video_encoder->GetFrameRate();
			m_output_format.m_video_pixel_format = lock->m_video_encoder->GetPixelFormat();
			m_output_format.m_video_colorspace = lock->m_video_encoder->GetColorSpace();
		} else {
			m_output_format.m_video_enabled = false;
		}
		if(lock->m_audio_encoder != NULL) {
			m_output_format.m_audio_enabled = true;
			m_output_format.m_audio_channels = lock->m_audio_encoder->GetChannels(); //TODO// never larger than AV_NUM_DATA_POINTERS
			assert(m_output_format.m_audio_channels <= AV_NUM_DATA_POINTERS);
			m_output_format.m_audio_sample_rate = lock->m_audio_encoder->GetSampleRate();
			m_output_format.m_audio_frame_size = lock->m_audio_encoder->GetFrameSize();
			m_output_format.m_audio_sample_format = lock->m_audio_encoder->GetSampleFormat();
		} else {
			m_output_format.m_audio_enabled = false;
		}
	}

	// start synchronizer
	m_synchronizer.reset(new Synchronizer(this));

	// start fragment thread
	if(m_fragmented) {
		m_thread = std::thread(&OutputManager::FragmentThread, this);
	}

}

void OutputManager::Free() {

	// stop the synchronizer
	m_synchronizer.reset();

	// stop the encoders and muxers
	{
		SharedLock lock(&m_shared_data);
		lock->m_video_encoder = NULL; // deleted by muxer
		lock->m_audio_encoder = NULL; // deleted by muxer
		lock->m_muxer.reset();
	}

}

int OutputManager::Encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt)
{   
	int result = -1;
	int error = 0;
	
	assert(enc_ctx != NULL);
	assert(frame != NULL);
	assert(pkt != NULL);

	AVFrame * avframe_gpu = NULL;

	if (enc_ctx->pix_fmt == AV_PIX_FMT_VAAPI) {
		avframe_gpu = av_frame_alloc();
		if (avframe_gpu == NULL) {
			Logger::LogInfo("[OutputManager::Encode] av_frame_alloc avframe_gpu failed.");
			goto out;
		}

		avframe_gpu->format = AV_PIX_FMT_VAAPI;
		avframe_gpu->width = frame->width;
		avframe_gpu->height = frame->height;
		avframe_gpu->pts = frame->pts;

		error = av_hwframe_get_buffer(enc_ctx->hw_frames_ctx, avframe_gpu, 0);
		if (error < 0) {
			Logger::LogInfo("[OutputManager::Encode] av_hwframe_get_buffer failed.");
			goto out;
		}

		error = av_hwframe_transfer_data(avframe_gpu, frame, 0);
		if (error < 0) {
			Logger::LogInfo("[OutputManager::Encode] av_hwframe_transfer_data failed.");
			goto out;
		}
	
		error = avcodec_send_frame(enc_ctx, avframe_gpu);
	} else {
		error = avcodec_send_frame(enc_ctx, frame);
	}

	if (error < 0) {
		Logger::LogInfo("[OutputManager::Encode] avcodec_send_frame failed");
		goto out;
	}
    
	while (error >= 0) {
		error = avcodec_receive_packet(enc_ctx, pkt);
		if (error == AVERROR(EAGAIN) || error == AVERROR_EOF) {
			result = 0;
			goto out;
		}
		else if (error < 0) { 
			Logger::LogError("[OutputManager::Encode] Error during encoding");
			result = -1;
			goto out;
		}
        
		// succeed
		av_packet_unref(pkt);
	}
	
out:

	if (avframe_gpu) {
#if SSR_USE_AV_FRAME_FREE
		av_frame_free(&avframe_gpu);
#elif SSR_USE_AVCODEC_FREE_FRAME
		avcodec_free_frame(&avframe_gpu);
#else
		av_free(avframe_gpu);
#endif
		avframe_gpu = NULL;
	}
	
	return result;
}

/*
 * nvenc refrence: https://github.com/Brainiarc7/ffmpeg-nvenc-magic/blob/master/doc/examples/decoding_encoding.c
 */
int OutputManager::CheckEncodeName(QString container_name, QString codecname) {

	int result = -1;
	int error = 0;

	const AVCodec *codec = NULL;
	AVCodecContext *context = NULL;

        AVBufferRef *hw_device_ctx_ref = NULL;
        AVBufferRef *hw_frames_ctx_ref = NULL;
	AVFrame *frame = NULL;
	AVPacket *pkt = NULL;
	AVDictionary* options = NULL;

	int i = 0;
	int x = 0, y = 0;

	// 提示函数已经过期，不需要调用了
	// avcodec_register_all();

	codec = avcodec_find_encoder_by_name(codecname.toLatin1().data());
	if (!codec) {
		Logger::LogInfo("[OutputManager::CheckEncodeNameValid] cannot fine codec by name: " + codecname);
		goto out;
	}

	context = avcodec_alloc_context3(codec);
	if (!context) {
		Logger::LogInfo("[OutputManager::CheckEncodeNameValid] avcodec_alloc_context3 failed, codecname: " + codecname);
		goto out;
	}

	pkt = av_packet_alloc();
	if (!pkt) {
		Logger::LogInfo("[OutputManager::CheckEncodeNameValid] av_packet_alloc failed, codecname: " + codecname);
		goto out;
	}

	context->codec_id = codec->id;
	context->codec_type = codec->type;

	context->bit_rate = 10000;
	/* resolution must be a multiple of two */
	context->width = 352;
	context->height = 288;
	/* frames per second */
	context->time_base = av_make_q(1, 10);
	context->framerate = av_make_q(10, 1);

	/* emit one intra frame every ten frames
	 * check frame pict_type before passing frame
	 * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
	 * then gop_size is ignored and the output of encoder
	 * will always be I frame irrespective to gop_size
	 */
	context->gop_size = 10;
	context->max_b_frames = 1;

	if (codecname.contains("nvenc", Qt::CaseInsensitive)) {

		context->pix_fmt = AV_PIX_FMT_NV12;
		av_opt_set(context->priv_data, "preset", "fast", 0);	
		
	} else 	if (codecname.contains("qsv", Qt::CaseInsensitive)) {
		error = av_hwdevice_ctx_create(&hw_device_ctx_ref, AV_HWDEVICE_TYPE_QSV, "auto", NULL, 0);
		if (error < 0) {
			Logger::LogInfo("[OutputManager::CheckEncodeNameValid] create qsv device failed, codecname: " + codecname);
			goto out;
		}

		context->pix_fmt = AV_PIX_FMT_NV12;	
		// bind device ref
		context->hw_device_ctx = av_buffer_ref(hw_device_ctx_ref);

	} else if (codecname.contains("vaapi", Qt::CaseInsensitive)) {

#if !SSR_USE_AVCODEC_PRIVATE_PRESET
		X264Preset(context, "superfast");
#else
		av_dict_set(&options, "preset", "superfast", 0);
#endif

		error = av_hwdevice_ctx_create(&hw_device_ctx_ref, AV_HWDEVICE_TYPE_VAAPI, "/dev/dri/renderD128", NULL, 0);
		if (error < 0) {
			Logger::LogInfo("[OutputManager::CheckEncodeNameValid] create vaapi device failed, codecname: " + codecname);
			goto out;
		}

		context->pix_fmt = AV_PIX_FMT_VAAPI;

                hw_frames_ctx_ref = av_hwframe_ctx_alloc(hw_device_ctx_ref);
                assert(hw_frames_ctx_ref != NULL);

                AVHWFramesContext *hw_frames_ctx = (AVHWFramesContext*)hw_frames_ctx_ref->data;
                hw_frames_ctx->format    = AV_PIX_FMT_VAAPI;
                hw_frames_ctx->sw_format = AV_PIX_FMT_NV12;
                hw_frames_ctx->initial_pool_size = 0;
                hw_frames_ctx->width     = context->width;
                hw_frames_ctx->height    = context->height;

                error = av_hwframe_ctx_init(hw_frames_ctx_ref);
                if (error < 0) {
			Logger::LogInfo("[OutputManager::CheckEncodeNameValid] av_hwframe_ctx_init failed, codecname: " + codecname);
			goto out;
		}

		// bind frame ref
                context->hw_frames_ctx = av_buffer_ref(hw_frames_ctx_ref);
                // context->hw_frames_ctx = hw_frames_ctx_ref;
	}

	error = avcodec_open2(context, codec, &options);
	if (error < 0) {
		Logger::LogInfo("[OutputManager::CheckEncodeNameValid] avcodec_open2 failed, codecname: " + codecname);
		goto out;
	}

#if SSR_USE_AV_FRAME_ALLOC
	frame = av_frame_alloc();
#else
	frame = avcodec_alloc_frame();
#endif
	if (!frame) {
		Logger::LogInfo("[OutputManager::CheckEncodeNameValid] av_frame_alloc failed, codecname: " + codecname);
		goto out;
	}

	frame->width  = context->width;
	frame->height = context->height;

	if (codecname.contains("qsv", Qt::CaseInsensitive)) {
		error = setenv("LIBVA_DRIVER_NAME", "iHD", 1);
		if (error < 0) {
			Logger::LogInfo("[OutputManager::CheckEncodeNameValid] setenv LIBVA_DRIVER_NAME=iHD failed, codecname: " + codecname);
                        goto out;
		}

		frame->format = context->pix_fmt;
		error = av_frame_get_buffer(frame, 32);
		if (error < 0) {
			Logger::LogInfo("[OutputManager::CheckEncodeNameValid] av_frame_get_buffer failed, codecname: " + codecname);
			goto out;
		}
		
		/* make sure the frame data is writabl */
		error = av_frame_make_writable(frame);
		if (error < 0) {
			Logger::LogInfo("[OutputManager::CheckEncodeNameValid] av_frame_make_writable failed, codecname: " + codecname);
			goto out;
		}

	} else if (codecname.contains("vaapi", Qt::CaseInsensitive) ||
			codecname.contains("nvenc", Qt::CaseInsensitive)) {

		error = unsetenv("LIBVA_DRIVER_NAME");
		if (error < 0) {
			Logger::LogInfo("[OutputManager::CheckEncodeNameValid] unsetenv LIBVA_DRIVER_NAME failed, codecname: " + codecname);
                        goto out;
		}

		frame->format = AV_PIX_FMT_NV12;

		// another way to alloc frame buffer
		/*size_t totalsize = grow_align16(context->width) * context->height * 1.5;
		frame_data = std::make_shared<AVFrameData>(totalsize);
		uint8_t *raw_data = frame_data->GetData();
		frame->data[0] = raw_data;
		frame->data[1] = raw_data + context->width * context->height;*/
		
		error = av_image_alloc(frame->data, frame->linesize, context->width, context->height, AV_PIX_FMT_NV12, 16);
		if (error < 0) {
			Logger::LogInfo("[OutputManager::CheckEncodeNameValid] av_image_alloc failed, codecname: " + codecname);
			goto out;
		}
	}

	// prepare yuv image
	for (i = 0; i < 10; i++) {

		/* prepare a dummy image */
		/* Y */
		for (y = 0; y < context->height; y++) {
			for (x = 0; x < context->width; x++) {
				frame->data[0][y * frame->linesize[0] + x] = x + y + i * 3;
			}
		}

		/* Cb and Cr */
		for (y = 0; y < context->height/2; y++) {
			for (x = 0; x < context->width/2; x++) {
				frame->data[1][y * frame->linesize[1] + 2 * x] = 128 + y + i * 2;
				frame->data[1][y * frame->linesize[1] + 2 * x + 1] = 64 + x + i * 5;
			}
		}

		frame->pts = i;

		/* encode the image */
		error = Encode(context, frame, pkt);
		if (error < 0) {
			// encode this frame failed, mean that this codec is invalid
			Logger::LogInfo("[OutputManager::CheckEncodeNameValid] encode failed, codecname: " + codecname);
			goto out;
		}
	}

	// encode all frames suceed
	result = 0;

out:

	if (options) {
		av_dict_free(&options);
		options = NULL;		
	}

	if (frame) {
		// 查看文档，av_image_alloc 分配的内存需要手动释放，通过 av_frame_get_buffer 分配的貌似不需要
		if (codecname.contains("vaapi", Qt::CaseInsensitive)) {
			av_freep(&frame->data[0]);
		}

#if SSR_USE_AV_FRAME_FREE
		av_frame_free(&frame);
#elif SSR_USE_AVCODEC_FREE_FRAME
		avcodec_free_frame(&frame);
#else
		av_free(frame);
#endif
		frame = NULL;
	}

	if (hw_frames_ctx_ref) {
		av_buffer_unref(&hw_frames_ctx_ref);
		hw_frames_ctx_ref = NULL;
	}

	if (hw_device_ctx_ref) {
		av_buffer_unref(&hw_device_ctx_ref);
		hw_device_ctx_ref = NULL;
	}	
	
	if (pkt) {
		av_packet_free(&pkt);
		pkt = NULL;
	}

	if (context) {
		avcodec_free_context(&context);
		context = NULL;
	}

	return result;
}

/*
 * choose encode type by cpu type and gpu type
 * container_name: video container name, [mp4|ogv]
 * ret: QString
 */
QString OutputManager::ChooseEncodeName(QString container_name) {

	if ((QString::compare(container_name, "mp4", Qt::CaseInsensitive) != 0) && 
		QString::compare(container_name, "ogg", Qt::CaseInsensitive) != 0) {
		Logger::LogWarning("[OutputManager::ChooseEncodeName] container name invalid: " + container_name);
		return QString("");
	}

	int ret = 0;
	if (QString::compare(container_name, "mp4", Qt::CaseInsensitive) == 0) {

		// check nvenc
		ret = CheckEncodeName(container_name, "h264_nvenc");
		if (ret >= 0) {
			return QString("h264_nvenc");
		}

		// check qsv
		ret = CheckEncodeName(container_name, "h264_qsv");
		if (ret >= 0) {
			return QString("h264_qsv");
		}

		// check vaapi
		ret = CheckEncodeName(container_name, "h264_vaapi");
		if (ret >= 0) {
			return QString("h264_vaapi");
		}
		
		return QString("libx264");
	}

	if (QString::compare(container_name, "ogg", Qt::CaseInsensitive) == 0) {

		// ogg doesnot support hw acce
		return QString("libtheora");
	}

	return QString("");
}

void OutputManager::StartFragment() {

	// get fragment number
	unsigned int fragment_number = 0;
	if(m_fragmented) {
		SharedLock lock(&m_shared_data);
		fragment_number = lock->m_fragment_number;
	}

	// create new muxer and encoders
	// we can't hold the lock while doing this because this could take some time
	QString filename;
	if(m_fragmented) {
		filename = GetNewFragmentFile(m_output_settings.file, fragment_number);
	} else {
		filename = m_output_settings.file;
	}
	std::unique_ptr<Muxer> muxer(new Muxer(m_output_settings.container_avname, filename));
	VideoEncoder *video_encoder = NULL;
	AudioEncoder *audio_encoder = NULL;

	// check which encode type should be used, 
	QString enc_name = ChooseEncodeName(m_output_settings.container_avname);
	int ret = 0;
	
	// reset encode options according to encode type
	// preset of libx264 and h264_vaapi: ultrafast superfast veryfast faster fast medium slow slower veryslow placebo
	// quality of qsv: global_quality [1-51] lower num mean higher quality
	ret = unsetenv("LIBVA_DRIVER_NAME");
	if (ret < 0) {
		Logger::LogError("unsetenv LIBVA_DRIVER_NAME failed");
		return ;
	}

	std::vector<std::pair<QString, QString> >().swap(m_output_settings.video_options);
	if (enc_name.contains("qsv")) {

		if (m_output_settings.encode_quality == "0") {
			m_output_settings.video_options.push_back(std::make_pair(QString("global_quality"), QString::number(15)));
		} else if (m_output_settings.encode_quality == "1") {
			m_output_settings.video_options.push_back(std::make_pair(QString("global_quality"), QString::number(14)));
		} else if (m_output_settings.encode_quality == "2") {
			m_output_settings.video_options.push_back(std::make_pair(QString("global_quality"), QString::number(13)));
		}
		
		ret = setenv("LIBVA_DRIVER_NAME", "iHD", 1);
		if (ret < 0) {
			Logger::LogError("setenv LIBVA_DRIVER_NAME failed");
			return ;
		}

	} else if (enc_name.contains("vaapi") ||
			enc_name.contains("nvenc")) {

		unsigned int vaapi_kbit_rate = 0;

		if (m_output_settings.encode_quality == "0") {
			vaapi_kbit_rate = 2048;
		} else if (m_output_settings.encode_quality == "1") {
			vaapi_kbit_rate = 4096;
		} else if (m_output_settings.encode_quality == "2") {
			vaapi_kbit_rate = 8192;
		}

		vaapi_kbit_rate = vaapi_kbit_rate * m_output_settings.video_width / 1920 * m_output_settings.video_height / 1080;
		m_output_settings.video_kbit_rate = vaapi_kbit_rate;

	} else if (enc_name == "libtheora") {

		// 在Recording.cpp里统一判断音频编码器
//		m_output_settings.audio_codec_avname = QString("libvorbis");
		// 音频采样率为8k时，比特率不能为128，否则会初始化失败
		m_output_settings.audio_kbit_rate = 32;

		unsigned int theora_kbit_rate = 0;

		if (m_output_settings.encode_quality == "0") {
			theora_kbit_rate = 2048;
		} else if (m_output_settings.encode_quality == "1") {
			theora_kbit_rate = 4096;
		} else if (m_output_settings.encode_quality == "2") {
			theora_kbit_rate = 8192;
		}

		theora_kbit_rate = theora_kbit_rate * m_output_settings.video_width / 1920 * m_output_settings.video_height / 1080;
		m_output_settings.video_kbit_rate = theora_kbit_rate;

	} else if (enc_name == "libx264") {

		m_output_settings.video_options.push_back(std::make_pair(QString("crf"), QString::number(23)));
		if (m_output_settings.encode_quality == "0") {
			m_output_settings.video_options.push_back(std::make_pair(QString("preset"), QString("superfast")));
		} else if (m_output_settings.encode_quality == "1") {
			m_output_settings.video_options.push_back(std::make_pair(QString("preset"), QString("fast")));
		} else if (m_output_settings.encode_quality == "2") {
			m_output_settings.video_options.push_back(std::make_pair(QString("preset"), QString("veryslow")));
                }

	} else {
		Logger::LogInfo("[OutputManager::StartFragment] enc name invalid");
		return;
	} 

	m_output_settings.video_codec_avname = enc_name;
	Logger::LogInfo("[OutputManager::StartFragment]  video codec name:" + m_output_settings.video_codec_avname);

	for(unsigned int i = 0; i < m_output_settings.video_options.size(); ++i) {
		const QString &key = m_output_settings.video_options[i].first, &value = m_output_settings.video_options[i].second;
		Logger::LogInfo("[OutputManager::StartFragment] video_options key: " + key + " val: " + value);
	}
	Logger::LogInfo("[OutputManager::StartFragment]  video kbit:" + QString::number(m_output_settings.video_kbit_rate));
	Logger::LogInfo("[OutputManager::StartFragment]  video framerate:" + QString::number(m_output_settings.video_frame_rate));

	Logger::LogInfo("[OutputManager::StartFragment]  audio codec name:" + m_output_settings.audio_codec_avname);
	for(unsigned int i = 0; i < m_output_settings.audio_options.size(); ++i) {
		const QString &key = m_output_settings.audio_options[i].first, &value = m_output_settings.audio_options[i].second;
		Logger::LogInfo("[OutputManager::StartFragment]  audio_options key: " + key + " val: " + value);
	}
	Logger::LogInfo("[OutputManager::StartFragment]  audio kbit:" + QString::number(m_output_settings.audio_kbit_rate));
	Logger::LogInfo("[OutputManager::StartFragment]  audio channels:" + QString::number(m_output_settings.audio_channels));
	Logger::LogInfo("[OutputManager::StartFragment]  audio samplerate:" + QString::number(m_output_settings.audio_sample_rate));

	video_encoder = muxer->AddVideoEncoder(m_output_settings.video_codec_avname, m_output_settings.video_options, m_output_settings.video_kbit_rate * 1000,
						   m_output_settings.video_width, m_output_settings.video_height, m_output_settings.video_frame_rate);

	if(!m_output_settings.audio_codec_avname.isEmpty()){
		audio_encoder = muxer->AddAudioEncoder(m_output_settings.audio_codec_avname, m_output_settings.audio_options, m_output_settings.audio_kbit_rate * 1000,
											   m_output_settings.audio_channels, m_output_settings.audio_sample_rate);
	}
	muxer->Start();

	// acquire lock and share the muxer and encoders
	SharedLock lock(&m_shared_data);
	lock->m_muxer = std::move(muxer);
	lock->m_video_encoder = video_encoder;
	lock->m_audio_encoder = audio_encoder;

	// increment fragment number
	// It's important that this is done here (i.e. after the encoders have been set up), because the fragment number
	// acts as a signal to AddVideoFrame/AddAudioFrame that they can pass frames to the encoders.
	++lock->m_fragment_number;

	// push queued frames to the new encoders
	if(m_fragmented) {
		while(!lock->m_video_frame_queue.empty()) {
			int64_t fragment_begin = m_fragment_length * m_output_format.m_video_frame_rate * (lock->m_fragment_number - 1);
			int64_t fragment_end = m_fragment_length * m_output_format.m_video_frame_rate * lock->m_fragment_number;
			if(lock->m_video_frame_queue.front()->GetFrame()->pts >= fragment_end)
				break;
			std::unique_ptr<AVFrameWrapper> frame = std::move(lock->m_video_frame_queue.front());
			lock->m_video_frame_queue.pop_front();
			frame->GetFrame()->pts -= fragment_begin;
			video_encoder->AddFrame(std::move(frame));
		}
		while(!lock->m_audio_frame_queue.empty()) {
			int64_t fragment_begin = m_fragment_length * m_output_format.m_audio_sample_rate * (lock->m_fragment_number - 1);
			int64_t fragment_end = m_fragment_length * m_output_format.m_audio_sample_rate * lock->m_fragment_number;
			if(lock->m_audio_frame_queue.front()->GetFrame()->pts >= fragment_end)
				break;
			std::unique_ptr<AVFrameWrapper> frame = std::move(lock->m_audio_frame_queue.front());
			lock->m_audio_frame_queue.pop_front();
			frame->GetFrame()->pts -= fragment_begin;
			audio_encoder->AddFrame(std::move(frame));
		}
	}

}

void OutputManager::StopFragment() {

	// acquire lock and steal the muxer
	std::unique_ptr<Muxer> muxer;
	{
		SharedLock lock(&m_shared_data);
		muxer = std::move(lock->m_muxer);
		lock->m_video_encoder = NULL; // deleted by muxer
		lock->m_audio_encoder = NULL; // deleted by muxer
	}

	// wait until the muxer is finished
	// we can't hold the lock while doing this because this could take some time
	assert(muxer != NULL);
	muxer->Finish();
	while(!muxer->IsDone() && !muxer->HasErrorOccurred()) {
		usleep(200000);
	}

	// delete everything
	muxer.reset();

}

void OutputManager::FragmentThread() {
	try {

		pid_t tid = gettid();
		Logger::LogInfo("[OutputManager::FragmentThread] " + Logger::tr("Fragment thread started. tid: ") + QString::number(tid));

		while(!m_should_stop) {

			// should we start a new fragment?
			bool finishing = m_should_finish, next_fragment = false;
			{
				SharedLock lock(&m_shared_data);
				//TODO// this delays the creation of a new fragment by one frame, this could be improved
				if(finishing) {
					next_fragment = (!lock->m_video_frame_queue.empty() || !lock->m_audio_frame_queue.empty());
				} else {
					next_fragment = (!lock->m_video_frame_queue.empty() && !lock->m_audio_frame_queue.empty());
				}
			}

			// do what needs to be done
			if(next_fragment) {
				Logger::LogInfo("[OutputManager::FragmentThread] " + Logger::tr("Next fragment ..."));
				StopFragment();
				StartFragment();
			} else if(finishing) {
				Logger::LogInfo("[OutputManager::FragmentThread] " + Logger::tr("Finishing ..."));
				StopFragment();
				break;
			} else {
				usleep(200000);
			}

		}

		// tell the others that we're done
		m_is_done = true;

		Logger::LogInfo("[OutputManager::FragmentThread] " + Logger::tr("Fragment thread stopped."));

	} catch(const std::exception& e) {
		m_error_occurred = true;
		Logger::LogError("[OutputManager::FragmentThread] " + Logger::tr("Exception '%1' in fragment thread.").arg(e.what()));
	} catch(...) {
		m_error_occurred = true;
		Logger::LogError("[OutputManager::FragmentThread] " + Logger::tr("Unknown exception in fragment thread."));
	}
}
