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
#include "configure_interface.h"
#include "daemon/configure/include/ksvaudit-configure_global.h"

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
	bool m_pause_state; //判断是否处于暂停状态
	bool m_recorded_something, m_wait_saving, m_error_occurred;

	enum_video_area m_video_area;
	bool m_video_area_follow_fullscreen;
	unsigned int m_video_x, m_video_y, m_video_in_width, m_video_in_height;
	unsigned int m_video_frame_rate;
	bool m_video_scaling;
	bool m_video_enabled; //是否录制视频
	bool m_audio_enabled; //是否录制音频
	QString m_audio_recordtype;
	std::vector<PulseAudioInput::Source> m_pulseaudio_sources;

	unsigned int m_audio_channels, m_audio_sample_rate;
	enum_audio_backend m_audio_backend;

	QString m_pulseaudio_source_input;
	QString m_pulseaudio_source_output;

	OutputSettings m_output_settings;
	std::unique_ptr<OutputManager> m_output_manager;

	QString m_file_base;
	QString m_file_protocol;
	bool m_separate_files; //暂停录制时， 录屏mp4文件是否分开
	bool m_add_timestamp;  //视频中添加时间戳

	std::unique_ptr<X11Input> m_x11_input;
	std::unique_ptr<PulseAudioInput> m_pulseaudio_input;
	std::unique_ptr<PulseAudioInput> m_pulseaudio_output;
	QSettings* m_settings;
	ConfigureInterface* m_configure_interface;

	QScreen* m_main_screen; //监测分辨率

	// 用于定时向前端发送录屏时间信息
	uint64_t m_last_send_time;
	pid_t m_recordUiPID;
	pid_t m_selfPID;
	QTimer *m_tm;

	// 用于审计录屏
	int m_timingPause;
	QString m_auditBaseFileName;
	QTimer *m_IdleTimer;
	bool m_auditDiskEnough;
	quint64 m_lastMinFreeSpace;
	QProcess *m_pNotifyProcess;

	//音频格式
	QString m_lastAlsaInput;
	QString m_lastAlsaOutput;
	QTimer *m_audioTimer;

public:
	// 是否停止录像，用于结束监测文件是否被删除
	bool m_bStopRecord;

public:
	Recording(QSettings* qsettings);
	~Recording();

	void OnRecordStart();  //开始录制
	void OnRecordPause(); //暂停录制
	void OnRecordStartPause(); //暂停后再次开始录制
	void OnRecordSave(bool confirm = true); //结束录制并刷新mp4容器
	void OnRecordSaveAndExit(bool confirm);  //保存并退出
	void ReNameFile();
	QString GetPulseAudioSourceName() {     return QString::fromStdString(m_pulseaudio_sources[0].m_name);}
	std::vector<QRect> GetScreenGeometries();
	QRect CombineScreenGeometries(const std::vector<QRect>& screen_geometries);
	void SaveSettings(QSettings* settings);
	bool IsOutputStarted(){return m_output_started ;}
	void AuditParamDeal();
	void SetFileTypeSetting();

private:
	void FinishOutput();
	void UpdateInput();
	bool TryStartPage();
	void StartPage();
	void StopPage(bool save);
	void StartOutput();
	void StopOutput(bool final);
	void StartInput();
	void StopInput();
	void WatchFile();
	bool parseJsonData(const QString &param,  QJsonObject &jsonObj);
	void operateCatchResume(bool bStartCatch = false, bool bRestartRecord = false);
	void callNotifyProcess();
	void clearNotify();
	QString getDbusSession();

signals:
	void fileRemoved(bool bRemove);

public slots:
	void UpdateConfigureData(QString, QString); //配置发生变化 响应槽
	void SwitchControl(int, int, QString); //启动、停止等控制开关 响应槽
	void ScreenChangedHandler(const QRect&);
	void ScreenAddedOrRemovedHandler(QScreen*);
	// 用于定时向前端发送录屏时间信息
	void OnRecordTimer();
	void OnAudioTimer();

private slots:
	//后台录屏无操作处理
	void kidleResumeEvent();
	void kidleTimeoutReached(int id, int timeout);
	void onFileRemove(bool bRemove);
	void OnIdleTimer();
};
