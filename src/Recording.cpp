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

#include "Recording.h"
#include "CommandLineOptions.h"
#include "EnumStrings.h"
#include "Muxer.h"
#include "VideoEncoder.h"
#include "AudioEncoder.h"
#include "Synchronizer.h"
#include "X11Input.h"
#include "SimpleSynth.h"

ENUMSTRINGS(Recording::enum_video_area) = {
    {Recording::VIDEO_AREA_SCREEN, "screen"},
    {Recording::VIDEO_AREA_FIXED, "fixed"},
    {Recording::VIDEO_AREA_CURSOR, "cursor"},
#if SSR_USE_OPENGL_RECORDING
    {Recording::VIDEO_AREA_GLINJECT, "glinject"},
#endif
#if SSR_USE_V4L2
    {Recording::VIDEO_AREA_V4L2, "v4l2"},
#endif
};

ENUMSTRINGS(Recording::enum_audio_backend) = {
#if SSR_USE_ALSA
    {Recording::AUDIO_BACKEND_ALSA, "alsa"},
#endif
#if SSR_USE_PULSEAUDIO
    {Recording::AUDIO_BACKEND_PULSEAUDIO, "pulseaudio"},
#endif
#if SSR_USE_JACK
    {Recording::AUDIO_BACKEND_JACK, "jack"},
#endif
};

ENUMSTRINGS(Recording::enum_container) = {
    {Recording::CONTAINER_MKV, "mkv"},
    {Recording::CONTAINER_MP4, "mp4"},
    {Recording::CONTAINER_WEBM, "webm"},
    {Recording::CONTAINER_OGG, "ogg"},
    {Recording::CONTAINER_OTHER, "other"},
};

ENUMSTRINGS(Recording::enum_video_codec) = {
    {Recording::VIDEO_CODEC_H264, "h264"},
    {Recording::VIDEO_CODEC_VP8, "vp8"},
    {Recording::VIDEO_CODEC_THEORA, "theora"},
    {Recording::VIDEO_CODEC_OTHER, "other"},
};

ENUMSTRINGS(Recording::enum_audio_codec) = {
    {Recording::AUDIO_CODEC_VORBIS, "vorbis"},
    {Recording::AUDIO_CODEC_MP3, "mp3"},
    {Recording::AUDIO_CODEC_AAC, "aac"},
    {Recording::AUDIO_CODEC_UNCOMPRESSED, "uncompressed"},
    {Recording::AUDIO_CODEC_OTHER, "other"},
};

ENUMSTRINGS(Recording::enum_h264_preset) = {
    {Recording::H264_PRESET_ULTRAFAST, "ultrafast"},
    {Recording::H264_PRESET_SUPERFAST, "superfast"},
    {Recording::H264_PRESET_VERYFAST, "veryfast"},
    {Recording::H264_PRESET_FASTER, "faster"},
    {Recording::H264_PRESET_FAST, "fast"},
    {Recording::H264_PRESET_MEDIUM, "medium"},
    {Recording::H264_PRESET_SLOW, "slow"},
    {Recording::H264_PRESET_SLOWER, "slower"},
    {Recording::H264_PRESET_VERYSLOW, "veryslow"},
    {Recording::H264_PRESET_PLACEBO, "placebo"},
};



static QString GetNewSegmentFile(const QString& file, bool add_timestamp) {
	QFileInfo fi(file);
	QDateTime now = QDateTime::currentDateTime();
	QString newfile;
	unsigned int counter = 0;
	do {
		++counter;
		newfile = fi.completeBaseName();
		if(add_timestamp) {
			if(!newfile.isEmpty())
				newfile += "-";
			newfile += now.toString("yyyy-MM-dd_hh.mm.ss");
		}
		if(counter != 1) {
			if(!newfile.isEmpty())
				newfile += "-";
			newfile += "(" + QString::number(counter) + ")";
		}
		if(!fi.suffix().isEmpty())
			newfile += "." + fi.suffix();
		newfile = fi.path() + "/" + newfile;
	} while(QFileInfo(newfile).exists());
	return newfile;
}

