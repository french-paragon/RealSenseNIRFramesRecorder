#ifndef CAMERAGRABBER_H
#define CAMERAGRABBER_H

#include <QThread>
#include <QMutex>

#include <librealsense2/rs.hpp>

#include "./imageframe.h"

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

	int opencvdeviceid() const;
	void setOpenCvDeviceId(const int &devid);

	virtual void run();
	void finish();

Q_SIGNALS:

	void framesReady(ImageFrame frameLeft, ImageFrame frameRight, ImageFrame frameRGB);
	void acquisitionEndedWithError(QString error);

protected:

	rs2::config _config;
	int _opencv_dev_id;

	QMutex _interruptionMutex;
	bool _continue;

};

ImageFrame realsenseFrameToImageFrame(const rs2::frame &f);
ImageFrame cvFrameToImageFrame(const cv::Mat &frame);

#endif // CAMERAGRABBER_H
