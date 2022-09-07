#ifndef REMOTESYNCCLIENT_H
#define REMOTESYNCCLIENT_H

#include <QObject>

class QTcpSocket;

class RemoteSyncClient: public QObject
{
	Q_OBJECT

public:

	explicit RemoteSyncClient(QObject* parent = nullptr);
	virtual ~RemoteSyncClient();

	bool connectToHost(QString server, quint16 port);
	bool disconnectFromHost();

	bool isConnected() const;

	void checkConnectionTime();

	void setSaveFolder(QString folder);
	void startRecording(int cameraNum);
	void saveImagesRecording(int nFrames);
	void stopRecording();
	void triggerExport();

	QString getHost() const;
	QString getDescr() const;

Q_SIGNALS:

	void connection_terminated();

protected:

	void collectData();
	void treatAnswer();

	void sendRequest(QByteArray type, QString msg = "");

	void manageStartRecordActionAnswer(bool status_ok, QDateTime const& serverTime, QByteArray const& msg);
	void manageSaveImagesActionAnswer(bool status_ok, QDateTime const& serverTime, QByteArray const& msg);
	void manageStopRecordActionAnswer(bool status_ok, QDateTime const& serverTime, QByteArray const& msg);
	void manageIsRecordingActionAnswer(bool status_ok, QDateTime const& serverTime, QByteArray const& msg);
	void manageTimeMeasureActionAnswer(bool status_ok, QDateTime const& serverTime, QByteArray const& msg);

	void manageInvalidAnswer();
	void manageFailingConnection();

	QTcpSocket* _socket;

	mutable QByteArray _previousRequestType;

	mutable QByteArray _messageBuffer;
	mutable int _messageBufferCurrentMessagePos;
};

#endif // REMOTESYNCCLIENT_H
