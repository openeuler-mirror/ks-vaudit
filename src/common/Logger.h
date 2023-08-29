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

class Logger : public QObject {
	Q_OBJECT

public:
	enum enum_type {
		TYPE_INFO,
		TYPE_WARNING,
		TYPE_ERROR,
		TYPE_STDERR
	};

private:
	std::mutex m_mutex;
	QFile m_log_file;

	std::thread m_capture_thread;
	int m_capture_pipe[2], m_shutdown_pipe[2], m_original_stderr;

	static Logger *s_instance;

public:
	Logger();
	~Logger();

	void SetLogFile(const QString& filename);
	void RedirectStderr();

	// These functions are thread-safe.
	static void LogInfo(const QString& str);
	static void LogWarning(const QString& str);
	static void LogError(const QString& str);

	inline static Logger* GetInstance() { assert(s_instance != NULL); return s_instance; }

signals:
	void NewLine(Logger::enum_type type, QString str);

private:
	void CaptureThread();

};

// #define NVENC_ENCODE_DEBUG
#ifdef NVENC_ENCODE_DEBUG

// test function for save local bmp
inline void WriteBmp(const uint8_t *image, int imageWidth, int imageHeight)
{
	static int count = 0;
	char fname_bmp[128] = {0};
	snprintf(fname_bmp, sizeof(fname_bmp), "/tmp/ramfs/vaudit-%d.bmp", count);

	unsigned char header[54] = {
		0x42, 0x4d, 0, 0, 0, 0, 0, 0, 0, 0,
		54, 0, 0, 0, 40, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 32, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0
	};

	long file_size = (long)imageWidth * (long)imageHeight * 4 + 54;
	header[2] = (unsigned char)(file_size &0x000000ff);
	header[3] = (file_size >> 8) & 0x000000ff;
	header[4] = (file_size >> 16) & 0x000000ff;
	header[5] = (file_size >> 24) & 0x000000ff;

	long width = imageWidth;
	header[18] = width & 0x000000ff;
	header[19] = (width >> 8) &0x000000ff;
	header[20] = (width >> 16) &0x000000ff;
	header[21] = (width >> 24) &0x000000ff;

	long height = imageHeight;
	header[22] = height &0x000000ff;
	header[23] = (height >> 8) &0x000000ff;
	header[24] = (height >> 16) &0x000000ff;
	header[25] = (height >> 24) &0x000000ff;

	FILE *fp;
	if (!(fp = fopen(fname_bmp, "wb"))) {
		Logger::LogError("WriteBmp fopen failed, path: " + QString(fname_bmp));
		return;
	}

	fwrite(header, sizeof(unsigned char), 54, fp);
	fwrite(image, sizeof(unsigned char), (size_t)(long)imageWidth * imageHeight * 4, fp);

	fflush(fp);
	fclose(fp);
	count ++;
}

inline void WriteRGB(const uint8_t * data, int width, int height) {
	static FILE * fp = NULL;
	const char * path= "/tmp/ramfs/vaudit.bgra";

	if (fp == NULL) {
		fp = fopen(path, "w");
		if (fp == NULL) {
			Logger::LogError("open debug file failed, path: " + QString(path));
			return;
		}
	}

	fwrite(data, width * height * 4, 1, fp);
	fflush(fp);
}

inline void WriteNv12(uint8_t * data, size_t size, const char* filename) {
	static std::map<std::string, FILE *> filemap;
	std::map<std::string, FILE *>::iterator item;

	FILE * fp = NULL;
	char path[128] = "/tmp/ramfs/vaudit.nv12";
	if (filename) {
		snprintf(path, sizeof(path), "/tmp/ramfs/%s", filename);
	}
	item = filemap.find(path);
	if (item == filemap.end()) {
		fp = fopen(path, "w");
		if (fp == NULL) {
			Logger::LogError("open debug file failed, path: " + QString(path));
			return;
		}
		filemap[path] = fp;
	} else {
		fp = item->second;
	}

	fwrite(data, size, 1, fp);
	fflush(fp);
}

#endif

Q_DECLARE_METATYPE(Logger::enum_type)
