#include "remotesyncserver.h"

#include <QTcpSocket>
#include <QDateTime>

#include "cameraapplication.h"

const int RemoteConnectionManager::MaxMessageSize = 256;

const int RemoteConnectionManager::actionCodeBytes = 4;

const char RemoteConnectionManager::EndMsgSymbol = '\n';

const QByteArray RemoteConnectionManager::SetSaveFolderActionCode = QByteArray("svfl",4); //save folder
const QByteArray RemoteConnectionManager::StartRecordActionCode = QByteArray("strt",4); //start
const QByteArray RemoteConnectionManager::SaveImgsActionCode = QByteArray("save",4); //save
const QByteArray RemoteConnectionManager::StopRecordActionCode = QByteArray("stop",4); //stop
const QByteArray RemoteConnectionManager::IsRecordingActionCode = QByteArray("ircd",4); //is recording
const QByteArray RemoteConnectionManager::TimeMeasureActionCode = QByteArray("ttdm",4); //transit time delay measure

RemoteConnectionManager::RemoteConnectionManager(RemoteSyncServer *server, QTcpSocket* socket) :
	QObject(server),
	_socket(socket),
	_server(server)
{
	_messageBufferCurrentMessagePos = -1;
}

bool RemoteConnectionManager::configure() {

	connect(_socket, &QIODevice::readyRead, this, &RemoteConnectionManager::collectData);

	connect(_socket, &QAbstractSocket::disconnected, this, &QObject::deleteLater);
	connect(_socket, &QObject::destroyed, this, &QObject::deleteLater);

	return true;

}

QTcpSocket* RemoteConnectionManager::socket() const {
	return _socket;
}
void RemoteConnectionManager::collectData() {

	while (_socket->bytesAvailable() > 0) {

		_messageBuffer += _socket->read(MaxMessageSize);

		while (_messageBufferCurrentMessagePos != _messageBuffer.size()) {

			if (_messageBuffer[_messageBufferCurrentMessagePos] == EndMsgSymbol) {

				treatRequest();
				_messageBuffer = _messageBuffer.mid(_messageBufferCurrentMessagePos+1);
				_messageBufferCurrentMessagePos = 0;

			} else if (_messageBufferCurrentMessagePos == MaxMessageSize-1) {

				_messageBuffer = _messageBuffer.mid(_messageBufferCurrentMessagePos+1);
				_messageBufferCurrentMessagePos = 0;
				manageInvalidRequest();

			}

			_messageBufferCurrentMessagePos++;

		}
	}

}

void RemoteConnectionManager::treatRequest() {

	if (_messageBufferCurrentMessagePos < 0) {
		return;
	}

	if (_messageBuffer[_messageBufferCurrentMessagePos] != EndMsgSymbol) {
		return;
	}

	QByteArray msg = _messageBuffer.left(_messageBufferCurrentMessagePos+1);

	if (msg.length() < actionCodeBytes) {
		manageInvalidRequest();
	}

	QByteArray actionCode = msg.left(actionCodeBytes);

	if (actionCode == SetSaveFolderActionCode) {
		manageSetSaveFolderActionRequest(msg.mid(actionCodeBytes));
		return;
	}

	if (actionCode == StartRecordActionCode) {
		manageStartRecordActionRequest(msg.mid(actionCodeBytes));
		return;
	}

	if (actionCode == SaveImgsActionCode) {
		manageSaveImagesActionRequest(msg.mid(actionCodeBytes));
		return;
	}

	if (actionCode == StopRecordActionCode) {
		manageStopRecordActionRequest(msg.mid(actionCodeBytes));
		return;
	}

	if (actionCode == TimeMeasureActionCode) {
		manageTimeMeasureActionRequest(msg.mid(actionCodeBytes));
		return;
	}

	// if request code not recognized
	manageInvalidRequest();
}

void RemoteConnectionManager::manageSetSaveFolderActionRequest(QByteArray const& msg) {

	QString data = QString::fromUtf8(msg);
	_server->setSaveFolder(data);
	sendAnswer(true);
}

void RemoteConnectionManager::manageStartRecordActionRequest(QByteArray const& msg) {

	QString data = QString::fromUtf8(msg);

	bool ok;
	int camNum = data.toInt(&ok, 16);

	if (ok) {
		_server->startRecording(camNum);
		sendAnswer(true);
	} else {
		sendAnswer(false);
	}

}
void RemoteConnectionManager::manageSaveImagesActionRequest(QByteArray const& msg) {

	QString data = QString::fromUtf8(msg);

	bool ok;
	int nFrames = data.toInt(&ok, 16);

	if (ok) {
		_server->saveImagesRecording(nFrames);
		sendAnswer(true);
	} else {
		sendAnswer(false);
	}

}
void RemoteConnectionManager::manageStopRecordActionRequest(QByteArray const& msg) {
	Q_UNUSED(msg);
	_server->stopRecording();
	sendAnswer(true);

}
void RemoteConnectionManager::manageTimeMeasureActionRequest(QByteArray const& msg) {
	QByteArray ans = msg;

	sendAnswer(true, ans);
}
void RemoteConnectionManager::manageIsRecordingActionRequest(QByteArray const& msg) {
	Q_UNUSED(msg);

	char status = (_server->appIsRecording()) ? 'y' : 'n';
	QByteArray ans(&status,1);

	sendAnswer(true, ans);
}

void RemoteConnectionManager::manageInvalidRequest() {

	sendAnswer(false, "invalid");
}

void RemoteConnectionManager::sendAnswer(bool ok, QString msg) {

	char code = (ok) ? 'y' : 'n';
	QByteArray ans(&code,1);

	QDateTime now = QDateTime::currentDateTimeUtc();
	qint64 ms = now.currentMSecsSinceEpoch();
	ans += QString("%1").arg(ms, 0, 16).toUtf8();

	QByteArray msgdata = msg.toUtf8();

	if (msgdata.length() > 0) {
		ans += ' ';
	}

	ans += msgdata;

	code = EndMsgSymbol;
	QByteArray end(&code,1);
	ans += end;

	_socket->write(ans);
}

const qint16 RemoteSyncServer::preferredPort = 5050;

RemoteSyncServer::RemoteSyncServer(QObject *parent) : QTcpServer(parent)
{

}

bool RemoteSyncServer::appIsRecording() const {
	return CameraApplication::GetCameraApp()->isRecordingToDisk();
}


void RemoteSyncServer::newPendingConnection() {

	QTcpSocket* socket = nextPendingConnection();
	RemoteConnectionManager* manager = new RemoteConnectionManager(this, socket);

	manager->configure();

}
