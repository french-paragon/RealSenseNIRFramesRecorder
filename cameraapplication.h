#ifndef CAMERAAPPLICATION_H
#define CAMERAAPPLICATION_H

#include <QObject>
#include <QMutex>
#include <QDir>

#include "./imageframe.h"

class QCoreApplication;

class MainWindow;
class ConsoleWatcher;
class CamerasList;
class CameraGrabber;
class RemoteSyncServer;

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

	void startRecording(int row);
	void saveFrames(int nFrames);
	void stopRecording();

	bool isRecording() const;
	bool isRecordingToDisk() const;

Q_SIGNALS:

	void outFolderChanged(QString outFolder);
	void triggerStopRecording();

protected:

	void receiveFrames(ImageFrame frameLeft, ImageFrame frameRight);
	void configureMainWindow();
	void configureConsoleWatcher();
	void configureApplicationServer();

	void manageAcquisitionError(QString txt);

	static CameraApplication* CurrentApp;

	QCoreApplication* getAppPointer(int &argc, char **argv);

	QCoreApplication* _QtApp;

	bool _isHeadLess;
	bool _isServer;

	CamerasList* _lst;
	CameraGrabber* _img_grab;

	QMutex _saveAcessControl;

	QDir _imgFolder;
	int _imgsToSave;

	MainWindow* _mw;
	ConsoleWatcher* _cw;
	RemoteSyncServer* _rs;

};

#endif // CAMERAAPPLICATION_H
