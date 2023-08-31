/*
Copyright (c) 2012-2020 Maarten Baert <maarten-baert@hotmail.com>

This file contains code from x11grab.c (part of ffmpeg/libav). The copyright information for x11grab.c is:
>> FFmpeg/Libav integration:
>> Copyright (C) 2006 Clemens Fruhwirth <clemens@endorphin.org>
>>                    Edouard Gomez <ed.gomez@free.fr>
>>
>> This file contains code from grab.c:
>> Copyright (c) 2000-2001 Fabrice Bellard
>>
>> This file contains code from the xvidcap project:
>> Copyright (C) 1997-1998 Rasca, Berlin
>>               2003-2004 Karl H. Beckers, Frankfurt

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

#include "X11Input.h"

#include "Logger.h"
#include "AVWrapper.h"
#include "Synchronizer.h"
#include "VideoEncoder.h"

/*
The code in this file is based on the MIT-SHM example code and the x11grab device in libav/ffmpeg (which is GPL):
http://www.xfree86.org/current/mit-shm.html
https://git.libav.org/?p=libav.git;a=blob;f=libavdevice/x11grab.c
I am doing the recording myself instead of just using x11grab (as I originally planned) because this is more flexible.
*/

/**
 * @brief 获取XImage 原始数据格式, 水印功能目前只支持 PBGRA格式
 * Converts a X11 image format to a format that libav/ffmpeg understands.
 *
 * @param image
 *
 * @return
 */

static AVPixelFormat X11ImageGetPixelFormat(XImage* image) {
	switch(image->bits_per_pixel) {
		case 8: return AV_PIX_FMT_PAL8;
		case 16: {
			if(image->red_mask == 0xf800 && image->green_mask == 0x07e0 && image->blue_mask == 0x001f) return AV_PIX_FMT_RGB565;
			if(image->red_mask == 0x7c00 && image->green_mask == 0x03e0 && image->blue_mask == 0x001f) return AV_PIX_FMT_RGB555;
			break;
		}
		case 24: {
			if(image->red_mask == 0xff0000 && image->green_mask == 0x00ff00 && image->blue_mask == 0x0000ff) return AV_PIX_FMT_BGR24;
			if(image->red_mask == 0x0000ff && image->green_mask == 0x00ff00 && image->blue_mask == 0xff0000) return AV_PIX_FMT_RGB24;
			break;
		}
		case 32: {
			if(image->red_mask == 0xff0000 && image->green_mask == 0x00ff00 && image->blue_mask == 0x0000ff) return AV_PIX_FMT_BGRA;
			if(image->red_mask == 0x0000ff && image->green_mask == 0x00ff00 && image->blue_mask == 0xff0000) return AV_PIX_FMT_RGBA;
			if(image->red_mask == 0xff000000 && image->green_mask == 0x00ff0000 && image->blue_mask == 0x0000ff00) return AV_PIX_FMT_ABGR;
			if(image->red_mask == 0x0000ff00 && image->green_mask == 0x00ff0000 && image->blue_mask == 0xff000000) return AV_PIX_FMT_ARGB;
			break;
		}
	}
	Logger::LogError("[X11ImageGetPixelFormat] " + Logger::tr("Error: Unsupported X11 image pixel format!") + "\n"
					 "    bits_per_pixel = " + QString::number(image->bits_per_pixel) + ", red_mask = 0x" + QString::number(image->red_mask, 16)
					 + ", green_mask = 0x" + QString::number(image->green_mask, 16) + ", blue_mask = 0x" + QString::number(image->blue_mask, 16));
	throw X11Exception();
}

 /**
 * @brief 清理Ximage 指定区域的图像
 * clears a rectangular area of an image (i.e. sets the memory to zero, which will most likely make the image black)
 *
 * @param image
 * @param x
 * @param y
 * @param w
 * @param h
 */
static void X11ImageClearRectangle(XImage* image, unsigned int x, unsigned int y, unsigned int w, unsigned int h) {

	// check the image format
	if(image->bits_per_pixel % 8 != 0)
		return;
	unsigned int pixel_bytes = image->bits_per_pixel / 8;

	// fill the rectangle with zeros
	for(unsigned int j = 0; j < h; ++j) {
		uint8_t *image_row = (uint8_t*) image->data + image->bytes_per_line * (y + j);
		memset(image_row + pixel_bytes * x, 0, pixel_bytes * w);
	}

}

