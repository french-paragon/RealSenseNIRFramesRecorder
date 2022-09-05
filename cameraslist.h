#ifndef CAMERASLIST_H
#define CAMERASLIST_H

#include <QAbstractListModel>
#include <QVector>

class CamerasList : public QAbstractListModel
{
public:
	CamerasList(QObject* parent = nullptr);

	int rowCount(const QModelIndex &parent = QModelIndex()) const;

	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	std::string serialNumber(int row);
	bool isRs(int row);
	int openCvDeviceId(int row);
	int v4l2DeviceId(int row);

	void refreshCamerasList();

protected:

	static QVector<int> openCvDevicesIds();

	struct camInfos {
		std::string serialNumber;
		QString name;
		bool isRs;
		int openCvDeviceId;
		int v4l2DeviceId;

		inline QString getDescr() const {
			if (!isRs) {
				if (v4l2DeviceId >= 0) {
					return QString("%1 (V4L2 %2)").arg(name).arg(v4l2DeviceId);
				}
				if (openCvDeviceId >= 0) {
					return QString("%1 (OpenCv %2)").arg(name).arg(openCvDeviceId);
				}
			}

			return QString("%1 (RS %2)").arg(name).arg(QString::fromStdString(serialNumber));
		}
	};

	camInfos buildRsCamInfos(std::string serialNumber, QString name);
	camInfos buildOpenCvCamInfos(int id);
	camInfos buildV4L2CamInfos(int id, QString name);

	QList<camInfos> _cams;
};

#endif // CAMERASLIST_H
