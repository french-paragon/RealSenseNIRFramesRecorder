#include "cameraslist.h"

#include <librealsense2/rs.hpp>
#include <opencv2/videoio.hpp>

#include <QVector>
#include <QDebug>

#include "v4l2camera.h"

CamerasList::CamerasList(QObject *parent) : QAbstractListModel(parent)
{

}

int CamerasList::rowCount(const QModelIndex &parent) const {

	if (parent == QModelIndex()) {
		return _cams.size();
	}

	return 0;
}


QVariant CamerasList::data(const QModelIndex &index, int role) const {

	if (index.parent() != QModelIndex()) {
		return QVariant();
	}

	if (index.row() < 0 or index.row() >= _cams.size()) {
		return QVariant();
	}

	switch (role) {
	case Qt::DisplayRole:
		return _cams.at(index.row()).getDescr();
	default:
		break;
	}

	return QVariant();
}
std::string CamerasList::serialNumber(int row) {
	return _cams[row].serialNumber;
}

bool CamerasList::isRs(int row) {
	return _cams[row].isRs;
}
int CamerasList::openCvDeviceId(int row) {
	return _cams[row].openCvDeviceId;
}

int CamerasList::v4l2DeviceId(int row) {
	return _cams[row].v4l2DeviceId;
}

QVector<int> CamerasList::openCvDevicesIds() {
	bool hasCam = true;
	int device_id = 0;
	QVector<int> devices;

	while(hasCam) {
		cv::VideoCapture camera = cv::VideoCapture(device_id);

		if (!camera.isOpened()) {
			hasCam = false; //Note, sometimes some devices will not work.
		} else {
			devices.append(device_id);
		}
		device_id++;
	}

	return devices;
}


CamerasList::camInfos CamerasList::buildRsCamInfos(std::string serialNumber, QString name) {
	return {serialNumber, name, true, -1, -1};
}
CamerasList::camInfos CamerasList::buildOpenCvCamInfos(int device_id) {
	return {QString("cv%1").arg(device_id).toStdString(), QString("OpenCV cam %1").arg(device_id), false, device_id, -1};
}
CamerasList::camInfos CamerasList::buildV4L2CamInfos(int id, QString name) {
	return {QString("v4l2_%1").arg(id).toStdString(), name, false, -1, id};
}

void CamerasList::refreshCamerasList() {

	beginResetModel();

	rs2::context ctx;

	auto lst = ctx.query_devices();

	//QVector<int> devlst = openCvDevicesIds();

	QVector<V4L2Camera::Descriptor> devlst = V4L2Camera::listAvailableCameras();

	_cams.clear();
	_cams.reserve(lst.size() + devlst.size());

	for (auto&& dev : lst) {

		std::string sn(dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
		std::string name(dev.get_info(RS2_CAMERA_INFO_NAME));

		camInfos info = buildRsCamInfos(sn, QString::fromStdString(name));
		_cams.push_back(info);
	}

	for (V4L2Camera::Descriptor const& descriptor : devlst) {

		camInfos info = buildV4L2CamInfos(descriptor.index,descriptor.name);
		_cams.push_back(info);
	}

	endResetModel();
}
