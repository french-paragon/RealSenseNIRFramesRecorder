#ifndef REMOTESYNCSERVER_H
#define REMOTESYNCSERVER_H

#include <QTcpServer>

class RemoteSyncServer;

class RemoteConnectionManager : public QObject
{
	Q_OBJECT
public:
	static const int MaxMessageSize;

	static const int actionCodeBytes;

	static const char EndMsgSymbol;

	static const QByteArray SetSaveFolderActionCode;
	static const QByteArray StartRecordActionCode;
	static const QByteArray SaveImgsActionCode;
	static const QByteArray StopSaveImgsActionCode;
	static const QByteArray StopRecordActionCode;
	static const QByteArray IrPatternActionCode;
	static const QByteArray ExportRecordActionCode;
	static const QByteArray IsRecordingActionCode;
	static const QByteArray TimeMeasureActionCode;
	static const QByteArray TimeSourceActionCode;

	explicit RemoteConnectionManager(RemoteSyncServer* server, QTcpSocket* socket);

	bool configure();

	QTcpSocket* socket() const;

protected:

	void collectData();
	void treatRequest();

	void manageSetSaveFolderActionRequest(QByteArray const& msg);
	void manageStartRecordActionRequest(QByteArray const& msg);
	void manageSaveImagesActionRequest(QByteArray const& msg);
	void manageStopSaveImagesActionRequest(QByteArray const& msg);
	void manageStopRecordActionRequest(QByteArray const& msg);
	void manageInfraRedPatternActionRequest(QByteArray const& msg);
	void manageExportRecordActionRequest(QByteArray const& msg);
	void manageIsRecordingActionRequest(QByteArray const& msg);
	void manageTimeMeasureActionRequest(QByteArray const& msg);
	void manageTimeSourceActionRequest(QByteArray const& msg);

	void manageInvalidRequest();

	void sendAnswer(bool ok, QString msg = "");

	QTcpSocket* _socket;
	RemoteSyncServer* _server;

	QByteArray _messageBuffer;
	int _messageBufferCurrentMessagePos;

	friend class RemoteSyncServer;
};

class RemoteSyncServer : public QTcpServer
{
	Q_OBJECT
public:

	static const qint16 preferredPort;

	explicit RemoteSyncServer(QObject *parent = nullptr);

	bool appIsRecording() const;

Q_SIGNALS:

	void setSaveFolder(QString folder);
	void startRecording(int cameraNum);
	void saveImagesRecording(int nFrames);
	void saveImagesRecordingContinuous();
	void stopSaveImagesRecording();
	void stopRecording();
	void setInfraRedPatternOn(bool on);
	void exportRecorded();
	void setTimeSource(QString addr, quint16 port);

protected:

	void manageNewPendingConnection();

};

#endif // REMOTESYNCSERVER_H