static std::vector<std::pair<QString, QString> > GetOptionsFromString(const QString& str) {
	std::vector<std::pair<QString, QString> > options;
	QStringList optionlist = SplitSkipEmptyParts(str, ',');
	for(int i = 0; i < optionlist.size(); ++i) {
		QString a = optionlist[i];
		int p = a.indexOf('=');
		if(p < 0) {
			options.push_back(std::make_pair(a.trimmed(), QString()));
		} else {
			options.push_back(std::make_pair(a.mid(0, p).trimmed(), a.mid(p + 1).trimmed()));
		}
	}
	return options;
}

static QString ReadableSizeIEC(uint64_t size, const QString& suffix) {
	if(size < (uint64_t) 10 * 1024)
		return QString::number(size) + " " + suffix;
	if(size < (uint64_t) 10 * 1024 * 1024)
		return QString::number((size + 512) / 1024) + " Ki" + suffix;
	if(size < (uint64_t) 10 * 1024 * 1024 * 1024)
		return QString::number((size / 1024 + 512) / 1024) + " Mi" + suffix;
	return QString::number((size / (1024 * 1024) + 512) / 1024) + " Gi" + suffix;
}

static QString ReadableSizeSI(uint64_t size, const QString& suffix) {
	if(size < (uint64_t) 10 * 1000)
		return QString::number(size) + " " + suffix;
	if(size < (uint64_t) 10 * 1000 * 1000)
		return QString::number((size + 500) / 1000) + " k" + suffix;
	if(size < (uint64_t) 10 * 1000 * 1000 * 1000)
		return QString::number((size / 1000 + 500) / 1000) + " M" + suffix;
	return QString::number((size / (1000 * 1000) + 512) / 1024) + " G" + suffix;
}

static QString ReadableTime(int64_t time_micro) {
	unsigned int time = (time_micro + 500000) / 1000000;
	return QString("%1:%2:%3")
			.arg(time / 3600)
			.arg((time / 60) % 60, 2, 10, QLatin1Char('0'))
			.arg(time % 60, 2, 10, QLatin1Char('0'));
}

static QString ReadableWidthHeight(unsigned int width, unsigned int height) {
	if(width == 0 && height == 0)
		return "?";
	return QString::number(width) + "x" + QString::number(height);
}

Recording::Recording(QSettings* qsettings){
	m_page_started = false;
	m_input_started = false;
	m_output_started = false;
	m_pulseaudio_sources = PulseAudioInput::GetSourceList();
	if(m_pulseaudio_sources.empty()){
		m_audio_enabled = false;
	}
	settings = qsettings;
}

Recording::~Recording() {}

bool Recording::TryStartPage() {
	if(m_page_started)
		return true;

	StartPage();
	return true;
}

