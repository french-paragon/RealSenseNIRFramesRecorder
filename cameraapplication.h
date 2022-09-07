#ifndef CAMERAAPPLICATION_H
#define CAMERAAPPLICATION_H

#include <QObject>
#include <QMutex>
#include <QDir>

#include "./imageframe.h"

class QCoreApplication;
class QTimer;

class MainWindow;
class ConsoleWatcher;
class CamerasList;
class CameraGrabber;
class RemoteSyncServer;
class RemoteConnectionList;

class CameraApplication : public QObject
{
	Q_OBJECT
public:
	static CameraApplication* GetCameraApp();

	explicit CameraApplication(int & argc, char** argv);
	~CameraApplication();

	bool isHeadLess() const;

	int exec();

	void setExportDir(QString const& outPath);
	QString exportDir();

	void setPrefferedCamera(int preffered);
	int prefferedCamera() const;

	void startRecordSession();
	void startRecording(int row);
	void saveFrames(int nFrames);
	void saveLocalFrames(int nFrames);
	void stopRecordSession();
	void stopRecording();
	void exportRecording();
	void exportRecorded();

	void connectToRemote(QString host);
	void disconnectFromRemote(QString host);

	bool isRecording() const;
	bool isRecordingToDisk() const;

Q_SIGNALS:

	void outFolderChanged(QString outFolder);
	void triggerStopRecording();
	void prefferedCameraChanged(int camRow);

	void serverAboutToStart();

protected:

	void pingAll();

	void receiveFrames(ImageFrame frameLeft, ImageFrame frameRight, ImageFrame frameRGB);

	void configureMainWindow();
	void configureConsoleWatcher();
	void configureApplicationServer();

	void manageAcquisitionError(QString txt);

	void timingInfoReceived(QString peerName,
							QString peerAddr,
							qint64 sent_ms,
							qint64 server_ms,
							qint64 now_ms);

	static CameraApplication* CurrentApp;

	QCoreApplication* getAppPointer(int &argc, char **argv);

	QCoreApplication* _QtApp;

	bool _isHeadLess;
	bool _isServer;

	int _prefferedCamera;
	CamerasList* _lst;
	CameraGrabber* _img_grab;

	RemoteConnectionList* _remoteConnections;
	QFile* _sessionTimingFile;
	QTimer* _pingTimer;

	QMutex _saveAcessControl;

	QDir _imgFolder;
	int _imgsToSave;

	MainWindow* _mw;
	ConsoleWatcher* _cw;
	RemoteSyncServer* _rs;

	QThread* _serverThread;

};

#endif // CAMERAAPPLICATION_H
