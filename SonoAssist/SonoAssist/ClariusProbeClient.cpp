#include "SonoAssist.h"
#include "ClariusProbeClient.h"

// global pointers (for the Clarius callback(s))
static ClariusProbeClient* probe_client_p = nullptr;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////// ClariusProbeClient public methods

void ClariusProbeClient::connect_device() {

    // making sure requirements are filled
    if (m_config_loaded && m_sensor_used && m_output_file_loaded) {

        // disconnecting
        disconnect_device();

        // global instance pointer (for the clarius callbacks)
        if (probe_client_p == nullptr) probe_client_p = this;

        // defining the new image callback
        auto new_processed_image_callback = [](const void* img, const ClariusProcessedImageInfo* nfo, int npos, const ClariusPosInfo* pos) {
   
            // defining the output image
            QImage output_img(probe_client_p->m_out_img_width, probe_client_p->m_out_img_height, QImage::Format_ARGB32);
            cv::Mat output_img_mat (probe_client_p->m_out_img_height, probe_client_p->m_out_img_width, CV_8UC4,
                output_img.bits(), output_img.bytesPerLine());

            // defining the input image
            QImage input_img(CLARIUS_DEFAULT_IMG_WIDTH, CLARIUS_DEFAULT_IMG_HEIGHT, QImage::Format_ARGB32);
            cv::Mat input_img_mat(CLARIUS_DEFAULT_IMG_HEIGHT, CLARIUS_DEFAULT_IMG_WIDTH, CV_8UC4, 
                input_img.bits(), input_img.bytesPerLine());
            
            // placing incoming img data in a mat for resizing
            memcpy(input_img.bits(), img, nfo->width * nfo->height * (nfo->bitsPerPixel / 8));
            cv::resize(input_img_mat, output_img_mat, output_img_mat.size(), 0, 0, cv::INTER_AREA);

            // writting data in main mode
            if (!probe_client_p->get_stream_preview_status()) {
                
                std::string output_str = probe_client_p->get_millis_timestamp() + "," +
                    std::to_string(pos->gx) + "," + std::to_string(pos->gy) + "," + std::to_string(pos->gz) + "," +
                    std::to_string(pos->ax) + "," + std::to_string(pos->ay) + "," + std::to_string(pos->az) + "\n";

                probe_client_p->m_output_imu_file << output_str;
                probe_client_p->m_video->write(output_img_mat);
            
            }

            // emitting the image and IMU data
            emit probe_client_p->new_us_image(std::move(input_img));

        };

        // mapping the listener's events to Qt application level events
        if (!clariusInitListener(0, nullptr, 
                QStandardPaths::writableLocation(QStandardPaths::AppDataLocation).toStdString().c_str(),
                new_processed_image_callback, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                CLARIUS_NORMAL_DEFAULT_WIDTH, CLARIUS_NORMAL_DEFAULT_HEIGHT))
        {
            m_device_connected = true;
        }

    }

    emit device_status_change(m_device_connected);

}

void ClariusProbeClient::disconnect_device() {
    
    if (m_device_connected) { 
        clariusDestroyListener();
        m_device_connected = false;
        emit device_status_change(false);
    }

}

void ClariusProbeClient::start_stream() {
    
    // making sure requirements are filled
    if (m_device_connected && !m_device_streaming) {
        
        configure_img_acquisition();

        // preparing the writing of data
        m_output_imu_file.open(m_output_imu_file_str, std::fstream::app);
        m_video = std::make_unique<cv::VideoWriter>(m_output_video_file_str, CV_FOURCC('M', 'J', 'P', 'G'),
            CLARIUS_VIDEO_FPS, cv::Size(m_out_img_width, m_out_img_height));

        // connecting to the probe events
        try {
            if (clariusConnect((*m_config_ptr)["us_probe_ip_address"].c_str(), 
                    std::stoi((*m_config_ptr)["us_probe_udp_port"]), nullptr) < 0) 
            {    
                m_device_streaming = true;
            }
        } catch (...) {}

    }

}

void ClariusProbeClient::stop_stream() {
    
    // stopping the acquisition
    if (m_device_streaming) {
        clariusDisconnect(nullptr);
        m_device_streaming = false;
    }

    // closing the outputs
    m_video->release();
    m_output_imu_file.close();

}

void ClariusProbeClient::set_output_file(std::string output_folder) {

    try {

        // defining the output file path
        m_output_imu_file_str = output_folder + "/clarius_imu.csv";
        m_output_video_file_str = output_folder + "/clarius_images.avi";
        if (m_output_imu_file.is_open()) m_output_imu_file.close();

        // writing the output file header
        m_output_imu_file.open(m_output_imu_file_str);
        m_output_imu_file << "Time (ms),gx,gy,gz,ax,ay,az" << std::endl;
        m_output_imu_file.close();
        m_output_file_loaded = true;

    }
    catch (...) {
        qDebug() << "n\GazeTracker - error occured while setting the output file";
    }


}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////// helper functions

/*
Loads the configurations for the generation of output images
*/
void ClariusProbeClient::configure_img_acquisition(void) {

    // defining output image dimensions (normal and preview) mode
    if (!m_stream_preview) {
        
        try {
            m_out_img_width = std::stoi((*m_config_ptr)["us_image_main_display_width"]);
            m_out_img_height = std::stoi((*m_config_ptr)["us_image_main_display_height"]);;
        }
        catch (...) {
            m_out_img_width = CLARIUS_NORMAL_DEFAULT_WIDTH;
            m_out_img_height = CLARIUS_NORMAL_DEFAULT_HEIGHT;
        }
   
    } else {
        m_out_img_width = CLARIUS_PREVIEW_IMG_WIDTH;
        m_out_img_height = CLARIUS_PREVIEW_IMG_HEIGHT;
    }
    
}