void Recording::StartPage() {

	if(m_page_started)
		return;

	assert(!m_input_started);
	assert(!m_output_started);

	// get the video input settings
		
	m_video_area = VIDEO_AREA_SCREEN;
	m_video_area_follow_fullscreen = true;


	//从QSetting 配置中读取x y width height 等相关参数
	m_video_x = settings->value("input/video_x", 0).toUInt();
	m_video_y = settings->value("input/video_y", 0).toUInt();
	m_video_in_width = settings->value("input/video_w", 800).toUInt();
	m_video_in_height = settings->value("input/video_h", 600).toUInt();
	m_video_frame_rate = settings->value("input/video_frame_rate", 30).toUInt();
	m_video_scaling = false;

	// get the audio input settings
	m_audio_enabled = settings->value("input/audio_enabled", true).toBool();
	m_audio_enabled = false;
	m_audio_channels = 2;
	m_audio_sample_rate = 48000;
	m_audio_backend = AUDIO_BACKEND_PULSEAUDIO;


	m_pulseaudio_source = settings->value("input/audio_pulseaudio_source", QString()).toString();

	// get file settings
	m_file_base = settings->value("output/file").toString();

	m_separate_files = true;
	m_add_timestamp = true;

	// get the output settings
	m_output_settings.file = QString(""); // will be set later
	m_output_settings.container_avname = settings->value("output/container_av", QString()).toString();
	//m_output_settings.container_avname = QString("mp4");   

	m_output_settings.video_codec_avname = settings->value("output/video_codec_av", QString()).toString();

	m_output_settings.video_kbit_rate = 128;
	//m_output_settings.video_options.clear();
	m_output_settings.video_width = 3840;
	m_output_settings.video_height = 2160;
	m_output_settings.video_frame_rate = m_video_frame_rate;
	m_output_settings.video_allow_frame_skipping = true;

	m_output_settings.audio_codec_avname = (m_audio_enabled)? settings->value("output/audio_codec_av", QString()).toString():QString();
	m_output_settings.audio_kbit_rate = 128;
	m_output_settings.audio_options.clear();
	m_output_settings.audio_channels = m_audio_channels;
	m_output_settings.audio_sample_rate = m_audio_sample_rate;


	//H264
	m_output_settings.video_options.push_back(std::make_pair(QString("crf"), QString::number(23)));
	m_output_settings.video_options.push_back(std::make_pair(QString("preset"), EnumToString(H264_PRESET_ULTRAFAST)));


	Logger::LogInfo("[PageRecord::StartPage] " + tr("Starting page ..."));

	try {       

#if SSR_USE_OPENGL_RECORDING
		// for OpenGL recording, create the input now
		if(m_video_area == PageInput::VIDEO_AREA_GLINJECT) {
			if(glinject_auto_launch)
				GLInjectInput::LaunchApplication(glinject_channel, glinject_relax_permissions, glinject_command, glinject_working_directory);
			m_gl_inject_input.reset(new GLInjectInput(glinject_channel, glinject_relax_permissions, m_video_record_cursor, glinject_limit_fps, m_video_frame_rate));
		}
#endif

#if SSR_USE_JACK
		if(m_audio_enabled) {
			// for JACK, start the input now
			if(m_audio_backend == PageInput::AUDIO_BACKEND_JACK)
				m_jack_input.reset(new JACKInput(jack_connect_system_capture, jack_connect_system_playback));
		}
#endif

	} catch(...) {
		Logger::LogError("[PageRecord::StartPage] " + tr("Error: Something went wrong during initialization."));
#if SSR_USE_OPENGL_RECORDING
		m_gl_inject_input.reset();
#endif
#if SSR_USE_JACK
		m_jack_input.reset();
#endif
	}

	Logger::LogInfo("[PageRecord::StartPage] " + tr("Started page."));

	m_page_started = true;
	m_recorded_something = false;
	m_wait_saving = false;
	m_error_occurred = false;


	UpdateInput();
}

/**
 * 获取录制视频的矩形区域
 **/
std::vector<QRect> Recording::GetScreenGeometries() {
	std::vector<QRect> screen_geometries;
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
	for(QScreen *screen :  QApplication::screens()) {
		QRect geometry = screen->geometry();
		qreal ratio = screen->devicePixelRatio();
		screen_geometries.emplace_back(geometry.x(), geometry.y(), lrint((qreal) geometry.width() * ratio), lrint((qreal) geometry.height() * ratio));
	}
#else
	for(int i = 0; i < QApplication::desktop()->screenCount(); ++i) {
		screen_geometries.push_back(QApplication::desktop()->screenGeometry(i));
	}
#endif
	return screen_geometries;
}

QRect Recording::CombineScreenGeometries(const std::vector<QRect>& screen_geometries) {
	QRect combined_geometry;
	for(const QRect &geometry : screen_geometries) {
		combined_geometry |= geometry;
	}
	return combined_geometry;
}

/**
 * 保存录屏相关 QSettings 配置
 **/
