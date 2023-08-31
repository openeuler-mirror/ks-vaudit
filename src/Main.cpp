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

#include "Global.h"
#include "Benchmark.h"
#include "CommandLineOptions.h"
#include "CPUFeatures.h"
#include "Logger.h"
#include "Recording.h"
#include "EnumStrings.h"
#include "ScreenScaling.h"
#include "kiran-log/qt5-log-i.h"
#include "KyNotify.h"
#include <qapplication.h>
extern "C" {
#include<signal.h>
}



static std::unique_ptr<Recording> recording_screen = NULL;
QSettings* settings_ptr = NULL;


static void sig_handler(int sig){
	KLOG_INFO() << "receive signal, cur display:" << getenv("DISPLAY") << "sig:" << sig;
	if(sig == SIGINT){
		if(recording_screen->IsOutputStarted()){
			recording_screen->OnRecordSaveAndExit(true);
		}else{
			qApp->quit();
		}
	}
}

int main(int argc, char* argv[]) {
	//#59171 提权
	setuid(0);

	int klog_ret = klog_qt5_init("", "kylinsec-system", "ks-vaudit", "ks-vaudit");
	if (0 == klog_ret)
		KLOG_DEBUG() << "init klog succeed";

	KLOG_INFO() << "compile time: " << __DATE__ << __TIME__ << "pid:" << getpid();
	KLOG_INFO() << "DBUS_SESSION_BUS_ADDRESS:" << getenv("DBUS_SESSION_BUS_ADDRESS");
	setenv("DBUS_SESSION_BUS_ADDRESS", "", 1);
	KyNotify::instance();
	signal(SIGINT, sig_handler);
	XInitThreads();

	// Workarounds for broken screen scaling.
	ScreenScalingFix();

	QApplication application(argc, argv);
	KLOG_INFO() << "pid:" << getpid();

	// SSR uses two separate character encodings:
	// - UTF-8: Used for all internal strings.
	//   Used by QString::fromAscii and QString::toAscii, and all implicit conversions from C-strings to QString. Also used for translations.
	// - Local character encoding: Used for file names and logs. In practice this will almost always be UTF-8 as well.
	//   Used by QString::fromLocal8Bit and QString::toLocal8Bit.
	// If it is not clear what encoding an external library uses, I use the local encoding for file names and UTF-8 for everything else.
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
	QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
	QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
#endif

	// set the application name
	QCoreApplication::setOrganizationName("SimpleScreenRecorder");
	QCoreApplication::setApplicationName("SimpleScreenRecorder");

	// load Qt translations
	QTranslator translator_qt;
	if(translator_qt.load(QLocale::system(), "qt", "_", QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
		QApplication::installTranslator(&translator_qt);
	}

	// load SSR translations
	QTranslator translator_ssr;
	if(translator_ssr.load(QLocale::system(), "simplescreenrecorder", "_", QCoreApplication::applicationDirPath() + "/translations")) {
		QApplication::installTranslator(&translator_ssr);
	} else if(translator_ssr.load(QLocale::system(), "simplescreenrecorder", "_", GetApplicationSystemDir("translations"))) {
		QApplication::installTranslator(&translator_ssr);
	}

	// Qt doesn't count hidden windows, so if the main window is hidden and a dialog box is closed, Qt thinks the application should quit.
	// That's not what we want, so disable this and do it manually.
	//QApplication::setQuitOnLastWindowClosed(false);

	// create logger
	Logger logger;
	Q_UNUSED(logger);

	// parse command line options
	CommandLineOptions command_line_options;
	try {
		command_line_options.Parse();
	} catch(const CommandLineException&) {
		return 1;
	}

	// configure the logger
	if(!CommandLineOptions::GetLogFile().isEmpty()) {
		logger.SetLogFile(CommandLineOptions::GetLogFile());
	}
	if(CommandLineOptions::GetRedirectStderr()) {
		logger.RedirectStderr();
	}

	// start main program
	Logger::LogInfo("==================== " + Logger::tr("SSR started") + " ====================");
	Logger::LogInfo(GetVersionInfo());

#if SSR_USE_X86_ASM
	// detect CPU features
	CPUFeatures::Detect();
#endif

	// show screen scaling message
	ScreenScalingMessage();

	// load icons
	//LoadIcons();

	// start the program
	int ret = 0;
	if(CommandLineOptions::GetBenchmark()) {
		Benchmark();
	}
	pid_t selfPID = getpid();
	// 使用sudo service 启动monitor服务无法获取到准确的HomePath，统一存放到/tmp下
	QString settingPath = QString("%1%2%3").arg("/tmp/.ks-vaudit/setting-").arg(selfPID).arg(".conf");
	static QSettings settings(settingPath, QSettings::IniFormat);
	settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
	settings_ptr = &settings;
//	settings.clear();

	recording_screen.reset(new Recording(&settings));
	recording_screen->SaveSettings(&settings);
	if(!CommandLineOptions::GetFrontRecord()){
		if (!recording_screen->AuditParamDeal()) //监控程序启的进程默认不开始录屏，收到信号才录屏
			recording_screen->OnRecordStart(); //后台审计录屏， 前台只有收到开始录制信号信号后才录屏
	}
	
	// stop main program
	//Logger::LogInfo("==================== " + Logger::tr("SSR stopped") + " ====================");

	int result = application.exec();

	settings.clear();
	settings.sync();
	QFile::remove(settings.fileName());

	return result;
}