// Draws the current cursor at the current position on the image. Requires XFixes.
// Note: In the original code from x11grab, the variables for red and blue are swapped
// (which doesn't change the result, but it's confusing).
// Note 2: This function assumes little-endianness.
// Note 3: This function only supports 24-bit and 32-bit images (it does nothing for other bit depths).
static void X11ImageDrawCursor(Display* dpy, XImage* image, int recording_area_x, int recording_area_y) {

	// check the image format
	unsigned int pixel_bytes, r_offset, g_offset, b_offset;
	if(image->bits_per_pixel == 24 && image->red_mask == 0xff0000 && image->green_mask == 0x00ff00 && image->blue_mask == 0x0000ff) {
		pixel_bytes = 3;
		r_offset = 2; g_offset = 1; b_offset = 0;
	} else if(image->bits_per_pixel == 24 && image->red_mask == 0x0000ff && image->green_mask == 0x00ff00 && image->blue_mask == 0xff0000) {
		pixel_bytes = 3;
		r_offset = 0; g_offset = 1; b_offset = 2;
	} else if(image->bits_per_pixel == 32 && image->red_mask == 0xff0000 && image->green_mask == 0x00ff00 && image->blue_mask == 0x0000ff) {
		pixel_bytes = 4;
		r_offset = 2; g_offset = 1; b_offset = 0;
	} else if(image->bits_per_pixel == 32 && image->red_mask == 0x0000ff && image->green_mask == 0x00ff00 && image->blue_mask == 0xff0000) {
		pixel_bytes = 4;
		r_offset = 0; g_offset = 1; b_offset = 2;
	} else if(image->bits_per_pixel == 32 && image->red_mask == 0xff000000 && image->green_mask == 0x00ff0000 && image->blue_mask == 0x0000ff00) {
		pixel_bytes = 4;
		r_offset = 3; g_offset = 2; b_offset = 1;
	} else if(image->bits_per_pixel == 32 && image->red_mask == 0x0000ff00 && image->green_mask == 0x00ff0000 && image->blue_mask == 0xff000000) {
		pixel_bytes = 4;
		r_offset = 1; g_offset = 2; b_offset = 3;
	} else {
		return;
	}

	// get the cursor
	XFixesCursorImage *xcim = XFixesGetCursorImage(dpy);
	if(xcim == NULL)
		return;

	// calculate the position of the cursor
	int x = xcim->x - xcim->xhot - recording_area_x;
	int y = xcim->y - xcim->yhot - recording_area_y;

	// calculate the part of the cursor that's visible
	int cursor_left = std::max(0, -x), cursor_right = std::min((int) xcim->width, image->width - x);
	int cursor_top = std::max(0, -y), cursor_bottom = std::min((int) xcim->height, image->height - y);

	// draw the cursor
	// XFixesCursorImage uses 'long' instead of 'int' to store the cursor images, which is a bit weird since
	// 'long' is 64-bit on 64-bit systems and only 32 bits are actually used. The image uses premultiplied alpha.
	for(int j = cursor_top; j < cursor_bottom; ++j) {
		unsigned long *cursor_row = xcim->pixels + xcim->width * j;
		uint8_t *image_row = (uint8_t*) image->data + image->bytes_per_line * (y + j);
		for(int i = cursor_left; i < cursor_right; ++i) {
			unsigned long cursor_pixel = cursor_row[i];
			uint8_t *image_pixel = image_row + pixel_bytes * (x + i);
			int cursor_a = (uint8_t) (cursor_pixel >> 24);
			int cursor_r = (uint8_t) (cursor_pixel >> 16);
			int cursor_g = (uint8_t) (cursor_pixel >> 8);
			int cursor_b = (uint8_t) (cursor_pixel >> 0);
			if(cursor_a == 255) {
				image_pixel[r_offset] = cursor_r;
				image_pixel[g_offset] = cursor_g;
				image_pixel[b_offset] = cursor_b;
			} else {
				image_pixel[r_offset] = (image_pixel[r_offset] * (255 - cursor_a) + 127) / 255 + cursor_r;
				image_pixel[g_offset] = (image_pixel[g_offset] * (255 - cursor_a) + 127) / 255 + cursor_g;
				image_pixel[b_offset] = (image_pixel[b_offset] * (255 - cursor_a) + 127) / 255 + cursor_b;
			}
		}
	}

	// free the cursor
	XFree(xcim);

}

/**
 * @brief 设置水印
 */