void Recording::SaveSettings(QSettings* settings) {
	//这些配置项后续通过DBus 接口从配置中心读取

	//略过欢迎界面
	settings->setValue("welcome/skip_page", true);

	//输入界面参数配置
	settings->setValue("input/video_area",EnumToString((Recording::enum_video_area)(Recording::VIDEO_AREA_SCREEN))); //全屏录制
	settings->setValue("input/video_area_follow_fullscreen",true);

	std::vector<QRect> screen_geometries = GetScreenGeometries();
	QRect rect = CombineScreenGeometries(screen_geometries); //录制屏幕的矩形区域
	settings->setValue("input/video_x", rect.left()); //X
	settings->setValue("input/video_y", rect.top()); //Y
	settings->setValue("input/video_w", rect.width()); //width
	settings->setValue("input/video_h", rect.height()); //height

	settings->setValue("input/video_frame_rate", 10); //帧率
	settings->setValue("input/video_scale", false);
	settings->setValue("input/video_scaled_w", 854);
	settings->setValue("input/video_scaled_h", 480);
	settings->setValue("input/video_record_cursor", false);
	settings->setValue("input/audio_enabled",false); //音频录制
	settings->setValue("input/audio_backend", EnumToString(Recording::AUDIO_BACKEND_PULSEAUDIO));
	//settings.setValue("input/audio_pulseaudio_source", GetPulseAudioSourceName());

	//输出页面配置参数
	settings->setValue("output/file", "yefeng.mp4");
	settings->setValue("output/separate_files", true);
	settings->setValue("output/add_timestamp", true);
	settings->setValue("output/container", EnumToString(Recording::CONTAINER_MP4));
	settings->setValue("output/container_av", "mp4"); //mp4 格式
	settings->setValue("output/video_codec", EnumToString(Recording::VIDEO_CODEC_H264));
	settings->setValue("output/video_codec_av", "libx264");
	settings->setValue("output/video_kbit_rate", 128);
	settings->setValue("output/video_h264_crf", 23);
	settings->setValue("output/video_h264_preset", (Recording::enum_h264_preset)Recording::H264_PRESET_SUPERFAST);
	//settings.setValue("output/video_vp8_cpu_used", GetVP8CPUUsed());
	//settings->setValue("output/video_options", GetVideoOptions());
	settings->setValue("output/video_allow_frame_skipping", true);


	settings->setValue("record/hotkey_enable", false); //禁用快捷键
	settings->setValue("record/hotkey_ctrl", false);
	settings->setValue("record/hotkey_shift", false);
	settings->setValue("record/hotkey_alt",false);
	settings->setValue("record/hotkey_super",false);
}

void Recording::StopPage(bool save) {

	if(!m_page_started)
		return;

	StopOutput(true);
	StopInput();

	Logger::LogInfo("[PageRecord::StopPage] " + tr("Stopping page ..."));

	if(m_output_manager != NULL) {

		// stop the output
		if(save)
			FinishOutput();
		m_output_manager.reset();

		// delete the file if it isn't needed
		if(!save && m_file_protocol.isNull()) {
			if(QFileInfo(m_output_settings.file).exists())
				QFile(m_output_settings.file).remove();
		}

	}

#if SSR_USE_OPENGL_RECORDING
	// stop GLInject input
	m_gl_inject_input.reset();
#endif

#if SSR_USE_JACK
	// stop JACK input
	m_jack_input.reset();
#endif

	Logger::LogInfo("[PageRecord::StopPage] " + tr("Stopped page."));

	m_page_started = false;
	exit(0);
}

