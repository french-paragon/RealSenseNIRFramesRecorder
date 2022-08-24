#include "cameragrabber.h"

CameraGrabber::CameraGrabber(QObject *parent) : QThread(parent)
{

}

rs2::config &CameraGrabber::config()
{
	return _config;
}

rs2::config const& CameraGrabber::config() const
{
	return _config;
}

void CameraGrabber::setConfig(const rs2::config &config)
{
	_config = config;

	_config.enable_stream(rs2_stream::RS2_STREAM_INFRARED, 1, 1280, 800, rs2_format::RS2_FORMAT_Y16, 15);
	_config.enable_stream(rs2_stream::RS2_STREAM_INFRARED, 2, 1280, 800, rs2_format::RS2_FORMAT_Y16, 15);
	//_config.enable_all_streams();

}


void CameraGrabber::run () {

	_interruptionMutex.lock();
	_continue = true;
	_interruptionMutex.unlock();

	rs2::pipeline pipe;
	pipe.start(_config);
	rs2::frameset frames;

	while (_continue) {
		try {
			frames = pipe.wait_for_frames();
		} catch (std::runtime_error & e) {
			emit acquisitionEndedWithError(e.what());
			break;
		}

		rs2::frame fl = frames.get_infrared_frame(1);
		rs2::frame fr = frames.get_infrared_frame(2);

		ImageFrame frameLeft = realsenseFrameToImageFrame(fl);
		ImageFrame frameRight = realsenseFrameToImageFrame(fr);

		emit framesReady(frameLeft, frameRight);
	}
}
void CameraGrabber::finish() {
	_interruptionMutex.lock();
	_continue = false;
	_interruptionMutex.unlock();
}

ImageFrame realsenseFrameToImageFrame(const rs2::frame &f) {

	using namespace rs2;

	auto vf = f.as<video_frame>();
	const int w = vf.get_width();
	const int h = vf.get_height();

	rs2_format format = f.get_profile().format();

	if (format == RS2_FORMAT_RGB8)
	{
		ImageFrame ret((uint8_t*) f.get_data(),
					   Multidim::Array<uint8_t,3>::ShapeBlock{h,w, 3},
					   Multidim::Array<uint8_t,3>::ShapeBlock{3*w,3,1},
					   false);
		return ret;
	}
	else if (format == RS2_FORMAT_Y8)
	{
		ImageFrame ret((uint8_t*) f.get_data(),
					   Multidim::Array<uint8_t,2>::ShapeBlock{h,w},
					   Multidim::Array<uint8_t,2>::ShapeBlock{w,1},
					   false);
		return ret;
	}
	else if (format == RS2_FORMAT_Y16)
	{
		ImageFrame ret((uint16_t*) f.get_data(),
					   Multidim::Array<uint16_t,2>::ShapeBlock{h,w},
					   Multidim::Array<uint16_t,2>::ShapeBlock{w,1},
					   false);
		return ret;
	}

	throw std::runtime_error("Unsupported frame format !");
}
