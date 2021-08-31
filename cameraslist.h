#ifndef CAMERASLIST_H
#define CAMERASLIST_H

#include <QAbstractListModel>

class CamerasList : public QAbstractListModel
{
public:
	CamerasList(QObject* parent = nullptr);

	int rowCount(const QModelIndex &parent = QModelIndex()) const;

	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	std::string serialNumber(int row);

	void refreshCamerasList();

protected:

	struct camInfos {
		std::string serialNumber;
		QString name;

		inline QString getDescr() const {
			return QString("%1 (%2)").arg(name).arg(QString::fromStdString(serialNumber));
		}
	};

	QList<camInfos> _cams;
};

#endif // CAMERASLIST_H
