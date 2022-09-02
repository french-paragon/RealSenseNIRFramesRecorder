#include "cameraslist.h"

#include <librealsense2/rs.hpp>
#include <opencv2/videoio.hpp>

#include <QVector>
#include <QDebug>

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

void CamerasList::refreshCamerasList() {

	beginResetModel();

	rs2::context ctx;

	auto lst = ctx.query_devices();

	QVector<int> devlst = openCvDevicesIds();

	_cams.clear();
	_cams.reserve(lst.size() + devlst.size());

	for (auto&& dev : lst) {

		std::string sn(dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
		std::string name(dev.get_info(RS2_CAMERA_INFO_NAME));

		camInfos info = {sn, QString::fromStdString(name), true, 0};
		_cams.push_back(info);
	}

	for (int device_id : devlst) {

		camInfos info = {QString("cv%1").arg(device_id).toStdString(), QString("OpenCV cam %1").arg(device_id), false, device_id};
		_cams.push_back(info);
	}

	endResetModel();
}
