#ifndef REMOTESYNCCLIENT_H
#define REMOTESYNCCLIENT_H

#include <QObject>

class QTcpSocket;

class RemoteSyncClient: public QObject
{
	Q_OBJECT
public:
	explicit RemoteSyncClient(QObject* parent = nullptr);

	bool connectToHost(QString server, quint16 port);
	bool disconnectFromHost();

	bool isConnected() const;

	void checkConnectionTime() const;

	void setSaveFolder(QString folder);
	void startRecording(int cameraNum);
	void saveImagesRecording(int nFrames);
	void stopRecording();

protected:

	void collectData();
	void treatAnswer();

	void sendRequest(QByteArray type, QString msg = "") const;

	void manageStartRecordActionAnswer(bool status_ok, QDateTime const& serverTime, QByteArray const& msg);
	void manageSaveImagesActionAnswer(bool status_ok, QDateTime const& serverTime, QByteArray const& msg);
	void manageStopRecordActionAnswer(bool status_ok, QDateTime const& serverTime, QByteArray const& msg);
	void manageIsRecordingActionAnswer(bool status_ok, QDateTime const& serverTime, QByteArray const& msg);
	void manageTimeMeasureActionAnswer(bool status_ok, QDateTime const& serverTime, QByteArray const& msg);

	void manageInvalidAnswer();

	QTcpSocket* _socket;

	mutable QByteArray _previousRequestType;

	QByteArray _messageBuffer;
	int _messageBufferCurrentMessagePos;
};

#endif // REMOTESYNCCLIENT_H
