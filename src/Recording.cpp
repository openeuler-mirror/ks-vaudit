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
#include "Logger.h"
#include "Muxer.h"
#include "VideoEncoder.h"
#include "AudioEncoder.h"
#include "Synchronizer.h"
#include "X11Input.h"
#include "SimpleSynth.h"
#include "KyNotify.h"
#include "kiran-log/qt5-log-i.h"
#include "kidletime.h"
#include <QAudioDeviceInfo>
#include <sys/inotify.h>

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
                        newfile += now.toString("yyyyMMdd_hhmmss");
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
	// 录制中的文件添加后缀 .tmp 不展示出来
	return newfile+".tmp";
}

//审计录屏文件以用户名_IP_YYYYMMDD_hhmmss命名，示例：张三_192.168.1.1_20220621_153620.mp4
static QString GetAuditNewSegmentFile(const QString& file, const QString &prefix, bool add_timestamp) {
	QFileInfo fi(file);
	QDateTime now = QDateTime::currentDateTime();
	QString newfile;
	unsigned int counter = 0;
	do {
		++counter;
		newfile = prefix;
		if (add_timestamp) {
			newfile += "_";
			newfile += now.toString("yyyyMMdd_hhmmss");
		}

		if (counter != 1) {
			newfile += "_";
			newfile += "(" + QString::number(counter) + ")";
		}

		if (!fi.suffix().isEmpty())
			newfile += "." + fi.suffix();
		newfile = fi.path() + "/" + newfile;
	} while (QFileInfo(newfile).exists());
	return newfile+".tmp";
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
	m_last_send_time = 0;

	//list all devices
	for(int i=0;i< m_pulseaudio_sources.size();i++){
		Logger::LogInfo("=============================================================================");
		Logger::LogInfo("the m_pulseaudio_sources name is " + QString::fromStdString(m_pulseaudio_sources[i].m_name));
		Logger::LogInfo("the m_pulseaudio_sources descriptions is " + QString::fromStdString(m_pulseaudio_sources[i].m_description));
		Logger::LogInfo("=============================================================================");
	}
	if(m_pulseaudio_sources.empty()){
		Logger::LogInfo("===================the sources list is empty==================================");
		m_audio_enabled = false;
	}
	m_settings = qsettings;

	m_configure_interface = new  ConfigureInterface(KSVAUDIT_CONFIGURE_SERVICE_NAME, KSVAUDIT_CONFIGURE_PATH_NAME, QDBusConnection::systemBus(), this);
	connect(m_configure_interface, SIGNAL(ConfigureChanged(QString, QString)), this, SLOT(UpdateConfigureData(QString, QString)));
	connect(m_configure_interface, SIGNAL(SignalSwitchControl(int,int, QString)), this, SLOT(SwitchControl(int,int,QString)));
	m_main_screen = qApp->primaryScreen();

	connect(m_main_screen, SIGNAL(geometryChanged(const QRect&)), this, SLOT(ScreenChangedHandler(const QRect&)));

	m_selfPID = getpid();
	m_recordUiPID = getppid();
	// 定时向前端发送录屏时间信息
	m_tm = new QTimer();
	m_tm->setInterval(1000);
	connect(m_tm, SIGNAL(timeout()), this, SLOT(OnRecordTimer()));
	m_tm->start();
	connect(this, SIGNAL(fileRemoved(bool)), this, SLOT(onFileRemove(bool)));
}


void Recording::OnRecordTimer() {
	if (m_output_manager == NULL) {
		return;
	}

	uint64_t total_time = (m_output_manager->GetSynchronizer() == NULL)? 0 : m_output_manager->GetSynchronizer()->GetTotalTime();
	if (total_time == m_last_send_time) {
		return;
	}
	m_last_send_time = total_time;
	QString msg = QString("totaltime ") + ReadableTime(total_time);
	m_configure_interface->SwitchControl(m_selfPID, m_recordUiPID, msg);
	KyNotify::instance().setRecordTime(total_time);
	// Logger::LogInfo("[Recording::OnRecordTimer] send msg:" + msg + " from:" + QString::number(m_selfPID) + " to:" + QString::number(m_recordUiPID));	
}

