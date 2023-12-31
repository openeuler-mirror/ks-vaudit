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
#include <cairo/cairo.h>
#include "CommandLineOptions.h"

class X11Input : public QObject, public VideoSource {
	Q_OBJECT

private:
	struct Rect {
		unsigned int m_x1, m_y1, m_x2, m_y2;
		inline Rect() {}
		inline Rect(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2) : m_x1(x1), m_y1(y1), m_x2(x2), m_y2(y2) {}
	};
	struct SharedData {
		unsigned int m_current_x, m_current_y, m_current_width, m_current_height;
	};
	typedef MutexDataPair<SharedData>::Lock SharedLock;

private:
	unsigned int m_x, m_y, m_width, m_height;
	bool m_record_cursor, m_follow_cursor, m_follow_fullscreen;

	std::atomic<uint32_t> m_frame_counter;
	int64_t m_fps_last_timestamp;
	uint32_t m_fps_last_counter;
	double m_fps_current;

	Display *m_x11_display;
	int m_x11_screen;
	Window m_x11_root;
	Visual *m_x11_visual;
	int m_x11_depth;
	bool m_x11_use_shm;

	int m_x11_img_idx = 0;
	XImage *m_x11_image[2];
	XShmSegmentInfo m_x11_shm_info[2];

	bool m_x11_shm_server_attached;
    
    //判断是否开启水印
    bool m_is_use_watermarking;
    QString watermark_content;
    
	//不录制屏幕录制音频，写黑色图片
	bool m_is_only_audio;
	std::vector<uint8_t> m_black_image_data;

	Rect m_screen_bbox;
	std::vector<Rect> m_screen_rects;
	std::vector<Rect> m_screen_dead_space;

	std::thread m_thread;
	MutexDataPair<SharedData> m_shared_data;
	std::atomic<bool> m_should_stop, m_error_occurred;

	// opengl window for nvenc
	int m_window;

public:
	X11Input(unsigned int x, unsigned int y, unsigned int width, unsigned int height, bool record_cursor, bool follow_cursor, bool follow_fullscreen, bool is_use_watermarking = true, bool is_only_audio = false);
	~X11Input();

	// Reads the current recording rectangle.
	// This function is thread-safe.
	void GetCurrentRectangle(unsigned int* x, unsigned int* y, unsigned int* width, unsigned int* height);

    // Reads the current size of the stream.
	// This function is thread-safe.
	void GetCurrentSize(unsigned int* width, unsigned int* height);

	// Returns the total number of captured frames.
	// This function is thread-safe.
	double GetFPS();

	// Returns whether an error has occurred in the input thread.
	// This function is thread-safe.
	inline bool HasErrorOccurred() { return m_error_occurred; }

private:
	void Init();
	void Free();

private:
	void AllocateImage(unsigned int width, unsigned int height);
	void FreeImage();
	void UpdateScreenConfiguration();

private:
	void InputThread();
	// 将InputThread里的内容拆为多个函数
	void followCursor(unsigned int &grab_x, unsigned int &grab_y, unsigned int &grab_width, unsigned int &grab_height, bool &has_initial_cursor, int64_t diff_timestamp);
	void getImageAndClearSpace(unsigned int grab_x, unsigned int grab_y, unsigned int grab_width, unsigned int grab_height);
	int drawWatermark(unsigned int grab_width, unsigned int grab_height, int64_t timestamp, uint8_t *&image_data);

signals:
	void CurrentRectangleChanged();

};
