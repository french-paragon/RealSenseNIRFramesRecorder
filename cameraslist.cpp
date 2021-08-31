#include "cameraslist.h"

#include <librealsense2/rs.hpp>

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


void CamerasList::refreshCamerasList() {

	beginResetModel();

	rs2::context ctx;

	auto lst = ctx.query_devices();

	_cams.clear();
	_cams.reserve(lst.size());

	for (auto&& dev : lst) {

		std::string sn(dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
		std::string name(dev.get_info(RS2_CAMERA_INFO_NAME));

		camInfos info = {sn, QString::fromStdString(name)};
		_cams.push_back(info);
	}

	endResetModel();
}
