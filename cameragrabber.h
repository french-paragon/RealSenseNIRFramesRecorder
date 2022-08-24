#ifndef CAMERAGRABBER_H
#define CAMERAGRABBER_H

#include <QThread>
#include <QMutex>

#include <librealsense2/rs.hpp>

#include "./imageframe.h"

class CameraGrabber : public QThread
{
	Q_OBJECT
public:
	explicit CameraGrabber(QObject *parent = nullptr);

	rs2::config& config();
	rs2::config const& config() const;
	void setConfig(const rs2::config &config);

	virtual void run();
	void finish();

signals:

	void framesReady(ImageFrame frameLeft, ImageFrame frameRight);
	void acquisitionEndedWithError(QString error);

protected:

	rs2::config _config;

	QMutex _interruptionMutex;
	bool _continue;

};

ImageFrame realsenseFrameToImageFrame(const rs2::frame &f);

#endif // CAMERAGRABBER_H
