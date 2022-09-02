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

	void refreshCamerasList();

protected:

	static QVector<int> openCvDevicesIds();

	struct camInfos {
		std::string serialNumber;
		QString name;
		bool isRs;
		int openCvDeviceId;

		inline QString getDescr() const {
			return QString("%1 (RS %2)").arg(name).arg(QString::fromStdString(serialNumber));
		}
	};

	QList<camInfos> _cams;
};

#endif // CAMERASLIST_H