void Recording::ScreenChangedHandler(const QRect& hanged_screen_rect){
	//不录制屏幕 不处理
	if (!m_settings->value("input/video_enabled").toInt())
		return;

	//没有开始录屏但分辨率出现变动,只更新分辨率
	if(!m_page_started){
		Logger::LogInfo("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX \n");
		Logger::LogInfo("the screen geometry changed \n");
		Logger::LogInfo("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX \n");

		std::vector<QRect> screen_geometries = GetScreenGeometries();//重新计算所有显示屏的宽、高
		QRect rect = CombineScreenGeometries(screen_geometries);
		
		//update the qsettings
		settings_ptr->setValue("input/video_x", rect.left()); //X
		settings_ptr->setValue("input/video_y", rect.top()); //Y
		settings_ptr->setValue("input/video_w", rect.width()); //width
		settings_ptr->setValue("input/video_h", rect.height()); //height
		m_video_in_width = rect.width();
		m_video_in_height = rect.height();
		m_output_settings.video_height = m_video_in_height;
		m_output_settings.video_width = m_video_in_width;
		return ;
	}

	//分辨率变化后的处理
	m_separate_files = true;
	OnRecordPause();
	std::vector<QRect> screen_geometries = GetScreenGeometries();//重新计算所有显示屏的宽、高
	QRect rect = CombineScreenGeometries(screen_geometries); 
	m_video_in_width = rect.width();
	m_video_in_height = rect.height();
	m_output_settings.video_height = m_video_in_height;
	m_output_settings.video_width = m_video_in_width;
	OnRecordStartPause();
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
	m_video_x = m_settings->value("input/video_x", 0).toUInt();
	m_video_y = m_settings->value("input/video_y", 0).toUInt();
	m_video_in_width = m_settings->value("input/video_w", 800).toUInt();
	m_video_in_height = m_settings->value("input/video_h", 600).toUInt();
	m_video_frame_rate = m_settings->value("input/video_frame_rate", 30).toUInt();
	m_video_scaling = false;

	m_video_enabled =  m_settings->value("input/video_enabled").toInt();
	if(m_video_enabled){
		Logger::LogInfo("[Recording::StartPage] XXXXXXXXXXXXXXXXXXXX 开启视频录制 XXXXXXXXXXXXXXXXXXXX");
	}

	if (!m_video_enabled)
	{
		m_video_in_width = 80;
		m_video_in_height = 60;
	}

	// 音频相关设置
	QString defualtAudioInput = QAudioDeviceInfo::defaultInputDevice().deviceName();
	QString defualtAudioOutput = QAudioDeviceInfo::defaultOutputDevice().deviceName();

	m_pulseaudio_source_input = "";
	m_pulseaudio_source_output = "";
	m_audio_enabled = false;
	m_audio_recordtype = "none";
	m_pulseaudio_sources = PulseAudioInput::GetSourceList();

	QString audio_enabled = m_settings->value("input/audio_enabled").toString();
	if (audio_enabled == "mic" || audio_enabled == "all"){ //录制麦克风
		if (!defualtAudioInput.isEmpty() && defualtAudioInput.contains("alsa_input")) {
			// 还有两种设备:alsa_null、alsa_output（输出设备）
			// 如果没有实际输入设备(alsa_input)、会把带"monitor"字段的output设备(扬声器设备)识别defualtAudioInput
			// 因此只判断是否有input关键字 完全区分开麦克风和扬声器 方便后续合成算法
			m_audio_enabled = true;
			m_pulseaudio_source_input = defualtAudioInput;
		}
	}
	if(audio_enabled == "speaker" || audio_enabled == "all"){ //扬声器
		if (m_pulseaudio_sources.size() > 0 && !defualtAudioOutput.isEmpty() && defualtAudioOutput.contains("alsa_output")){
			// 同上alsa_input逻辑，避免重复选择同一个设备
			for(auto asource : m_pulseaudio_sources){
				if (QString::fromStdString(asource.m_name).contains("monitor") && QString::fromStdString(asource.m_name).contains(defualtAudioOutput)){
					m_audio_enabled = true;
					m_pulseaudio_source_output = QString::fromStdString(asource.m_name);
					break;
				}
			}
		}
	}
	if (m_pulseaudio_source_input != "" && m_pulseaudio_source_output != "") {
		m_audio_recordtype = "all";
	} else if (m_pulseaudio_source_input != "") {
		m_audio_recordtype = "mic";
	} else if (m_pulseaudio_source_output != "") {
		m_audio_recordtype = "speaker";
	}

	m_audio_channels = 2;
	m_audio_sample_rate = 8000;
	m_audio_backend = AUDIO_BACKEND_PULSEAUDIO;

	// get file settings
	m_file_base = m_settings->value("output/file").toString();

	m_separate_files = false;
	m_add_timestamp = true;

	// get the output settings
	m_output_settings.file = QString(""); // will be set later
	if(m_settings->value("output/container_av", QString()).toString() == "ogv"){
		m_output_settings.container_avname = "ogg";
	}else{
		m_output_settings.container_avname = m_settings->value("output/container_av", QString()).toString();
	}

	m_output_settings.video_codec_avname = m_settings->value("output/video_codec_av", QString()).toString();
	// Logger::LogInfo("[Recording::StartPage] the m_output_settings.video_codec_avname is " + m_output_settings.video_codec_avname);

	m_output_settings.video_kbit_rate = 128;
	m_output_settings.video_width = m_video_in_width;
	m_output_settings.video_height = m_video_in_height;
	m_output_settings.video_frame_rate = m_video_frame_rate;
	m_output_settings.video_allow_frame_skipping = true;
	m_output_settings.audio_codec_avname = (m_audio_enabled)? m_settings->value("output/audio_codec_av", QString()).toString():QString();
	if(m_audio_enabled){
		Logger::LogInfo("[Recording::StartPage] XXXXXXXXXXXXXXXXXXXX 开启音频录制 XXXXXXXXXXXXXXXXXXXX");
	}else{
		Logger::LogInfo("[Recording::StartPage] XXXXXXXXXXXXXXXXXXXX 未开启音频录制 XXXXXXXXXXXXXXXXXXXX");
	}
	Logger::LogInfo("[Recording::StartPage] the audio_codec_avname is: " + m_output_settings.audio_codec_avname);
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
	//QApplication::sync();
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

	//通过DBus 接口从配置中心读取
	QString value;
	if(!CommandLineOptions::GetFrontRecord()){ //后台审计
		value = m_configure_interface->GetAuditInfo();
	}else{ //前台录屏
		value = m_configure_interface->GetRecordInfo();
	}
	QJsonDocument doc = QJsonDocument::fromJson(value.toUtf8());

	if(!doc.isObject()){
		Logger::LogError("Cann't get the DBus configure!");
		return;
	}

	QJsonObject jsonObj = doc.object();
	for(auto key:jsonObj.keys()){
		Logger::LogInfo("[Recording::SaveSettings] --------------keys and value is -------------- " + key + "  " + jsonObj[key].toString());
	}

	//bool ret = m_configure_interface->SetRecordItemValue("{\"Fps\": \"7\", \"FileType\":\"mp4\"}");


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

	QString key("Fps");	
	settings->setValue("input/video_frame_rate", jsonObj[key].toString().toInt()); //帧率
	settings->setValue("input/video_scale", false);
	settings->setValue("input/video_scaled_w", 854);
	settings->setValue("input/video_scaled_h", 480);
	settings->setValue("input/video_record_cursor", false);

	key = "RecordAudio";
	QString audio_codec_name = "";
	QString video_type = jsonObj["FileType"].toString().toLower();
	if(QString::compare(video_type, "mp4", Qt::CaseInsensitive) == 0) {
		audio_codec_name = "aac";
	}else if (QString::compare(video_type, "ogv", Qt::CaseInsensitive) == 0){
		audio_codec_name = "libvorbis";
	}else{
		audio_codec_name = "";
	}

	settings->setValue("output/audio_codec_av", audio_codec_name);
	settings->setValue("input/audio_enabled",jsonObj[key].toString()); //音频录制
	settings->setValue("input/audio_backend", EnumToString(Recording::AUDIO_BACKEND_PULSEAUDIO));
	//settings->setValue("input/audio_pulseaudio_source", GetPulseAudioSourceName());
	Logger::LogInfo("[SaveSettings] the audio source name is" + GetPulseAudioSourceName());

	key = "RecordVideo";
	settings->setValue("input/video_enabled",jsonObj[key].toString().toInt()); //是否录制视频
	//QString::fromStdString(m_pulseaudio_sources[0].m_name);

	//输出页面配置参数
	key = "FilePath";
	QString sys_user(getenv("USER"));
	QString file_path(jsonObj[key].toString());

	//file_path = "~/test/demo/yefeng/women/hello/world/XXXXX/"; //测试用
	if(file_path.contains('~')){
		QString home_dir = getenv("HOME");
		file_path.replace('~', home_dir);
		//Logger::LogInfo("===============> " + file_path + "XXXXXXXXXXXXXX");

	}

	if(!QFile::exists(file_path)){ //存放目录不存在时创建一个
		QDir dir;
		if(!dir.exists(file_path)){
			if(dir.mkpath(file_path) == false){
				Logger::LogError("can't mkdir the file path " + file_path);
				exit(-1);
			}
		}
	}

	key = "FileType";
	QString file_type = jsonObj[key].toString().toLower();
	QString file_suffix;

	if(QString::compare(file_type, "mp4", Qt::CaseInsensitive) == 0) {
		file_suffix = ".mp4";

		settings->setValue("output/container_av", file_type); //mp4 ogv 格式等
		settings->setValue("output/container", EnumToString(Recording::CONTAINER_MP4));
		settings->setValue("output/video_codec", EnumToString(Recording::VIDEO_CODEC_H264));
		settings->setValue("output/video_codec_av", "libx264");
		settings->setValue("output/video_kbit_rate", 128);
		settings->setValue("output/video_h264_crf", 23);
		settings->setValue("output/video_h264_preset", (Recording::enum_h264_preset)Recording::H264_PRESET_SUPERFAST);
	}else if(QString::compare(file_type, "ogv", Qt::CaseInsensitive) == 0){
		file_suffix = ".ogv";

		settings->setValue("output/container_av", QString("ogv")); //mp4 ogv 格式等
		settings->setValue("output/container", EnumToString(Recording::CONTAINER_OGG));
		settings->setValue("output/video_codec", EnumToString(Recording::VIDEO_CODEC_VP8));
		settings->setValue("output/video_codec_av", "libvpx"); //硬件加速用h264_vaapi
		settings->setValue("output/video_kbit_rate", 128);
		settings->setValue("output/video_h264_preset", (Recording::enum_h264_preset)Recording::H264_PRESET_SUPERFAST);
	}else{
		Logger::LogError("the video codec error \n");
		qApp->quit();
	}
//	settings->setValue("output/file", file_path + "/" + sys_user + file_suffix); //只能用绝对路径， 有空优化一下这地方
    settings->setValue("output/file", file_path + "/" + file_suffix); //只能用绝对路径， 有空优化一下这地方
	settings->setValue("output/separate_files", true);
	settings->setValue("output/add_timestamp", true);
	settings->setValue("output/video_allow_frame_skipping", true);

	//水印相关设置
	key = "WaterPrint";
	if(jsonObj[key].toString().toInt() == 0){
		settings->setValue("record/is_use_watermark", 0);
		key = "WaterPrintText";
		settings->setValue("record/water_print_text", jsonObj[key].toString());
	}else{
		settings->setValue("record/is_use_watermark", 1);
		key = "WaterPrintText";
		settings->setValue("record/water_print_text", jsonObj[key].toString());
	}

	//Logger::LogInfo(" --------------the waterprintText is -------------- " + settings ->value("record/water_print_text").toString());

	settings->setValue("record/hotkey_enable", false); //禁用快捷键
	settings->setValue("record/hotkey_ctrl", false);
	settings->setValue("record/hotkey_shift", false);
	settings->setValue("record/hotkey_alt",false);
	settings->setValue("record/hotkey_super",false);

	// save encode Quality
	settings->setValue("encode/quality", jsonObj["Quality"].toString());
	m_output_settings.encode_quality = jsonObj["Quality"].toString();

	key = "TimingReminder";
	KyNotify::instance().setTiming(jsonObj[key].toString().toInt());

	if (!CommandLineOptions::GetFrontRecord())
	{
		key = "TimingPause";
		m_timingPause = jsonObj[key].toString().toInt();
		key = "MinFreeSpace";
		KyNotify::instance().setReserveSize(jsonObj[key].toString().toULongLong());
	}
}

