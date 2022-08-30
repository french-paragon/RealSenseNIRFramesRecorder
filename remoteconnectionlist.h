#ifndef REMOTECONNECTIONLIST_H
#define REMOTECONNECTIONLIST_H

#include <QAbstractListModel>
#include <QMutex>

class RemoteSyncClient;

class RemoteConnectionList : public QAbstractListModel
{
	Q_OBJECT
public:
	explicit RemoteConnectionList(QObject *parent = nullptr);

	int rowCount(const QModelIndex &parent = QModelIndex()) const;

	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

	RemoteSyncClient* getConnectionAtRow(int row);
	RemoteSyncClient* getConnectionByHost(QString host);

	void addConnection(RemoteSyncClient* connection);
	void removeConnection(RemoteSyncClient* connection);

Q_SIGNALS:

protected:

	QMutex _connectionListMutex;
	QVector<RemoteSyncClient*> _connections;

};

#endif // REMOTECONNECTIONLIST_H
