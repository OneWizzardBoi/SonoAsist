#ifndef SCREENRECORDER_H
#define SCREENRECORDER_H

#include "SensorDevice.h"

#include <string>
#include <thread>
#include <chrono>
#include <fstream>
#include <Windows.h>

#include <QImage>
#include <opencv2/opencv.hpp>

// SR_PREVIEW_RESIZE_FACTOR : to fit a (360 x 640) px display
#define SR_PREVIEW_RESIZE_FACTOR 3
#define REDIS_RESIZE_FACTOR 2

#define SCREEN_CAPTURE_FPS 20
#define CAPTURE_DISPLAY_THREAD_DELAY_MS 150

class ScreenRecorder : public SensorDevice {

	Q_OBJECT

	public:
		
		ScreenRecorder(int device_id, std::string device_description, std::string redis_state_entry, std::string log_file_path);
		~ScreenRecorder();

		// SensorDevice interface functions
		void stop_stream(void);
		void start_stream(void);
		void connect_device(void);
		void disconnect_device(void);
		void set_output_file(std::string output_folder);
		
		// threaded collection function
		void collect_window_captures(void);

		// utility functions
		void get_screen_dimensions(int&, int&) const;

	signals:
		void new_window_capture(QImage image);

	protected:

		// window capture vars
		RECT m_window_rc;
		HWND m_window_handle;
		HBITMAP m_hbwindow;
		BITMAPINFOHEADER m_bi;
		HDC m_hwindowDC, m_hwindowCompatibleDC;
		
		// image handling containers
		QImage m_preview_img;
		cv::Mat m_preview_img_mat;
		cv::Mat m_capture_mat;
		cv::Mat m_capture_cvt_mat;
		cv::Mat m_redis_img_mat;

		// thread vars
		bool m_collect_data = false;
		std::thread m_collection_thread;

		// output file vars
		bool m_output_file_loaded = false;
		std::ofstream m_output_index_file;
		std::string m_output_index_file_str;
		std::string m_output_video_file_str;

		// video output vars
		std::unique_ptr<cv::VideoWriter> m_video;

		// redis entry
		std::string m_redis_img_entry;
		
};

#endif