void Recording::StopPage(bool save) {

	if(!m_page_started)
		return;

	m_bStopRecord = true;
	StopOutput(true);
	StopInput();

	Logger::LogInfo("[PageRecord::StopPage] " + tr("Stopping page ..."));

	if(m_output_manager != NULL) {

		// stop the output
		if(save)
			FinishOutput();
		m_output_manager.reset();

		// delete the file if it isn't needed
		// 原ssr界面里取消保存的功能
		if(!save && m_file_protocol.isNull()) {
			if(QFileInfo(m_output_settings.file).exists())
				QFile(m_output_settings.file).remove();
		}
		else
		{
			// #60874 前端无法获取准确的正在录制的视频时长，因此正在录制的视频后缀为.tmp
			// 完成录制后移除.tmp后缀
			QString fileName = m_output_settings.file;
			QString fileBaseName = fileName.left(fileName.lastIndexOf("."));
			if (QFile(fileName).exists()){
				QFile(fileName).rename(fileName, fileBaseName);
			}
//			Logger::LogInfo("************** " + fileName + " | " + fileBaseName);
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
}

void Recording::StartOutput() {
	if(m_output_started)
		return;

	try {

		Logger::LogInfo("[PageRecord::StartOutput] " + tr("Starting output ..."));

		if(m_output_manager == NULL) {

			// set the file name
			if (m_auditBaseFileName.isEmpty())
				m_output_settings.file = GetNewSegmentFile(m_file_base, m_add_timestamp);
			else
			{
				m_output_settings.file = GetAuditNewSegmentFile(m_file_base, m_auditBaseFileName, m_add_timestamp);
				m_configure_interface->MonitorNotification(getpid(), m_output_settings.file);
			}

			KLOG_DEBUG() << "m_output_settings.file:" << m_output_settings.file;

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
			//if(m_x11_input != NULL)
			//	m_x11_input->GetCurrentSize(&m_video_in_width, &m_video_in_height);

			// calculate the output width and height

				// If the user did not explicitly select scaling, then don't force scaling just because the recording area is one pixel too large.
				// One missing row/column of pixels is probably better than a blurry video (and scaling is SLOW).
			//m_video_in_width = m_video_in_width / 2 * 2;
			//m_video_in_height = m_video_in_height / 2 * 2;
			m_output_settings.video_width = m_video_in_width;
			m_output_settings.video_height = m_video_in_height;

			// start the output
			//m_output_settings.container_avname
			//Logger::LogInfo("the m_output_settings.container_avname is --->" + m_output_settings.container_avname);
			//Logger::LogInfo("the m_output_settings.video_codec_avname  is --->" + m_output_settings.video_codec_avname);
			m_output_manager.reset(new OutputManager(m_output_settings));
			WatchFile();

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

		if(m_video_enabled){
			Logger::LogInfo("[Recording::StartInput] start recording video");
			// start the video input
			if(m_video_area == VIDEO_AREA_SCREEN || m_video_area == VIDEO_AREA_FIXED || m_video_area == VIDEO_AREA_CURSOR) {
				m_x11_input.reset(new X11Input(m_video_x, m_video_y, m_video_in_width, m_video_in_height, true,
							m_video_area == VIDEO_AREA_CURSOR, m_video_area_follow_fullscreen));

				//connect(m_x11_input.get(), SIGNAL(CurrentRectangleChanged()), this, SLOT(OnUpdateRecordingFrame()), Qt::QueuedConnection);
			}

		}
		else
		{
			m_video_enabled = true;
			m_x11_input.reset(new X11Input(m_video_x, m_video_y, m_video_in_width, m_video_in_width, false, false, false, false, true));
		}


		// start the audio input
		if(m_audio_enabled) {
			if(m_audio_backend == AUDIO_BACKEND_PULSEAUDIO) {
				Logger::LogInfo("[Recording::StartInput] m_audio_recordtype is: " + m_audio_recordtype +  "  m_pulseaudio_source_input: " + m_pulseaudio_source_input + "  m_pulseaudio_source_output: " + m_pulseaudio_source_output);
				// 这里recordtype只用来判断是否需要new对象 这里需要加上{}
				if (m_audio_recordtype == "mic" || m_audio_recordtype == "all"){
					m_pulseaudio_input.reset(new PulseAudioInput(m_pulseaudio_source_input, m_audio_sample_rate, "mic"));
				}
				if (m_audio_recordtype == "speaker" || m_audio_recordtype == "all"){
					m_pulseaudio_output.reset(new PulseAudioInput(m_pulseaudio_source_output, m_audio_sample_rate, "speaker"));
				}

			}
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
		m_pulseaudio_output.reset();
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
	KLOG_DEBUG() << "start m_x11_input reset";
	m_x11_input.reset();
	KLOG_DEBUG() << "end m_x11_input reset";
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
	m_pulseaudio_output.reset();
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
	AudioSourceInput *audio_source_input = NULL;
	AudioSource *audio_source_output = NULL;

	video_source = m_x11_input.get();

	if(m_audio_enabled) {
		if(m_audio_backend == AUDIO_BACKEND_PULSEAUDIO) {
			audio_source_input = m_pulseaudio_input.get();
			audio_source_output = m_pulseaudio_output.get();
		}
	}

	// connect sinks
	if(m_output_manager != NULL) {
		if(m_output_started) {
			m_output_manager->GetSynchronizer()->ConnectVideoSource(video_source, PRIORITY_RECORD);
			if (m_audio_recordtype == "mic" || m_audio_recordtype == "all"){
				m_output_manager->GetSynchronizer()->ConnectAudioSourceInput(audio_source_input, PRIORITY_RECORD);
			} else {
				m_output_manager->GetSynchronizer()->ConnectAudioSourceInput(NULL);
			}

			if (m_audio_recordtype == "speaker" || m_audio_recordtype == "all") {
				m_output_manager->GetSynchronizer()->ConnectAudioSource(audio_source_output, PRIORITY_RECORD);
			} else {
				m_output_manager->GetSynchronizer()->ConnectAudioSource(NULL);
			}
		} else {
			m_output_manager->GetSynchronizer()->ConnectVideoSource(NULL);
			m_output_manager->GetSynchronizer()->ConnectAudioSourceInput(NULL);
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

void Recording::OnRecordSaveAndExit(bool confirm) {
    if(!m_page_started)
        return;
    if(m_wait_saving)
        return;
    if(!m_recorded_something && confirm) {
		Logger::LogInfo("You haven't recorded anything, there is nothing to save.");
        return;
    }
    StopPage(true);
    qApp->quit();
}

void Recording::UpdateResolutionParameter(){
	std::vector<QRect> screen_geometries = GetScreenGeometries();
	QRect rect = CombineScreenGeometries(screen_geometries); //录制屏幕的矩形区域
	//m_video_in_width = 2560;
	//m_video_in_height = 1440;
	m_output_settings.video_width = m_video_in_width;
	m_output_settings.video_height = m_video_in_height;
}

void Recording::OnRecordRestart()	{
	OnRecordPause(); //暂停录屏
	UpdateResolutionParameter(); //更新分辨率相关参数参数
	OnRecordStart(); //重新开始录制
}


/**
 * 配置发生改变
 **/
void Recording::UpdateConfigureData(QString key, QString value){
	//Logger::LogInfo("%%%%%%%%%%%%%  UpdateConfigureData 信号槽函数 %%%%%%%%%%%%%%");
	//Logger::LogInfo("the first is " + key + "the second is " + value);
	if(key == "record"){
		QJsonDocument doc = QJsonDocument::fromJson(value.toUtf8());
		if(!doc.isObject()){
			Logger::LogError("Cann't get the DBus configure!");
			return;
		}

		QJsonObject jsonObj = doc.object();
		for(auto key:jsonObj.keys()){
			Logger::LogInfo("[Recording::UpdateConfigureData] --------------keys and value is -------------" + key + "==========" + jsonObj[key].toString());

			//修改settings
			if(key == "Fps"){
				m_settings->setValue("input/video_frame_rate", jsonObj[key].toString().toInt());
			}else if(key == "RecordAudio"){
				m_settings->setValue("input/audio_enabled",jsonObj[key].toString());
			}else if(key == "FilePath" || key == "FileType"){
				// 修改fileType需要一起修改filePath
				QString file_path(jsonObj[key].toString());
				QString file_suffix;
				QString fileType = jsonObj["FileType"].toString();
				if(QString::compare(fileType, "mp4", Qt::CaseInsensitive) == 0){
					file_suffix = ".mp4";
					m_settings->setValue("output/container_av", "mp4");
				}else if (QString::compare(fileType, "ogv", Qt::CaseInsensitive) == 0){
					file_suffix = ".ogv";
					m_settings->setValue("output/container_av", "ogv");
				}else{
					KLOG_DEBUG() << "container_av 没有这种配置:" << fileType;
				}
				m_settings->setValue("output/file", file_path + "/" + file_suffix);
				SetFileTypeSetting();
			}else if(key == "Quality"){
				m_settings->setValue("encode/quality", jsonObj[key].toString());
				m_output_settings.encode_quality = jsonObj["Quality"].toString();
			} else if (key == "TimingReminder") {
				KyNotify::instance().setTiming(jsonObj[key].toString().toInt());
			}else if(key == "WaterPrint"){
				m_settings->setValue("record/is_use_watermark", jsonObj[key].toString().toInt());
			}else if(key == "WaterPrintText"){
				m_settings->setValue("record/water_print_text", jsonObj[key].toString());
			}else if(key == "RecordVideo"){ //更新是否开启视频配置
				m_settings->setValue("input/video_enabled", jsonObj[key].toString().toInt());
			}
		}
	}else { //后台审计
		QJsonDocument doc = QJsonDocument::fromJson(value.toUtf8());
		if(!doc.isObject()){
			Logger::LogError("Cann't get the DBus configure!");
			return;
		}

		QJsonObject jsonObj = doc.object();
		for(auto key:jsonObj.keys()){
			Logger::LogInfo("[Recording::UpdateConfigureData] --------------keys and value is --------------" + key + "==========" + jsonObj[key].toString());

			//修改settings
			if(key == "Fps"){
				m_settings->setValue("input/video_frame_rate", jsonObj[key].toString().toInt());
			}else if(key == "RecordAudio"){
				m_settings->setValue("input/audio_enabled",jsonObj[key].toString());
			}else if(key == "FilePath" || key == "FileType"){
				// 修改fileType需要一起修改filePath
				QString file_path(jsonObj[key].toString());
				QString file_suffix;
				QString fileType = jsonObj["FileType"].toString();
				if(QString::compare(fileType, "mp4", Qt::CaseInsensitive) == 0){
					file_suffix = ".mp4";
					m_settings->setValue("output/container_av", "mp4");
				}else if (QString::compare(fileType, "ogv", Qt::CaseInsensitive) == 0){
					file_suffix = ".ogv";
					m_settings->setValue("output/container_av", "ogv");
				}else{
					KLOG_DEBUG() << "container_av 没有这种配置:" << fileType;
				}
				m_settings->setValue("output/file", file_path + "/" + file_suffix);
				SetFileTypeSetting();
			}else if(key == "WaterPrint"){
				m_settings->setValue("record/is_use_watermark", jsonObj[key].toString().toInt());
			}else if(key == "WaterPrintText"){
				m_settings->setValue("record/water_print_text", jsonObj[key].toString());
			} else if (key == "TimingPause") {
				m_timingPause = jsonObj[key].toString().toInt();
			} else if (key == "MinFreeSpace") {
				KyNotify::instance().setReserveSize(jsonObj[key].toString().toULongLong());
			}

		}
	}
}

/**
 *
 **/
void Recording::SwitchControl(int from_pid,int to_pid,QString op){
	//Logger::LogInfo("%%%%%%%%% switchControl 信号槽函数 %%%%%%%%%%%%%%%%%%%%%%%%%");
	if(to_pid != getpid()){
		return;
	}
	//start stop restart exit
	if(op == "start"){
		Logger::LogInfo("[Recording::SwitchControl] start record");
		OnRecordStart();
	}else if(op == "pause"){
		Logger::LogInfo("[Recording::SwitchControl] pause record");
		OnRecordPause();
	}else if(op == "restart"){
		Logger::LogInfo("[Recording::SwitchControl] restart record");
		OnRecordStartPause();
	}else if(op == "stop"){
		Logger::LogInfo("[Recording::SwitchControl] stop record");
		OnRecordSave(); //结束视频录制但不退出
	}else if(op == "exit"){
		Logger::LogInfo("[Recording::SwitchControl] exit signal");
		OnRecordSaveAndExit(true);
		exit(0);
	} else if(op == "disk_notify"){
		KyNotify::instance().sendNotify(op);
	} else if(op == "disk_notify_stop") {
		KyNotify::instance().setContinueNotify(false);
	}

	//后台审计不需要提示
	if (m_auditBaseFileName.isEmpty())
		KyNotify::instance().sendNotify(op);
}

bool Recording::AuditParamDeal()
{
	QStringList args = QCoreApplication::arguments();
	KLOG_DEBUG() << "arguments:" << args << "cur display:" << getenv("DISPLAY");
	if (args.size() != 2)
		return false;

	//后台审计不需要发送录屏时间
	if (m_tm && m_tm->isActive())
		m_tm->stop();

	m_auditFirstStart = true;
	QStringList strlist = args[1].split("-");
	strlist.removeAll("");
	m_auditBaseFileName = strlist[1] + "_" + strlist[2];
	connect(&KIdleTime::instance(), &KIdleTime::resumingFromIdle, this, &Recording::kidleResumeEvent);
	connect(&KIdleTime::instance(), &KIdleTime::timeoutReached, this, &Recording::kidleTimeoutReached);
	KIdleTime::instance().catchNextResumeEvent();
	KLOG_DEBUG() << m_auditBaseFileName << "idleTime:" << KIdleTime::instance().idleTime();
	return true;
}

void Recording::SetFileTypeSetting()
{
	//通过DBus 接口从配置中心读取
	QString value;
	if(!CommandLineOptions::GetFrontRecord()){ //后台审计
		value = m_configure_interface->GetAuditInfo();
	}else{ //前台录屏
		value = m_configure_interface->GetRecordInfo();
	}
	QJsonDocument doc = QJsonDocument::fromJson(value.toUtf8());

	if(!doc.isObject()){
		Logger::LogError("[Recording:SetFileTypeSetting]: Cann't get the DBus configure!");
		return;
	}

	QJsonObject jsonObj = doc.object();

	QString file_type = jsonObj["FileType"].toString().toLower();
	QString file_suffix;

	if(QString::compare(file_type, "mp4", Qt::CaseInsensitive) == 0) {
		file_suffix = ".mp4";

		m_settings->setValue("output/audio_codec_av", "aac");
		m_settings->setValue("output/container_av", file_type); //mp4 ogv 格式等
		m_settings->setValue("output/container", EnumToString(Recording::CONTAINER_MP4));
		m_settings->setValue("output/video_codec", EnumToString(Recording::VIDEO_CODEC_H264));
		m_settings->setValue("output/video_codec_av", "libx264");
		m_settings->setValue("output/video_kbit_rate", 128);
		m_settings->setValue("output/video_h264_crf", 23);
		m_settings->setValue("output/video_h264_preset", (Recording::enum_h264_preset)Recording::H264_PRESET_SUPERFAST);
	}else if(QString::compare(file_type, "ogv", Qt::CaseInsensitive) == 0){
		file_suffix = ".ogv";

		m_settings->setValue("output/audio_codec_av", "libvorbis");
		m_settings->setValue("output/container_av", QString("ogv")); //mp4 ogv 格式等
		m_settings->setValue("output/container", EnumToString(Recording::CONTAINER_OGG));
		m_settings->setValue("output/video_codec", EnumToString(Recording::VIDEO_CODEC_VP8));
		m_settings->setValue("output/video_codec_av", "libvpx"); //硬件加速用h264_vaapi
		m_settings->setValue("output/video_kbit_rate", 128);
		m_settings->setValue("output/video_h264_preset", (Recording::enum_h264_preset)Recording::H264_PRESET_SUPERFAST);
	}else{
		Logger::LogError("[Recording:SetFileTypeSetting]: The container name error: "+file_type);
		return;
	}
	QString file_path(jsonObj["FilePath"].toString());

	if(file_path.contains('~')){
		QString home_dir = getenv("HOME");
		file_path.replace('~', home_dir);
	}

	if(!QFile::exists(file_path)){ //存放目录不存在时创建一个
		QDir dir;
		if(!dir.exists(file_path)){
			if(dir.mkpath(file_path) == false){
				Logger::LogError("[Recording:SetFileTypeSetting]: Can't mkdir the file path " + file_path);
				return;
			}
		}
	}

	m_settings->setValue("output/file", file_path + "/" + file_suffix); //只能用绝对路径， 有空优化一下这地方
}

void Recording::kidleResumeEvent()
{
	KLOG_DEBUG() << "m_timingPause:" << m_timingPause << "cur display:" << getenv("DISPLAY") << m_auditBaseFileName;
	KIdleTime::instance().removeAllIdleTimeouts();
	KIdleTime::instance().addIdleTimeout(m_timingPause*60000);
	if (!m_auditFirstStart && !KyNotify::instance().getContinueNotify())
		OnRecordStartPause();

	m_auditFirstStart = false;
}

void Recording::kidleTimeoutReached(int id, int timeout)
{
	KLOG_DEBUG() << "id:" << id << "timeout:" << timeout << "cur display:" << getenv("DISPLAY") << m_auditBaseFileName;
	KIdleTime::instance().catchNextResumeEvent();
	OnRecordPause();
}

//不使用 while 循环原因：非删除文件事件下read不阻塞了
void thread_function(QString fileName, void *user)
{
	Recording *pThis = (Recording *)user;
	int fd = inotify_init();
	if (fd < 0)
	{
		KLOG_INFO() << "inotify_init failed" << errno << strerror(errno);
		return;
	}

	//IN_DELETE	| IN_DELETE_SELF 对于删除没作用， IN_ATTRIB 删除有作用
	int	wd = inotify_add_watch(fd, fileName.toStdString().data(), IN_ATTRIB | IN_CLOSE_WRITE);
	if (wd < 0)
	{
		KLOG_INFO() << "inotify_add_watch failed" << fileName.toStdString().data() << errno << strerror(errno);
		pThis->fileRemoved(true);
		close(fd);
		return;
	}

	int nread = 0;
	char buf[BUFSIZ]{};
	struct inotify_event *event;
	int length = read(fd, buf, sizeof(buf) - 1);

	// inotify 事件发生时
	while (length > 0)
	{
		event = (struct inotify_event *)&buf[nread];
		if (pThis->m_bStopRecord)
		{
			KLOG_DEBUG("stop record, event->mask:%#x", event->mask);
			inotify_rm_watch(fd, wd);
			close(fd);
			return;
		}
		nread = nread + sizeof(struct inotify_event) + event->len;
		length = length - sizeof(struct inotify_event) - event->len;
		KLOG_DEBUG("event->mask:%#x, nread:%d, length:%d", event->mask, nread, length);
	}

	//非删除文件和停止录屏，重新监控文件
	inotify_rm_watch (fd, wd);
	close(fd);
	pThis->fileRemoved(false);
	return;
}

void Recording::WatchFile()
{
	KLOG_DEBUG() << "m_output_settings.file:" << m_output_settings.file;
	m_bStopRecord = false;
	std::thread t(&thread_function, m_output_settings.file, this);
	t.detach();
}

void Recording::onFileRemove(bool bRemove)
{
	if (bRemove)
	{
		KLOG_INFO() << "file:" << m_output_settings.file << "is deleted, restart the recording";
		OnRecordSave();  // 结束视频录制
		OnRecordStart(); // 开始新视频
	}
	else
	{
		KLOG_INFO() << "re-watch file" << m_output_settings.file;
		WatchFile();
	}
}
