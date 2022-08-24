#include "remotesyncclient.h"

#include <QTextStream>
#include <QTcpSocket>
#include <QHostAddress>
#include <QDateTime>

#include "remotesyncserver.h"

RemoteSyncClient::RemoteSyncClient(QObject *parent) :
	QObject(parent),
	_socket(nullptr)
{
	_messageBufferCurrentMessagePos = -1;
}

void RemoteSyncClient::collectData() {

	while (_socket->bytesAvailable() > 0) {

		_messageBuffer += _socket->read(RemoteConnectionManager::MaxMessageSize);

		while (_messageBufferCurrentMessagePos != _messageBuffer.size()) {

			if (_messageBuffer[_messageBufferCurrentMessagePos] == RemoteConnectionManager::EndMsgSymbol) {

				treatAnswer();
				_messageBuffer = _messageBuffer.mid(_messageBufferCurrentMessagePos+1);
				_messageBufferCurrentMessagePos = 0;

			} else if (_messageBufferCurrentMessagePos == RemoteConnectionManager::MaxMessageSize-1) {

				_messageBuffer = _messageBuffer.mid(_messageBufferCurrentMessagePos+1);
				_messageBufferCurrentMessagePos = 0;
				manageInvalidAnswer();

			}

			_messageBufferCurrentMessagePos++;

		}
	}

}
void RemoteSyncClient::treatAnswer() {

	if (_previousRequestType.isEmpty()) {
		return;
	}

	if (_messageBufferCurrentMessagePos < 0) {
		return;
	}

	if (_messageBuffer[_messageBufferCurrentMessagePos] != RemoteConnectionManager::EndMsgSymbol) {
		return;
	}

	QByteArray msg = _messageBuffer.left(_messageBufferCurrentMessagePos+1);

	if (msg.length() < RemoteConnectionManager::actionCodeBytes) {
		manageInvalidAnswer();
	}

	char status = msg.at(0);

	bool status_ok = false;

	if (status == 'y') {
		status_ok = true;

	} else if (status != 'n') {
		manageInvalidAnswer();
		return;
	}

	msg = msg.mid(1);
	int space_pos = msg.indexOf(' ');

	if (space_pos < 0) {
		manageInvalidAnswer();
		return;
	}

	QByteArray timestamp = msg.left(space_pos);

	bool int_ok;
	qint64 ms_server = QString::fromUtf8(timestamp).toInt(&int_ok, 16);

	if (!int_ok) {
		manageInvalidAnswer();
		return;
	}

	QDateTime serverTime = QDateTime::fromMSecsSinceEpoch(ms_server);

	if (_previousRequestType == RemoteConnectionManager::StartRecordActionCode) {
		manageStartRecordActionAnswer(status_ok, serverTime, msg.mid(space_pos+1));
		return;
	}

	if (_previousRequestType == RemoteConnectionManager::StopRecordActionCode) {
		manageStopRecordActionAnswer(status_ok, serverTime, msg.mid(space_pos+1));
		return;
	}

	if (_previousRequestType == RemoteConnectionManager::TimeMeasureActionCode) {
		manageTimeMeasureActionAnswer(status_ok, serverTime, msg.mid(space_pos+1));
		return;
	}

	// if request code not recognized
	manageInvalidAnswer();
}

bool RemoteSyncClient::connectToHost(QString server, quint16 port) {

	disconnect();

	_socket = new QTcpSocket(this);
	_socket->connectToHost(server, port);

	connect(_socket, &QIODevice::readyRead, this, &RemoteSyncClient::collectData);

	connect(_socket, &QAbstractSocket::disconnected, this, &RemoteSyncClient::disconnectFromHost);
	connect(_socket, &QObject::destroyed, this, &RemoteSyncClient::disconnectFromHost);

	bool ok = _socket->waitForConnected();

	if (!ok) {
		QTextStream err(stderr);

		err << "Impossible to open connection to " << _socket->peerName() << " [" << _socket->peerAddress().toString() << "]\n";
		err << "Error: " << _socket->error() << Qt::endl;
	}

	return ok;
}


bool RemoteSyncClient::disconnectFromHost() {

	bool ok = true;

	if (_socket != nullptr) {

		ok = _socket->waitForDisconnected();

		if (!ok) {
			QTextStream err(stderr);

			err << "Impossible to close connection to " << _socket->peerName() << " [" << _socket->peerAddress().toString() << "]\n";
			err << "Error: " << _socket->error() << Qt::endl;
		}

		disconnect(_socket, &QIODevice::readyRead, this, &RemoteSyncClient::collectData);

		disconnect(_socket, &QAbstractSocket::disconnected, this, &RemoteSyncClient::disconnectFromHost);
		disconnect(_socket, &QObject::destroyed, this, &RemoteSyncClient::disconnectFromHost);

		_socket->deleteLater();
		_socket = nullptr;
	}

	return ok;
}

