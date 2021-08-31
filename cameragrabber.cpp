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

	_config.enable_all_streams();

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

		QImage frameLeft = realsenseFrameToQImage(frames.get_infrared_frame(0));
		QImage frameRight = realsenseFrameToQImage(frames.get_infrared_frame(2));

		emit framesReady(frameLeft, frameRight);
	}
}
void CameraGrabber::finish() {
	_interruptionMutex.lock();
	_continue = false;
	_interruptionMutex.unlock();
}

QImage realsenseFrameToQImage(const rs2::frame &f)
{
	using namespace rs2;

	auto vf = f.as<video_frame>();
	const int w = vf.get_width();
	const int h = vf.get_height();

	if (f.get_profile().format() == RS2_FORMAT_RGB8)
	{
		QImage ret((uchar*) f.get_data(), w, h, w*3, QImage::Format_RGB888);
		return ret;
	}
	else if (f.get_profile().format() == RS2_FORMAT_Y8)
	{
		QImage ret((uchar*) f.get_data(), w, h, w, QImage::Format_Grayscale8);
		return ret;
	}
	else if (f.get_profile().format() == RS2_FORMAT_Y16)
	{
		QImage ret((uchar*) f.get_data(), w, h, w*2, QImage::Format_Grayscale16);
		return ret;
	}

	throw std::runtime_error("Unsupported frame format !");
}
