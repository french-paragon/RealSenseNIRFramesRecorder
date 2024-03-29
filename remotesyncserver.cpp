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
const QByteArray RemoteConnectionManager::StopSaveImgsActionCode = QByteArray("spsv",4); //save
const QByteArray RemoteConnectionManager::StopRecordActionCode = QByteArray("stop",4); //stop
const QByteArray RemoteConnectionManager::IrPatternActionCode = QByteArray("irpt",4); //ir pattern
const QByteArray RemoteConnectionManager::ExportRecordActionCode = QByteArray("xprt",4); //export
const QByteArray RemoteConnectionManager::IsRecordingActionCode = QByteArray("ircd",4); //is recording
const QByteArray RemoteConnectionManager::TimeMeasureActionCode = QByteArray("ttdm",4); //transit time delay measure
const QByteArray RemoteConnectionManager::TimeSourceActionCode = QByteArray("tsst",4); //transit time delay measure

RemoteConnectionManager::RemoteConnectionManager(RemoteSyncServer *server, QTcpSocket* socket) :
	QObject(server),
	_socket(socket),
	_server(server)
{
	_messageBufferCurrentMessagePos = 0;
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

		qDebug() << "Message buffer grows: " << _messageBuffer;

		while (_messageBufferCurrentMessagePos != _messageBuffer.size()) {

			if (_messageBuffer[_messageBufferCurrentMessagePos] == EndMsgSymbol) {

				treatRequest();
				_messageBuffer = _messageBuffer.mid(_messageBufferCurrentMessagePos+1);
				_messageBufferCurrentMessagePos = -1;

				qDebug() << "Message buffer after treating request: " << _messageBuffer << " pos: " << _messageBufferCurrentMessagePos;

			} else if (_messageBufferCurrentMessagePos == MaxMessageSize-1) {

				_messageBuffer = _messageBuffer.mid(_messageBufferCurrentMessagePos+1);
				_messageBufferCurrentMessagePos = -1;
				manageInvalidRequest();

				qDebug() << "Message buffer after invalid request: " << _messageBuffer << " pos: " << _messageBufferCurrentMessagePos;

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

	QByteArray msg = _messageBuffer.left(_messageBufferCurrentMessagePos);

	if (msg.length() < actionCodeBytes) {
		manageInvalidRequest();
	}

	QByteArray actionCode = msg.left(actionCodeBytes);

	qDebug() << "Request received for: " << actionCode;

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

	if (actionCode == StopSaveImgsActionCode) {
		manageStopSaveImagesActionRequest(msg.mid(actionCodeBytes));
		return;
	}

	if (actionCode == StopRecordActionCode) {
		manageStopRecordActionRequest(msg.mid(actionCodeBytes));
		return;
	}

	if (actionCode == IrPatternActionCode) {
		manageInfraRedPatternActionRequest(msg.mid(actionCodeBytes));
		return;
	}

	if (actionCode == ExportRecordActionCode) {
		manageExportRecordActionRequest(msg.mid(actionCodeBytes));
		return;
	}

	if (actionCode == TimeMeasureActionCode) {
		manageTimeMeasureActionRequest(msg.mid(actionCodeBytes));
		return;
	}

	if (actionCode == TimeSourceActionCode) {
		manageTimeSourceActionRequest(msg.mid(actionCodeBytes));
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
    int camNum = data.toInt(&ok, 10);

    qDebug() << "Start recording action request received with message: " << msg << " camNum: " << camNum << " status: " << ok;

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
    int nFrames = data.toInt(&ok, 10);

	if (!ok and (data.toLower() == "c")) { //continuous mode
		ok = true;
		nFrames = -1;
	}

	qDebug() << "Frame save action request received with message: " << msg << " nFrames: " << ((nFrames > 0) ? QString("%1").arg(nFrames) : "infinity") << " status: " << ok;

	if (ok) {
		if (nFrames > 0) {
			_server->saveImagesRecording(nFrames);
		} else {
			_server->saveImagesRecordingContinuous();
		}
		sendAnswer(true);
	} else {
		sendAnswer(false);
	}

}
void RemoteConnectionManager::manageStopSaveImagesActionRequest(QByteArray const& msg) {

	qDebug() << "Stop saving images action request received with message: " << msg;

	Q_UNUSED(msg);
	_server->stopSaveImagesRecording();
	sendAnswer(true);

}
void RemoteConnectionManager::manageStopRecordActionRequest(QByteArray const& msg) {

    qDebug() << "Stop recording action request received with message: " << msg;

	Q_UNUSED(msg);
	_server->stopRecording();
	sendAnswer(true);

}

void RemoteConnectionManager::manageInfraRedPatternActionRequest(QByteArray const& msg) {

	QString data = QString::fromUtf8(msg);

	qDebug() << "Infrared pattern action request received with message: " << msg;

	if (data.toLower() == "on") {
		_server->setInfraRedPatternOn(true);
		sendAnswer(true);
	} else if (data.toLower() == "off") {
		_server->setInfraRedPatternOn(false);
		sendAnswer(true);
	} else {
		sendAnswer(false);
	}

}

void RemoteConnectionManager::manageExportRecordActionRequest(QByteArray const& msg) {

	qDebug() << "Export recording action request received with message: " << msg;

	Q_UNUSED(msg);
	_server->exportRecorded();
	sendAnswer(true);
}

void RemoteConnectionManager::manageTimeMeasureActionRequest(QByteArray const& msg) {

	qDebug() << "Timing action request received with message: " << msg;

	QByteArray ans = msg;

	sendAnswer(true, ans);
}

void RemoteConnectionManager::manageTimeSourceActionRequest(QByteArray const& msg) {

	qDebug() << "Time source request received with message: " << msg;

	QString params = QString::fromUtf8(msg);

	QStringList split = params.split(' ', QString::SplitBehavior::SkipEmptyParts);

	if (split.size() != 2) {
		sendAnswer(false);
		return;
	}

	bool ok = true;
	quint16 port = split[1].toUInt(&ok);

	if (!ok) {
		sendAnswer(false);
		return;
	}

	QHostAddress addr(split[0]);

	if (addr.isNull()) {
		sendAnswer(false);
		return;
	}

	_server->setTimeSource(split[0], port);

	sendAnswer(true);

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

	qint64 ms = CameraApplication::GetCameraApp()->getTimeMs();
	ans += QString("%1").arg(ms, 0, 16).toUtf8();

	QByteArray msgdata = msg.toUtf8();

	if (msgdata.length() > 0) {
		ans += ' ';
	}

	ans += msgdata;

	code = EndMsgSymbol;
	QByteArray end(&code,1);
	ans += end;

	qDebug() << "Sending answer: " << ans;

	_socket->write(ans);
}

const qint16 RemoteSyncServer::preferredPort = 5050;

RemoteSyncServer::RemoteSyncServer(QObject *parent) : QTcpServer(parent)
{
	connect(this, &QTcpServer::newConnection, this, &RemoteSyncServer::manageNewPendingConnection);
}

bool RemoteSyncServer::appIsRecording() const {
	return CameraApplication::GetCameraApp()->isRecordingToDisk();
}


void RemoteSyncServer::manageNewPendingConnection() {

	QTcpSocket* socket = nextPendingConnection();

	QTextStream out(stdout);
	out << "received a connection from " << socket->peerAddress().toString() << endl;

	RemoteConnectionManager* manager = new RemoteConnectionManager(this, socket);

	manager->configure();

}