bool RemoteSyncClient::isConnected() const {

	if (_socket != nullptr) {
		return _socket->state() == QAbstractSocket::ConnectedState;
	}

	return false;
}

void RemoteSyncClient::checkConnectionTime() const {

	if (isConnected()) {

		QDateTime now = QDateTime::currentDateTimeUtc();
		qint64 ms = now.currentMSecsSinceEpoch();

		sendRequest(RemoteConnectionManager::TimeMeasureActionCode, QString("%1").arg(ms, 0, 16).toUtf8());

	}

}

void RemoteSyncClient::setSaveFolder(QString folder) {
	if (isConnected()) {
		sendRequest(RemoteConnectionManager::SetSaveFolderActionCode, folder.toUtf8());
	}
}
void RemoteSyncClient::startRecording(int cameraNum) {
	if (isConnected()) {
		sendRequest(RemoteConnectionManager::StartRecordActionCode, QString("%1").arg(cameraNum, 0, 16).toUtf8());
	}
}
void RemoteSyncClient::saveImagesRecording(int nFrames) {
	if (isConnected()) {
		sendRequest(RemoteConnectionManager::SaveImgsActionCode, QString("%1").arg(nFrames, 0, 16).toUtf8());
	}
}
void RemoteSyncClient::stopRecording() {
	if (isConnected()) {
		sendRequest(RemoteConnectionManager::StopRecordActionCode);
	}
}


void RemoteSyncClient::sendRequest(QByteArray type, QString msg) const {

	if (isConnected() and _previousRequestType.isEmpty()) {

		_previousRequestType = type;

		QByteArray ans=type;

		/*QDateTime now = QDateTime::currentDateTimeUtc();
		qint64 ms = now.currentMSecsSinceEpoch();
		ans += QString("%1").arg(ms, 0, 16).toUtf8();*/

		QByteArray msgdata = msg.toUtf8();

		/*if (msgdata.length() > 0) {
			ans += ' ';
		}*/

		ans += msgdata;

		char code = RemoteConnectionManager::EndMsgSymbol;
		QByteArray end(&code,1);
		ans += end;

		_socket->write(ans);
	}

}

void RemoteSyncClient::manageStartRecordActionAnswer(bool status_ok, QDateTime const& serverTime, QByteArray const& msg) {

	Q_UNUSED(msg);

	QTextStream out(stdout);

	out << "Start record action answer received! Status is : " << ((status_ok) ? "ok" : "error") << "\n\t";
	out << "Server time was :" << serverTime.toString() << Qt::endl;

}
void RemoteSyncClient::manageSaveImagesActionAnswer(bool status_ok, QDateTime const& serverTime, QByteArray const& msg) {

	Q_UNUSED(msg);

	QTextStream out(stdout);

	out << "Save images action answer received! Status is : " << ((status_ok) ? "ok" : "error") << "\n\t";
	out << "Server time was :" << serverTime.toString() << Qt::endl;
}
void RemoteSyncClient::manageStopRecordActionAnswer(bool status_ok, QDateTime const& serverTime, QByteArray const& msg) {

	Q_UNUSED(msg);

	QTextStream out(stdout);

	out << "Stop record action answer received! Status is : " << ((status_ok) ? "ok" : "error") << "\n\t";
	out << "Server time was :" << serverTime.toString() << Qt::endl;

}
void RemoteSyncClient::manageIsRecordingActionAnswer(bool status_ok, QDateTime const& serverTime, QByteArray const& msg) {

	Q_UNUSED(msg);

	QTextStream out(stdout);

	out << "Is recording action answer received! Status is : " << ((status_ok) ? "ok" : "error") << "\n\t";
	out << "Server time was :" << serverTime.toString() << Qt::endl;

}
void RemoteSyncClient::manageTimeMeasureActionAnswer(bool status_ok, QDateTime const& serverTime, QByteArray const& msg) {

	qint64 now_ms = QDateTime::currentDateTimeUtc().currentMSecsSinceEpoch();

	bool ok = true;
	qint64 sent_ms = msg.toLongLong(&ok);

	if (!ok) {
		return;
	}

	qint64 server_ms = serverTime.currentMSecsSinceEpoch();

	QTextStream out(stdout);

	out << "Measure connection time answer received! Status is : " << ((status_ok) ? "ok" : "error") << "\n\t";
	out << "Submission time [ms] :" << sent_ms << "\n\t";
	out << "Reception time [ms] :" << now_ms << "\n\t";
	out << "time delta [ms] :" << (now_ms - sent_ms) << "\n\t";
	out << "Server time [ms] :" << server_ms << Qt::endl;
}

void RemoteSyncClient::manageInvalidAnswer() {

}