void Recording::StartOutput() {
	if(m_output_started)
		return;

	try {

		Logger::LogInfo("[PageRecord::StartOutput] " + tr("Starting output ..."));

		if(m_output_manager == NULL) {

			// set the file name
			m_output_settings.file = GetNewSegmentFile(m_file_base, m_add_timestamp);
			//m_output_settings.file = QString("/home/yefeng/ks-vaudit/build-release/src/yefeng.mp4"); //调试用

			// log the file name
			{
				QString file_name;
				if(m_file_protocol.isNull())
					file_name = m_output_settings.file;
				else
					file_name = "(" + m_file_protocol + ")";
				Logger::LogInfo("[PageRecord::StartOutput] " + tr("Output file: %1").arg(file_name));
			}

			// for X11 recording, update the video size (if possible)
			if(m_x11_input != NULL)
				m_x11_input->GetCurrentSize(&m_video_in_width, &m_video_in_height);

			// calculate the output width and height
		
				// If the user did not explicitly select scaling, then don't force scaling just because the recording area is one pixel too large.
				// One missing row/column of pixels is probably better than a blurry video (and scaling is SLOW).
			//m_video_in_width = m_video_in_width / 2 * 2;
			//m_video_in_height = m_video_in_height / 2 * 2;
			m_video_in_width = 3840;
			m_video_in_height = 2160;   
			m_output_settings.video_width = m_video_in_width;
			m_output_settings.video_height = m_video_in_height;

			// start the output
            //m_output_settings.container_avname
            Logger::LogInfo("the m_output_settings.container_avname is --->" + m_output_settings.container_avname);
			Logger::LogInfo("the m_output_settings.video_codec_avname  is --->" + m_output_settings.video_codec_avname);
			
			m_output_manager.reset(new OutputManager(m_output_settings));
		

		} else {

			// start a new segment
			m_output_manager->GetSynchronizer()->NewSegment();

		}

		Logger::LogInfo("[PageRecord::StartOutput] " + tr("Started output."));

		m_output_started = true;
		m_recorded_something = true;	
	
		UpdateInput();
		
	} catch(...) {
        Logger::LogError("[PageRecord::StartOutput] " + tr("Error: Something went wrong during initialization."));
	}

}

void Recording::StopOutput(bool final) {
	assert(m_page_started);

	if(!m_output_started)
		return;

	Logger::LogInfo("[PageRecord::StopOutput] " + tr("Stopping output ..."));

	// if final, then StopPage will stop the output (and delete the file if needed)
	if(m_separate_files && !final) {

		// stop the output
		FinishOutput();
		m_output_manager.reset();

		// change the file name
		m_output_settings.file = QString();

		// reset the output video size
		m_output_settings.video_width = 0;
		m_output_settings.video_height = 0;

	}

	Logger::LogInfo("[PageRecord::StopOutput] " + tr("Stopped output."));

#if SSR_USE_ALSA
	// if final, don't play the notification (it would get interrupted anyway)
	if(m_simple_synth != NULL && !final)
		m_simple_synth->PlaySequence(SEQUENCE_RECORD_STOP.data(), SEQUENCE_RECORD_STOP.size());
#endif

	m_output_started = false;
	UpdateInput();
}

void Recording::StartInput() {
	assert(m_page_started);

	if(m_input_started)
		return;

	assert(m_x11_input == NULL);
	assert(m_pulseaudio_input == NULL);

	try {

		Logger::LogInfo("[PageRecord::StartInput] " + tr("Starting input ..."));

		// start the video input
		if(m_video_area == VIDEO_AREA_SCREEN || m_video_area == VIDEO_AREA_FIXED || m_video_area == VIDEO_AREA_CURSOR) {
			m_x11_input.reset(new X11Input(m_video_x, m_video_y, m_video_in_width, m_video_in_height, false,
										   m_video_area == VIDEO_AREA_CURSOR, m_video_area_follow_fullscreen));

			//connect(m_x11_input.get(), SIGNAL(CurrentRectangleChanged()), this, SLOT(OnUpdateRecordingFrame()), Qt::QueuedConnection);
		}


		// start the audio input
		if(m_audio_enabled) {
			if(m_audio_backend == AUDIO_BACKEND_PULSEAUDIO)
				m_pulseaudio_input.reset(new PulseAudioInput(m_pulseaudio_source, m_audio_sample_rate));

			
		}

		Logger::LogInfo("[PageRecord::StartInput] " + tr("Started input."));

		m_input_started = true;

	} catch(...) {
		Logger::LogError("[PageRecord::StartInput] " + tr("Error: Something went wrong during initialization."));
		m_x11_input.reset();
#if SSR_USE_OPENGL_RECORDING
		if(m_gl_inject_input != NULL)
			m_gl_inject_input->SetCapturing(false);
#endif
#if SSR_USE_V4L2
		m_v4l2_input.reset();
#endif
#if SSR_USE_ALSA
		m_alsa_input.reset();
#endif
#if SSR_USE_PULSEAUDIO
		m_pulseaudio_input.reset();
#endif
		// JACK shouldn't stop until the page stops
		return;
	}

}

