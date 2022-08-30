#include "remoteconnectionlist.h"

#include "remotesyncclient.h"

RemoteConnectionList::RemoteConnectionList(QObject *parent) : QAbstractListModel(parent)
{

}

int RemoteConnectionList::rowCount(const QModelIndex &parent) const {
	Q_UNUSED(parent);
	return _connections.size();
}

QVariant RemoteConnectionList::data(const QModelIndex &index, int role) const {

	if (index.parent() != QModelIndex()) {
		return QVariant();
	}

	if (index.row() < 0 or index.row() >= _connections.size()) {
		return QVariant();
	}

	switch (role) {
	case Qt::DisplayRole:
		return _connections.at(index.row())->getDescr();
	default:
		break;
	}

	return QVariant();

}

RemoteSyncClient* RemoteConnectionList::getConnectionAtRow(int row) {
	return _connections.at(row);
}
RemoteSyncClient* RemoteConnectionList::getConnectionByHost(QString host) {

	for (RemoteSyncClient* remote : _connections) {

		if (remote->getHost() == host) {
			return remote;
		}
	}

	return nullptr;
}


void RemoteConnectionList::addConnection(RemoteSyncClient* connection) {

	connect(connection, &RemoteSyncClient::connection_terminated, this, [this, connection] () {

		connection->deleteLater();
		removeConnection(connection);

	}, Qt::DirectConnection);

	connect(connection, &QObject::destroyed, this, [this, connection] () {

		removeConnection(connection);

	}, Qt::DirectConnection);

	beginInsertRows(QModelIndex(), _connections.size(), _connections.size());

	_connectionListMutex.lock();
	_connections.push_back(connection);
	_connectionListMutex.unlock();

	endInsertRows();
}

void RemoteConnectionList::removeConnection(RemoteSyncClient* connection) {

	_connectionListMutex.lock();
	int row = _connections.indexOf(connection);

	if (row >= 0) {
		beginRemoveRows(QModelIndex(),row,row);
		_connections.removeAt(row);
		endRemoveRows();
	}
	_connectionListMutex.unlock();

}