static uint8_t* X11ImageDrawWatermark(uint8_t* image_data,QString watermark_content , unsigned int grab_width, unsigned int grab_height){
	//水印处理, pixel format为PBGRA格式
	cairo_surface_t * surface;
	cairo_t* cr;
    surface = cairo_image_surface_create_for_data(image_data, CAIRO_FORMAT_ARGB32, grab_width, grab_height, grab_width * 4);
	cr = cairo_create(surface);
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.5);
	cairo_set_line_width(cr, 8);
	cairo_select_font_face(cr, "STSong", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 36);
    double xmove = grab_width - 790;
    cairo_move_to(cr, xmove, grab_height - 80);

	if(!CommandLineOptions::GetFrontRecord()){ //后台录屏添加时间+用户名水印
		QDateTime current_date_time = QDateTime::currentDateTime();
		QString cdt = current_date_time.toString("yyyy/MM/dd hh:mm:ss"); //录制时间
//		QString name = qgetenv("USER"); //当前系统用户名
		QString name = settings_ptr->value("record/user", QString("root")).toString(); //当前系统用户名

		QString watermark_content_all = QString("%1 %2").arg(name).arg(cdt);
		cairo_show_text(cr, watermark_content_all.toStdString().c_str());
	}else{
		cairo_show_text(cr, watermark_content.toStdString().c_str());
	}
	cairo_destroy(cr);
	cairo_surface_flush(surface);
	uint8_t * imagedata_front = cairo_image_surface_get_data(surface);
	cairo_surface_destroy(surface);
	cr = NULL;
	surface = NULL;

	return imagedata_front;
}


X11Input::X11Input(unsigned int x, unsigned int y, unsigned int width, unsigned int height, bool record_cursor, bool follow_cursor, bool follow_full_screen, bool is_use_watermarking, bool is_only_audio) {

	m_x = x;
	m_y = y;
	m_width = width;
	m_height = height;
	m_record_cursor = record_cursor;
	m_follow_cursor = follow_cursor;
	m_follow_fullscreen = follow_full_screen;

	m_x11_display = NULL;
	// use two image buffer to check whether image changed to prev image
	m_x11_img_idx = 0;
	m_x11_image[0] = NULL;
	m_x11_image[1] = NULL;
	m_x11_shm_info[0].shmseg = 0;
	m_x11_shm_info[0].shmid = -1;
	m_x11_shm_info[0].shmaddr = (char*) -1;
	m_x11_shm_info[0].readOnly = false;
	m_x11_shm_info[1].shmseg = 0;
	m_x11_shm_info[1].shmid = -1;
	m_x11_shm_info[1].shmaddr = (char*) -1;
	m_x11_shm_info[1].readOnly = false;
   	m_x11_shm_server_attached = false;
 
	m_is_use_watermarking = is_use_watermarking;
	m_is_only_audio = is_only_audio;

	// 只录音频或者视频文件数据重复用黑色图片填充
	{
		int image_size = m_height * m_width * 4;
		m_black_image_data.resize(image_size, 0);
		for (int i = 0; i < image_size; i++)
		{
			if ((i + 1) % 4 == 0)
				m_black_image_data[i] = 0xff;
		}
	}

	m_screen_bbox = Rect(m_x, m_y, m_x + m_width, m_y + m_height);

	{
		SharedLock lock(&m_shared_data);
		lock->m_current_x = m_x;
		lock->m_current_y = m_y;
		lock->m_current_width = m_width;
		lock->m_current_height = m_height;
	}

	if(m_width == 0 || m_height == 0) {
		Logger::LogError("[X11Input::Init] " + Logger::tr("Error: Width or height is zero!"));
		throw X11Exception();
	}
	if(m_width > SSR_MAX_IMAGE_SIZE || m_height > SSR_MAX_IMAGE_SIZE) {
		Logger::LogError("[X11Input::Init] " + Logger::tr("Error: Width or height is too large, the maximum width and height is %1!").arg(SSR_MAX_IMAGE_SIZE));
		throw X11Exception();
	}

	try {
		Init();
	} catch(...) {
		Free();
		throw;
	}

}

X11Input::~X11Input() {

	// tell the thread to stop
	if(m_thread.joinable()) {
		Logger::LogInfo("[X11Input::~X11Input] " + Logger::tr("Stopping input thread ..."));
		m_should_stop = true;
		m_thread.join();
	}

	// free everything
	Free();

}

void X11Input::GetCurrentRectangle(unsigned int *x, unsigned int *y, unsigned int *width, unsigned int *height) {
	SharedLock lock(&m_shared_data);
	*x = lock->m_current_x;
	*y = lock->m_current_y;
	*width = lock->m_current_width;
	*height = lock->m_current_height;
}