void Recording::StopInput() {
	assert(m_page_started);

	if(!m_input_started)
		return;

	Logger::LogInfo("[PageRecord::StopInput] " + tr("Stopping input ..."));

	m_x11_input.reset();
#if SSR_USE_OPENGL_RECORDING
	if(m_gl_inject_input != NULL)
		m_gl_inject_input->SetCapturing(false);
#endif
#if SSR_USE_V4L2
	m_v4l2_input.reset();
#endif
#if SSR_USE_ALSA
	m_alsa_input.reset();
#endif
#if SSR_USE_PULSEAUDIO
	m_pulseaudio_input.reset();
#endif
	// JACK shouldn't stop until the page stops

	Logger::LogInfo("[PageRecord::StopInput] " + tr("Stopped input."));

	m_input_started = false;

}

void Recording::FinishOutput() {
	assert(m_output_manager != NULL);

	// tell the output manager to finish
	m_output_manager->Finish();

	// wait until it has actually finished
	m_wait_saving = true;

	//unsigned int frames_left = m_output_manager->GetVideoEncoder()->GetFrameLatency();
	unsigned int frames_done = 0, frames_total = 0;
	//QProgressDialog dialog(tr("Encoding remaining data ..."), QString(), 0, frames_total, this);

	while(!m_output_manager->IsFinished()) {
		unsigned int frames = m_output_manager->GetTotalQueuedFrameCount();
		if(frames > frames_total)
			frames_total = frames;
		if(frames_total - frames > frames_done)
			frames_done = frames_total - frames;
		//qDebug() << "frames_done" << frames_done << "frames_total" << frames_total << "frames" << frames;
		usleep(20000);
	}
	m_wait_saving = false;

}

void Recording::UpdateInput() {
	assert(m_page_started);

	if(m_output_started) {
		StartInput();
	} else {
		StopInput();
	}

	// get sources
	VideoSource *video_source = NULL;
	AudioSource *audio_source = NULL;
	video_source = m_x11_input.get();

	if(m_audio_enabled) {
		if(m_audio_backend == AUDIO_BACKEND_PULSEAUDIO)
			audio_source = m_pulseaudio_input.get();
	}

	// connect sinks
	if(m_output_manager != NULL) {
		if(m_output_started) {
			m_output_manager->GetSynchronizer()->ConnectVideoSource(video_source, PRIORITY_RECORD);
			m_output_manager->GetSynchronizer()->ConnectAudioSource(audio_source, PRIORITY_RECORD);
		} else {
			m_output_manager->GetSynchronizer()->ConnectVideoSource(NULL);
			m_output_manager->GetSynchronizer()->ConnectAudioSource(NULL);
		}
	}
}

void Recording::OnRecordStart() {
	if(!TryStartPage())
		return;
	if(m_wait_saving)
		return;
	if(!m_output_started)
		StartOutput();
}

void Recording::OnRecordPause() {
	if(!m_page_started)
		return;
	if(m_wait_saving)
		return;
	if(m_output_started)
		StopOutput(false);
}

void Recording::OnRecordStartPause() {
	if(m_page_started && m_output_started) {
		OnRecordPause();
	} else {
		OnRecordStart();
	}
}


void Recording::OnRecordSave(bool confirm) {
    if(!m_page_started)
        return;
    if(m_wait_saving)
        return;
    if(!m_recorded_something && confirm) {      
		Logger::LogInfo("You haven't recorded anything, there is nothing to save.");
        return;
    }
    StopPage(true);   
}
