#ifndef CAMERAGRABBER_H
#define CAMERAGRABBER_H

#include <QThread>
#include <QMutex>

#include <librealsense2/rs.hpp>

#include "./imageframe.h"

#include "./v4l2camera.h"

namespace cv{
	class Mat;
}

class CameraGrabber : public QThread
{
	Q_OBJECT
public:
	explicit CameraGrabber(QObject *parent = nullptr);

	rs2::config& config();
	rs2::config const& config() const;
	void setConfig(const rs2::config &config);

	V4L2Camera::Config& v4l2config();
	V4L2Camera::Config const& v4l2config() const;
	void setV4L2Config(const V4L2Camera::Config &config);

	int opencvdeviceid() const;
	void setOpenCvDeviceId(const int &devid);

	V4L2Camera::Descriptor v4l2descr() const;
	void setV4l2descr(const V4L2Camera::Descriptor &v4l2descr);

	virtual void run();
	void finish();

	void setInfraRedPatternOn(bool on);

Q_SIGNALS:

	void framesReady(ImageFrame frameLeft, ImageFrame frameRight, ImageFrame frameRGB);
	void acquisitionEndedWithError(QString error);

protected:

	rs2::config _config;
	rs2::pipeline_profile _pipeline_profile;

	int _opencv_dev_id;

	V4L2Camera::Config _v4l2config;
	V4L2Camera::Descriptor _v4l2descr;

	QMutex _interruptionMutex;
	bool _continue;

};

ImageFrame realsenseFrameToImageFrame(const rs2::frame &f);
ImageFrame cvFrameToImageFrame(const cv::Mat &frame);

#endif // CAMERAGRABBER_H