void X11Input::GetCurrentSize(unsigned int *width, unsigned int *height) {
	SharedLock lock(&m_shared_data);
	*width = lock->m_current_width;
	*height = lock->m_current_height;
}

double X11Input::GetFPS() {
	int64_t timestamp = hrt_time_micro();
	uint32_t frame_counter = m_frame_counter;
	unsigned int time = timestamp - m_fps_last_timestamp;
	if(time > 500000) {
		unsigned int frames = frame_counter - m_fps_last_counter;
		m_fps_last_timestamp = timestamp;
		m_fps_last_counter = frame_counter;
		m_fps_current = (double) frames / ((double) time * 1.0e-6);
	}
	return m_fps_current;
}

void X11Input::Init() {

	// do the X11 stuff
	// we need a separate display because the existing one would interfere with what Qt is doing in some cases
	m_x11_display = XOpenDisplay(NULL); //QX11Info::display();
	if(m_x11_display == NULL) {
		Logger::LogError("[X11Input::Init] " + Logger::tr("Error: Can't open X display!", "Don't translate 'display'"));
		throw X11Exception();
	}
	m_x11_screen = DefaultScreen(m_x11_display); //QX11Info::appScreen();
	m_x11_root = RootWindow(m_x11_display, m_x11_screen); //QX11Info::appRootWindow(m_x11_screen);
	m_x11_visual = DefaultVisual(m_x11_display, m_x11_screen); //(Visual*) QX11Info::appVisual(m_x11_screen);
	m_x11_depth = DefaultDepth(m_x11_display, m_x11_screen); //QX11Info::appDepth(m_x11_screen);
	m_x11_use_shm = XShmQueryExtension(m_x11_display);
	if(m_x11_use_shm) {
		Logger::LogInfo("[X11Input::Init] " + Logger::tr("Using X11 shared memory."));
	} else {
		Logger::LogInfo("[X11Input::Init] " + Logger::tr("Not using X11 shared memory."));
	}

	// showing the cursor requires XFixes (which should be supported on any modern X server, but let's check it anyway)
	if(m_record_cursor) {
		int event, error;
		if(!XFixesQueryExtension(m_x11_display, &event, &error)) {
			Logger::LogWarning("[X11Input::Init] " + Logger::tr("Warning: XFixes is not supported by X server, the cursor has been hidden.", "Don't translate 'XFixes'"));
			m_record_cursor = false;
		}
	}

	// get screen configuration information, so we can replace the unused areas with black rectangles (rather than showing random uninitialized memory)
	// this is also used by the mouse following code to make sure that the rectangle stays on the screen
	UpdateScreenConfiguration();

	// initialize frame counter
	m_frame_counter = 0;
	m_fps_last_timestamp = hrt_time_micro();
	m_fps_last_counter = 0;
	m_fps_current = 0.0;

	//initialize the watermark_content
	if(settings_ptr->value("record/is_use_watermark").toUInt() == 0){
		m_is_use_watermarking = false;
		watermark_content = settings_ptr ->value("record/water_print_text").toString();
	}else{
		m_is_use_watermarking = true;
		watermark_content = settings_ptr ->value("record/water_print_text").toString();
	}
	Logger::LogInfo("[X11Input::Init] the watermark content is " + watermark_content);
	// start input thread
	m_should_stop = false;
	m_error_occurred = false;
	m_thread = std::thread(&X11Input::InputThread, this);

}

void X11Input::Free() {
	FreeImage();
	if(m_x11_display != NULL) {
		XCloseDisplay(m_x11_display);
		m_x11_display = NULL;
	}
}

/*
 * when use shm to get image, init image object
 * width: width of image resolution
 * height: height of image resolution
 */
