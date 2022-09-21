#include "cameragrabber.h"

#include <QCoreApplication>

#include <opencv2/videoio.hpp>

#include <QDebug>

CameraGrabber::CameraGrabber(QObject *parent) :
	QThread(parent),
	_pipeline_profile()
{
	_opencv_dev_id = -1;
	_v4l2descr = {"", -1};
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

	_config.enable_stream(rs2_stream::RS2_STREAM_INFRARED, 1, 848, 480, rs2_format::RS2_FORMAT_Y8, 60);
	_config.enable_stream(rs2_stream::RS2_STREAM_INFRARED, 2, 848, 480, rs2_format::RS2_FORMAT_Y8, 60);
	_config.enable_stream(rs2_stream::RS2_STREAM_COLOR, -1, 848, 480, rs2_format::RS2_FORMAT_RGB8, 60);
	//_config.enable_all_streams();

}

V4L2Camera::Config& CameraGrabber::v4l2config() {
	return _v4l2config;
}
V4L2Camera::Config const& CameraGrabber::v4l2config() const {
	return _v4l2config;
}
void CameraGrabber::setV4L2Config(const V4L2Camera::Config &config) {
	_v4l2config = config;

	_v4l2config.frameSize.width = 3840;
	_v4l2config.frameSize.height = 1080;

	_v4l2config.fps.numerator=1;
	_v4l2config.fps.denominator=30;
}

int CameraGrabber::opencvdeviceid() const
{
	return _opencv_dev_id;
}

void CameraGrabber::setOpenCvDeviceId(const int &devid)
{
	_opencv_dev_id = devid;
}

V4L2Camera::Descriptor CameraGrabber::v4l2descr() const
{
	return _v4l2descr;
}

void CameraGrabber::setV4l2descr(const V4L2Camera::Descriptor &v4l2descr)
{
	_v4l2descr = v4l2descr;
}


void CameraGrabber::run () {

	_interruptionMutex.lock();
	_continue = true;
	_interruptionMutex.unlock();

	if (_v4l2descr.index >= 0) {

		V4L2Camera cam(_v4l2descr);

		if (!cam.isValid()) {
			emit acquisitionEndedWithError("Unable to open video device with v4l2");;
			return;
		}

		bool ok = cam.start(_v4l2config);

		if (!ok) {
			emit acquisitionEndedWithError("Unable to start streaming with v4l2 video device");;
			return;
		}

		while (_continue) {
			ok = cam.treatNextFrame([this, &cam] (Multidim::Array<uint8_t,3> & frame) {

				ImageFrame left = ImageFrame();
				ImageFrame right = ImageFrame();

				ImageFrame rgb(&frame.atUnchecked(0,0,0),frame.shape(), frame.strides(), false);
				if (!cam.colorSpace().isEmpty()) {
					rgb.additionalInfos()[ImageFrame::colorSpaceKey] = cam.colorSpace();
				}

				Q_EMIT framesReady(left, right, rgb);
			});

			if (!ok) {
				emit acquisitionEndedWithError("Unable to load frames with v4l2 video device");;
				return;
			}
		}

		cam.stop();

	} else if (_opencv_dev_id >= 0) {

		cv::Mat frame;
		cv::VideoCapture cap;

		cap.open(_opencv_dev_id);
		if (!cap.isOpened()) {
			emit acquisitionEndedWithError("Unable to open video device with opencv");;
			return;
		}

		while (_continue) {
			cap.read(frame);

			if (frame.empty()) {
				emit acquisitionEndedWithError("Missing frame");
				break;
			}

			ImageFrame frameLeft = ImageFrame();
			ImageFrame frameRight = ImageFrame();

			ImageFrame frameRGB = cvFrameToImageFrame(frame);

			Q_EMIT framesReady(frameLeft, frameRight, frameRGB);
		}

	} else {

		rs2::pipeline pipe;
		_pipeline_profile = pipe.start(_config);
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

			rs2::frame frgb = frames.get_color_frame();

			ImageFrame frameLeft = realsenseFrameToImageFrame(fl);
			ImageFrame frameRight = realsenseFrameToImageFrame(fr);

			ImageFrame frameRGB = realsenseFrameToImageFrame(frgb);

			Q_EMIT framesReady(frameLeft, frameRight, frameRGB);
		}
	}
}
void CameraGrabber::finish() {
	_interruptionMutex.lock();
	_continue = false;
	_pipeline_profile = rs2::pipeline_profile();
	_interruptionMutex.unlock();
}

void CameraGrabber::setInfraRedPatternOn(bool on) {
	_interruptionMutex.lock();
	if (_pipeline_profile) {
		rs2::device dev = _pipeline_profile.get_device();

		auto depth_sensor = dev.first<rs2::depth_sensor>();

		if (depth_sensor.supports(RS2_OPTION_EMITTER_ENABLED)) {

			float value = (on) ? 1.f : 0.f;

			qDebug() << "discrete value: " << value;
			depth_sensor.set_option(RS2_OPTION_EMITTER_ENABLED, value);
		}

		if (depth_sensor.supports(RS2_OPTION_LASER_POWER))
		{
			// Query min and max values:
			auto range = depth_sensor.get_option_range(RS2_OPTION_LASER_POWER);
			float value = (on) ? range.max : 0.f;

			qDebug() << "continuous value: " << value;
			depth_sensor.set_option(RS2_OPTION_LASER_POWER, value);
		}
	}
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

ImageFrame cvFrameToImageFrame(const cv::Mat &frame) {

	const int w = frame.cols;
	const int h = frame.rows;
	const int c = frame.channels();

	int depth = frame.depth();

	if (depth == CV_8U) {

		ImageFrame ret((uint8_t*) frame.data,
					   Multidim::Array<uint8_t,3>::ShapeBlock{h,w, c},
					   Multidim::Array<uint8_t,3>::ShapeBlock{c*w,c,1},
					   false);
		return ret;

	} else if (depth == CV_16U and c == 1) {

		ImageFrame ret((uint16_t*) frame.data,
					   Multidim::Array<uint16_t,2>::ShapeBlock{h,w},
					   Multidim::Array<uint16_t,2>::ShapeBlock{w,1},
					   false);
		return ret;
	} else if (depth == CV_32F and c == 1) {

		ImageFrame ret((float*) frame.data,
					   Multidim::Array<float,2>::ShapeBlock{h,w},
					   Multidim::Array<float,2>::ShapeBlock{w,1},
					   false);
		return ret;
	}

	throw std::runtime_error("Unsupported frame format !");
}