void X11Input::AllocateImage(unsigned int width, unsigned int height) {
	assert(m_x11_use_shm);

	if(m_x11_shm_server_attached && m_x11_image[0]->width == (int) width && m_x11_image[0]->height == (int) height
		&& m_x11_image[1]->width == (int) width && m_x11_image[1]->height == (int) height) {
		// Logger::LogInfo("[X11Input::AllocateImage] reuse image");
		return; // reuse existing image
	}

	Logger::LogInfo("[X11Input::AllocateImage] create new image");
	FreeImage();

	for (int i = 0;i < 2; i++) {
		m_x11_image[i] = XShmCreateImage(m_x11_display, m_x11_visual, m_x11_depth, ZPixmap, NULL, &m_x11_shm_info[i], width, height);
		if(m_x11_image[i] == NULL) {
			Logger::LogError("[X11Input::Init] " + Logger::tr("Error: Can't create shared image!"));
			throw X11Exception();
		}

		m_x11_shm_info[i].shmid = shmget(IPC_PRIVATE, m_x11_image[i]->bytes_per_line * m_x11_image[i]->height, IPC_CREAT | 0700);
		if(m_x11_shm_info[i].shmid == -1) {
			Logger::LogError("[X11Input::Init] " + Logger::tr("Error: Can't get shared memory!"));
			throw X11Exception();
		}

		m_x11_shm_info[i].shmaddr = (char*) shmat(m_x11_shm_info[i].shmid, NULL, SHM_RND);
		if(m_x11_shm_info[i].shmaddr == (char*) -1) {
			Logger::LogError("[X11Input::Init] " + Logger::tr("Error: Can't attach to shared memory!"));
			throw X11Exception();
		}

		m_x11_image[i]->data = m_x11_shm_info[i].shmaddr;
		if(!XShmAttach(m_x11_display, &m_x11_shm_info[i])) {
			Logger::LogError("[X11Input::Init] " + Logger::tr("Error: Can't attach server to shared memory!"));
			throw X11Exception();
		}
	}

	m_x11_shm_server_attached = true;
}

void X11Input::FreeImage() {
	if(m_x11_shm_server_attached) {
		XShmDetach(m_x11_display, &m_x11_shm_info[0]);
		XShmDetach(m_x11_display, &m_x11_shm_info[1]);
		m_x11_shm_server_attached = false;
	}

	for (int i = 0;i < 2;i ++) {
		if(m_x11_shm_info[i].shmaddr != (char*) -1) {
			shmdt(m_x11_shm_info[i].shmaddr);
			m_x11_shm_info[i].shmaddr = (char*) -1;
		}

		if(m_x11_shm_info[i].shmid != -1) {
			shmctl(m_x11_shm_info[i].shmid, IPC_RMID, NULL);
			m_x11_shm_info[i].shmid = -1;
		}

		if(m_x11_image[i] != NULL) {
			XDestroyImage(m_x11_image[i]);
			m_x11_image[i] = NULL;
		}
	}
}

void X11Input::UpdateScreenConfiguration() {

	Logger::LogInfo("[X11Input::Init] " + Logger::tr("Detecting screen configuration ..."));

	// get screen rectangles
	m_screen_rects.clear();
	int event_base, error_base;
	if(XineramaQueryExtension(m_x11_display, &event_base, &error_base)) {
		int num_screens;
		XineramaScreenInfo *screens = XineramaQueryScreens(m_x11_display, &num_screens);
		try {
			for(int i = 0; i < num_screens; ++i) {
				m_screen_rects.emplace_back(screens[i].x_org, screens[i].y_org, screens[i].x_org + screens[i].width, screens[i].y_org + screens[i].height);
			}
		} catch(...) {
			XFree(screens);
			throw;
		}
		XFree(screens);
	} else {
		Logger::LogWarning("[X11Input::Init] " + Logger::tr("Warning: Xinerama is not supported by X server, multi-monitor support may not work properly.", "Don't translate 'Xinerama'"));
		return;
	}

	// make sure that we have at least one monitor
	if(m_screen_rects.size() == 0) {
		Logger::LogWarning("[X11Input::Init] " + Logger::tr("Warning: No monitors detected, multi-monitor support may not work properly."));
		return;
	}

	// log the screen rectangles
	for(size_t i = 0; i < m_screen_rects.size(); ++i) {
		Rect &rect = m_screen_rects[i];
		Logger::LogInfo("[X11Input::Init] " + Logger::tr("Screen %1:").arg(i)
						+ " x1 = " + QString::number(rect.m_x1) + ", y1 = " + QString::number(rect.m_y1)
						+ ", x2 = " + QString::number(rect.m_x2) + ", y2 = " + QString::number(rect.m_y2));
	}

	// calculate bounding box
	m_screen_bbox = m_screen_rects[0];
	for(size_t i = 1; i < m_screen_rects.size(); ++i) {
		Rect &rect = m_screen_rects[i];
		if(rect.m_x1 < m_screen_bbox.m_x1)
			m_screen_bbox.m_x1 = rect.m_x1;
		if(rect.m_y1 < m_screen_bbox.m_y1)
			m_screen_bbox.m_y1 = rect.m_y1;
		if(rect.m_x2 > m_screen_bbox.m_x2)
			m_screen_bbox.m_x2 = rect.m_x2;
		if(rect.m_y2 > m_screen_bbox.m_y2)
			m_screen_bbox.m_y2 = rect.m_y2;
	}
	if(m_screen_bbox.m_x1 >= m_screen_bbox.m_x2 || m_screen_bbox.m_y1 >= m_screen_bbox.m_y2 ||
	   m_screen_bbox.m_x2 - m_screen_bbox.m_x1 > SSR_MAX_IMAGE_SIZE || m_screen_bbox.m_y2 - m_screen_bbox.m_y1 > SSR_MAX_IMAGE_SIZE) {
		Logger::LogError("[X11Input::UpdateScreenConfiguration] " + Logger::tr("Error: Invalid screen bounding box!") + "\n"
						 "    x1 = " + QString::number(m_screen_bbox.m_x1) + ", y1 = " + QString::number(m_screen_bbox.m_y1)
						 + ", x2 = " + QString::number(m_screen_bbox.m_x2) + ", y2 = " + QString::number(m_screen_bbox.m_y2));
		throw X11Exception();
	}

	/*qDebug() << "m_screen_rects:";
	for(Rect &rect : m_screen_rects) {
		qDebug() << "    rect" << rect.m_x1 << rect.m_y1 << rect.m_x2 << rect.m_y2;
	}
	qDebug() << "m_screen_bbox:";
	qDebug() << "    rect" << m_screen_bbox.m_x1 << m_screen_bbox.m_y1 << m_screen_bbox.m_x2 << m_screen_bbox.m_y2;*/

	// calculate dead space
	m_screen_dead_space = {m_screen_bbox};
	for(size_t i = 0; i < m_screen_rects.size(); ++i) {
		/*qDebug() << "PARTIAL m_screen_dead_space:";
		for(Rect &rect : m_screen_dead_space) {
			qDebug() << "    rect" << rect.m_x1 << rect.m_y1 << rect.m_x2 << rect.m_y2;
		}*/
		Rect &subtract = m_screen_rects[i];
		std::vector<Rect> result;
		for(Rect &rect : m_screen_dead_space) {
			if(rect.m_x1 < subtract.m_x2 && rect.m_y1 < subtract.m_y2 && subtract.m_x1 < rect.m_x2 && subtract.m_y1 < rect.m_y2) {
				unsigned int mid_y1 = std::max(rect.m_y1, subtract.m_y1);
				unsigned int mid_y2 = std::min(rect.m_y2, subtract.m_y2);
				if(rect.m_y1 < subtract.m_y1)
					result.emplace_back(rect.m_x1, rect.m_y1, rect.m_x2, subtract.m_y1);
				if(rect.m_x1 < subtract.m_x1)
					result.emplace_back(rect.m_x1, mid_y1, subtract.m_x1, mid_y2);
				if(subtract.m_x2 < rect.m_x2)
					result.emplace_back(subtract.m_x2, mid_y1, rect.m_x2, mid_y2);
				if(subtract.m_y2 < rect.m_y2)
					result.emplace_back(rect.m_x1, subtract.m_y2, rect.m_x2, rect.m_y2);
			} else {
				result.emplace_back(rect);
			}
		}
		m_screen_dead_space = std::move(result);
	}

	// log the dead space rectangles
	for(size_t i = 0; i < m_screen_dead_space.size(); ++i) {
		Rect &rect = m_screen_dead_space[i];
		Logger::LogInfo("[X11Input::Init] " + Logger::tr("Dead space %1:").arg(i)
						+ " x1 = " + QString::number(rect.m_x1) + ", y1 = " + QString::number(rect.m_y1)
						+ ", x2 = " + QString::number(rect.m_x2) + ", y2 = " + QString::number(rect.m_y2));
	}

	/*qDebug() << "m_screen_dead_space:";
	for(Rect &rect : m_screen_dead_space) {
		qDebug() << "    rect" << rect.m_x1 << rect.m_y1 << rect.m_x2 << rect.m_y2;
	}*/

}

void X11Input::InputThread() {
	try {

		// init opengl env and create window for nvenc
		/*int argc = 1;
		char *argv[1] = {(char*)"dummy"};

		glutInit(&argc, argv);
		glutInitDisplayMode(GLUT_RGB | GLUT_SINGLE);
		glutInitWindowSize(80, 60);

		m_window = glutCreateWindow("gl");
		if (!m_window) {
			Logger::LogError("[X11Input::InputThread] create glut window failed");
			glutHideWindow();
		}*/

		pid_t tid = gettid();
		Logger::LogInfo("[X11Input::InputThread] " + Logger::tr("Input thread started. tid: ") + QString::number(tid));

		unsigned int grab_x = m_x, grab_y = m_y, grab_width = m_width/2*2, grab_height = m_height/2*2;
		bool has_initial_cursor = false;
		int64_t last_timestamp = hrt_time_micro();

		while(!m_should_stop) {

			// sleep
			int64_t next_timestamp = CalculateNextVideoTimestamp();
			int64_t timestamp = hrt_time_micro();
			if(next_timestamp == SINK_TIMESTAMP_NONE) {
				usleep(20000);
				continue;
			} else if(next_timestamp != SINK_TIMESTAMP_ASAP) {
				int64_t wait = next_timestamp - timestamp;
				if(wait > 21000) {
					// the thread can't sleep for too long because it still has to check the m_should_stop flag periodically
					usleep(20000);
					continue;
				} else if(wait > 0) {
					usleep(wait);
					timestamp = hrt_time_micro();
				}
			}

			// follow the cursor
			if(m_follow_cursor) {
				int mouse_x, mouse_y, dummy;
				Window dummy_win;
				unsigned int dummy_mask;
				if(XQueryPointer(m_x11_display, m_x11_root, &dummy_win, &dummy_win, &dummy, &dummy, &mouse_x, &mouse_y, &dummy_mask)) {
					if(m_follow_fullscreen) {
						for(Rect &rect : m_screen_rects) {
							if(mouse_x >= (int) rect.m_x1 && mouse_y >= (int) rect.m_y1 && mouse_x < (int) rect.m_x2 && mouse_y < (int) rect.m_y2) {
								grab_x = rect.m_x1;
								grab_y = rect.m_y1;
								grab_width = rect.m_x2 - rect.m_x1;
								grab_height = rect.m_y2 - rect.m_y1;
								break;
							}
						}
					} else {
						int grab_x_target = (mouse_x - (int) grab_width / 2) >> 1;
						int grab_y_target = (mouse_y - (int) grab_height / 2) >> 1;
						int frac = (has_initial_cursor)? lrint(1024.0 * exp(-1e-5 * (double) (timestamp - last_timestamp))) : 0;
						grab_x_target = (grab_x_target + ((int) (grab_x >> 1) - grab_x_target) * frac / 1024) << 1;
						grab_y_target = (grab_y_target + ((int) (grab_y >> 1) - grab_y_target) * frac / 1024) << 1;
						grab_x = clamp(grab_x_target, (int) m_screen_bbox.m_x1, (int) m_screen_bbox.m_x2 - (int) grab_width);
						grab_y = clamp(grab_y_target, (int) m_screen_bbox.m_y1, (int) m_screen_bbox.m_y2 - (int) grab_height);
					}
				}
				has_initial_cursor = true;
			}

			// save current size
			{
				SharedLock lock(&m_shared_data);
				if(lock->m_current_x != grab_x || lock->m_current_y != grab_y || lock->m_current_width != grab_width || lock->m_current_height != grab_height) {
					lock->m_current_x = grab_x;
					lock->m_current_y = grab_y;
					lock->m_current_width = grab_width;
					lock->m_current_height = grab_height;
					emit CurrentRectangleChanged();
				}
			}

			// swap image buffer
			int old_img_idx = m_x11_img_idx;
			m_x11_img_idx = m_x11_img_idx == 0 ? 1 : 0;

			if (m_is_only_audio)
			{
				int image_stride = grab_width * 4;
				PushVideoFrame(grab_width, grab_height, m_black_image_data.data(), image_stride, AV_PIX_FMT_BGRA, SWS_CS_DEFAULT, timestamp, 1);
				++m_frame_counter;
				last_timestamp = timestamp;
				continue;
			}

			// get the image
			if(m_x11_use_shm) {
				AllocateImage(grab_width, grab_height);
				if(!XShmGetImage(m_x11_display, m_x11_root, m_x11_image[m_x11_img_idx], grab_x, grab_y, AllPlanes)) {
					Logger::LogError("[X11Input::InputThread] " + Logger::tr("Error: Can't get image (using shared memory)!\n"
									 "    Usually this means the recording area is not completely inside the screen. Or did you change the screen resolution?"));
					throw X11Exception();
				}
			} else {
				if(m_x11_image[m_x11_img_idx] != NULL) {
					XDestroyImage(m_x11_image[m_x11_img_idx]);
					m_x11_image[m_x11_img_idx] = NULL;
				}
				m_x11_image[m_x11_img_idx] = XGetImage(m_x11_display, m_x11_root, grab_x, grab_y, grab_width, grab_height, AllPlanes, ZPixmap);
				if(m_x11_image[m_x11_img_idx] == NULL) {
					Logger::LogError("[X11Input::InputThread] " + Logger::tr("Error: Can't get image (not using shared memory)!\n"
									 "    Usually this means the recording area is not completely inside the screen. Or did you change the screen resolution?"));
					throw X11Exception();
				}
			}

			// clear the dead space
			for(size_t i = 0; i < m_screen_dead_space.size(); ++i) {
				Rect rect = m_screen_dead_space[i];
				if(rect.m_x1 < grab_x)
					rect.m_x1 = grab_x;
				if(rect.m_y1 < grab_y)
					rect.m_y1 = grab_y;
				if(rect.m_x2 > grab_x + grab_width)
					rect.m_x2 = grab_x + grab_width;
				if(rect.m_y2 > grab_y + grab_height)
					rect.m_y2 = grab_y + grab_height;
				if(rect.m_x2 > rect.m_x1 && rect.m_y2 > rect.m_y1)
					X11ImageClearRectangle(m_x11_image[m_x11_img_idx], rect.m_x1 - grab_x, rect.m_y1 - grab_y, rect.m_x2 - rect.m_x1, rect.m_y2 - rect.m_y1);
			}

			// draw the cursor
			if(m_record_cursor) {
				X11ImageDrawCursor(m_x11_display, m_x11_image[m_x11_img_idx], grab_x, grab_y);
			}

			// increase the frame counter
			++m_frame_counter;

			// push the frame
			uint8_t *image_data = (uint8_t*) m_x11_image[m_x11_img_idx]->data;
			int image_stride = m_x11_image[m_x11_img_idx]->bytes_per_line;
			AVPixelFormat x11_image_format = X11ImageGetPixelFormat(m_x11_image[m_x11_img_idx]);
          
			//TODO: 这地方以后优化一下, 每10帧检测一次, QSetting内部用的读写锁
			if(settings_ptr->value("record/is_use_watermark").toUInt() == 0){//动态检测水印是否开启
				m_is_use_watermarking = false;
			}else{
				m_is_use_watermarking = true;
			}

			int image_size = m_x11_image[m_x11_img_idx]->bytes_per_line * m_x11_image[m_x11_img_idx]->height;

			// 如果存在隐藏水印说明数据重复，推送黑图
			if (!(strncmp((char *)(&image_data[100]), "\x12\x34\x56\x78", 4) || strncmp((char *)(&image_data[200]), "\x12\x34\x56\x78", 4)
				|| strncmp((char *)(&image_data[300]), "\x12\x34\x56\x78", 4) || strncmp((char *)(&image_data[400]), "\x12\x34\x56\x78", 4)))
			{
				PushVideoFrame(grab_width, grab_height, m_black_image_data.data(), image_stride, AV_PIX_FMT_BGRA, SWS_CS_DEFAULT, timestamp, 1);
				last_timestamp = timestamp;
				continue;
			}
			// 打隐藏水印，在100 200 300 400 四个位置打上特定值
			for (int i = 100; i < 500; i=i+100)
			{
				strncpy((char *)(&image_data[i]), "\x12\x34\x56\x78", 4);
			}

			// 需要先打水印再判断是否变化
			if (m_is_use_watermarking) {
				watermark_content = settings_ptr ->value("record/water_print_text").toString();
				X11ImageDrawWatermark(image_data, watermark_content, grab_width, grab_height);
			}

			if (m_x11_image[old_img_idx] && m_x11_image[old_img_idx]->data && memcmp(m_x11_image[old_img_idx]->data, image_data, image_size) == 0) {
				PushVideoFrame(grab_width, grab_height, image_data, image_stride, x11_image_format, SWS_CS_DEFAULT, timestamp, 0);
			} else {
				PushVideoFrame(grab_width, grab_height, image_data, image_stride, x11_image_format, SWS_CS_DEFAULT, timestamp, 1);
			}
    
			last_timestamp = timestamp;

		}

		Logger::LogInfo("[X11Input::InputThread] " + Logger::tr("Input thread stopped."));

	} catch(const std::exception& e) {
		m_error_occurred = true;
		Logger::LogError("[X11Input::InputThread] " + Logger::tr("Exception '%1' in input thread.").arg(e.what()));
	} catch(...) {
		m_error_occurred = true;
		Logger::LogError("[X11Input::InputThread] " + Logger::tr("Unknown exception in input thread."));
	}

//	if (m_window) {
//		glutDestroyWindow(m_window);
//		m_window = 0;
//	}
}
