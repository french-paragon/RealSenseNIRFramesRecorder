#include "cameraapplication.h"

#include "cameraslist.h"
#include "cameragrabber.h"
#include "mainwindow.h"
#include "consolewatcher.h"
#include "remotesyncserver.h"
#include "remotesyncclient.h"
#include "remoteconnectionlist.h"

#include <QApplication>
#include <QDateTime>
#include <QStandardPaths>
#include <QTextStream>

#include <QDebug>

CameraApplication* CameraApplication::CurrentApp = nullptr;

CameraApplication* CameraApplication::GetCameraApp() {
	return CurrentApp;
}

CameraApplication::CameraApplication(int &argc, char **argv) :
	QObject(nullptr)
{
	_isHeadLess = false;
	_isServer = false;

	for (int i = 1; i < argc; i++) {
		if (!qstrcmp(argv[i], "--headless")) {
			_isHeadLess = true;
		}

		if (!qstrcmp(argv[i], "--server")) {
			_isServer = true;
		}
	}

	_img_grab = nullptr;

	_imgFolder.setPath(QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).first());

	_prefferedCamera = 0;
	_lst = new CamerasList(this);
	_lst->refreshCamerasList();

	_remoteConnections = new RemoteConnectionList(this);

	_imgsToSave = 0;

	_QtApp = getAppPointer(argc, argv);
	CurrentApp = this;

	_mw = nullptr;

	connect(this, &CameraApplication::triggerStopRecording, this, &CameraApplication::stopRecording, Qt::QueuedConnection);
}

CameraApplication::~CameraApplication() {
	if (_mw != nullptr) {
		delete _mw;
	}
	delete _QtApp;
}

QCoreApplication* CameraApplication::getAppPointer(int &argc, char **argv) {

	if (_isHeadLess or _isServer) {
		return new QCoreApplication(argc, argv);
	}

	return new QApplication(argc, argv);
}

bool CameraApplication::isHeadLess() const
{
	return _isHeadLess;
}


int CameraApplication::exec() {

	configureMainWindow();
	configureConsoleWatcher();
	configureApplicationServer();

	return _QtApp->exec();
}

void CameraApplication::setExportDir(QString const& outPath) {

	QDir out(outPath);
	QTextStream outstream(stdout);

	if (out.exists()) {

		if (out.canonicalPath() == _imgFolder.canonicalPath()) {
			if (_mw == nullptr) {
				outstream << "Output folder was already " << _imgFolder.canonicalPath() << endl;
			}
		} else {
			_imgFolder = out;
			Q_EMIT outFolderChanged(out.path());
			if (_mw == nullptr) {
				outstream << "Output folder set to " << out.path() << endl;
			}
		}
	} else {
		if (_mw == nullptr) {
			outstream << "Output folder " << out.path() << " does not exist !";
		}
	}
}
QString CameraApplication::exportDir() {
	return _imgFolder.path();
}

void CameraApplication::setPrefferedCamera(int preffered) {
	if (_prefferedCamera != preffered and preffered >= 0 and preffered < _lst->rowCount()) {
		_prefferedCamera = preffered;
		Q_EMIT prefferedCameraChanged(preffered);
	}
}
int CameraApplication::prefferedCamera() const {
	return _prefferedCamera;
}

void CameraApplication::startRecordSession() {

	if (_lst->rowCount() > 0) {
		startRecording(-1);
	}

	for (int i = 0; i < _remoteConnections->rowCount(); i++) {
		_remoteConnections->getConnectionAtRow(i)->startRecording(-1);
	}

}

void CameraApplication::startRecording(int prow) {

	int row = prow;

	if (row < 0) {
		row = _prefferedCamera;
	}

	if (row < -1 or row >= _lst->rowCount()) {
		if (_mw == nullptr) {
			QTextStream out(stdout);
			out << "Failed to start recording, non existing camera " << row << endl;
		}

		return;
	}

	if (_mw == nullptr) {
		QTextStream out(stdout);
		out << "Start recording with camera " << _lst->data(_lst->index(row)).toString() << endl;
	}

	bool isRealSense = _lst->isRs(row);

	std::string sn = _lst->serialNumber(row);

	_img_grab = new CameraGrabber(this);

	if (isRealSense) {

		rs2::config config;
		config.enable_device(sn);

		_img_grab->setConfig(config);

	} else {

		_img_grab->setOpenCvDeviceId(_lst->openCvDeviceId(row));
	}

	connect(_img_grab, &CameraGrabber::framesReady, this, &CameraApplication::receiveFrames, Qt::DirectConnection);
	connect(_img_grab, &CameraGrabber::acquisitionEndedWithError, this, &CameraApplication::manageAcquisitionError);

	_img_grab->start();
}
void CameraApplication::saveFrames(int nFrames) {

	saveLocalFrames(nFrames);

	for (int i = 0; i < _remoteConnections->rowCount(); i++) {
		_remoteConnections->getConnectionAtRow(i)->saveImagesRecording(nFrames);
	}
}

void CameraApplication::saveLocalFrames(int nFrames) {

	if (isRecording()) {
		if (nFrames > 0) {
			_saveAcessControl.lock();
			_imgsToSave += nFrames;
			_saveAcessControl.unlock();
		}
	}
}

void CameraApplication::stopRecordSession() {

	if (isRecording()) {
		stopRecording();
	}

	for (int i = 0; i < _remoteConnections->rowCount(); i++) {
		_remoteConnections->getConnectionAtRow(i)->stopRecording();
	}

}
void CameraApplication::stopRecording() {

	if (_mw == nullptr) {
		QTextStream out(stdout);
		out << "Acquisition terminated !" << endl;
	}

	_img_grab->finish();
	disconnect(_img_grab, &CameraGrabber::framesReady, this, &CameraApplication::receiveFrames);
	disconnect(_img_grab, &CameraGrabber::acquisitionEndedWithError, this, &CameraApplication::manageAcquisitionError);
	_img_grab->wait();
	_img_grab->deleteLater();

	_img_grab = nullptr;
}

void CameraApplication::exportRecorded() {

	QTextStream out(stdout);

	if (isRecording()) {
		out << "Not exporting while recording !" << endl;
		return;
	}

	QList<QString> exported = _imgFolder.entryList({"*.stevimg"});

	out << "Exporting !" << endl;
	for (QString& file : exported) {
        QFileInfo info(_imgFolder.filePath(file));

		if (info.exists()) {
            ImageFrame frame(_imgFolder.filePath(file));

			if (frame.isValid()) {
				bool ok = frame.save(_imgFolder.filePath(info.baseName() + ".png"));

				if (ok) {
					out << "Exported " << file << endl;
                    QFile f(_imgFolder.filePath(file));
					f.remove();
				} else {
					out << "Could not export " << file << endl;
				}
			}
		}
	}
	out << "Exports done !" << endl;
}

void CameraApplication::connectToRemote(QString host) {

	RemoteSyncClient* remote = new RemoteSyncClient(this);
	bool ok = remote->connectToHost(host, RemoteSyncServer::preferredPort);

	if (ok) {
		_remoteConnections->addConnection(remote);
	} else {
		remote->deleteLater();
	}

}

void CameraApplication::disconnectFromRemote(QString host) {
	RemoteSyncClient* remote = _remoteConnections->getConnectionByHost(host);

	if (remote != nullptr) {
		remote->disconnectFromHost(); //remote connections should delete it, so not need to do it.
	}

}

bool CameraApplication::isRecording() const {
	return _img_grab != nullptr;
}
bool CameraApplication::isRecordingToDisk() const {
	return isRecording() and _imgsToSave;
}

void CameraApplication::receiveFrames(ImageFrame frameLeft, ImageFrame frameRight, ImageFrame frameRGB) {

	if (_imgsToSave > 0) {
		QDateTime date = QDateTime::currentDateTimeUtc();
		QString timestamp =date.toString("yyyy_MM_dd_hh_mm_ss_zzz");
		QString leftFramePath = _imgFolder.filePath(timestamp + "_left.stevimg");
		QString rightFramePath = _imgFolder.filePath(timestamp + "_right.stevimg");
		QString rgbFramePath = _imgFolder.filePath(timestamp + "_rgb.stevimg");

		frameLeft.save(leftFramePath);
		frameRight.save(rightFramePath);
		frameRGB.save(rgbFramePath);

		_saveAcessControl.lock();
		_imgsToSave--;
		_saveAcessControl.unlock();

	} else {
		if (_mw != nullptr) {
			_mw->setFrames(frameLeft, frameRight);
		}
	}

}

void CameraApplication::configureMainWindow() {

	if (!isHeadLess() and !_isServer) {
		_mw = new MainWindow();
		_mw->setCameraList(_lst);
		connect(_mw, &QObject::destroyed, this, [this] () { _mw = nullptr; });
		_mw->show();
	}
}

void CameraApplication::configureConsoleWatcher() {

	if (isHeadLess()) {
		_cw = new ConsoleWatcher(this);

		connect(_cw, &ConsoleWatcher::exitTriggered, _QtApp, &QCoreApplication::exit);

		connect(_cw, &ConsoleWatcher::setFolderTriggered, this, &CameraApplication::setExportDir);
		connect(_cw, &ConsoleWatcher::setCameraTriggered, this, &CameraApplication::setPrefferedCamera);

		connect(_cw, &ConsoleWatcher::startRecordTriggered, this, &CameraApplication::startRecording);
		connect(_cw, &ConsoleWatcher::startRecordSessionTriggered, this, &CameraApplication::startRecordSession);
		connect(_cw, &ConsoleWatcher::saveImgsTriggered, this, &CameraApplication::saveFrames);
		connect (_cw, &ConsoleWatcher::stopRecordTriggered, this, &CameraApplication::stopRecordSession);
		connect (_cw, &ConsoleWatcher::exportRecordTriggered, this, &CameraApplication::exportRecorded);

		connect (_cw, &ConsoleWatcher::listCamerasTriggered, this, [this] () {
			QTextStream out(stdout);

			for (int i = 0; i < _lst->rowCount(); i++) {
				out << "Local cameras:" << endl;
				out << "\t" << i << ". " << _lst->data(_lst->index(i)).toString() << endl;
			}
		});

		connect (_cw, &ConsoleWatcher::listConnectionsTriggered, this, [this] () {
			QTextStream out(stdout);

			for (int i = 0; i < _remoteConnections->rowCount(); i++) {
				out << "Connected servers:" << endl;
				out << "\t" << i << ". " << _remoteConnections->data(_lst->index(i)).toString() << endl;
			}
		});

		connect(_cw, &ConsoleWatcher::remoteConnectionTriggered, this, &CameraApplication::connectToRemote);
		connect(_cw, &ConsoleWatcher::remotePingTriggered, this, [this] (int row) {
			qDebug() << "checking communication time for: " << row;
			if (row >= 0 and row < _remoteConnections->rowCount()) {
				_remoteConnections->getConnectionAtRow(row)->checkConnectionTime();
			}
		});
		connect(_cw, &ConsoleWatcher::remoteDisconnectionTriggered, this, &CameraApplication::disconnectFromRemote);

		connect(_cw, &ConsoleWatcher::InvalidTriggered, this, [this] (QString line) {
			QString infos = QString("Invalid command entered:\n%1").arg(line);
			manageAcquisitionError(infos);
		});

		_cw->run();
	}
}

void CameraApplication::configureApplicationServer() {

	if (_isServer) {
		_rs = new RemoteSyncServer(this);

		connect(_rs, &RemoteSyncServer::setSaveFolder, this, &CameraApplication::setExportDir);

		connect(_rs, &RemoteSyncServer::startRecording, this, &CameraApplication::startRecording);
		connect(_rs, &RemoteSyncServer::saveImagesRecording, this, &CameraApplication::saveLocalFrames);
		connect (_rs, &RemoteSyncServer::stopRecording, this, &CameraApplication::stopRecording);

		_rs->listen(QHostAddress::Any, RemoteSyncServer::preferredPort);

		QTextStream out(stdout);
		out << "RealSense NIR Frame recorder - server mode" << "\n";
		out << QDateTime::currentDateTime().toString() << "\n";
		out << "Started listening on port " << _rs->serverPort() << endl;
	}

}

void CameraApplication::manageAcquisitionError(QString txt) {
	if (_mw != nullptr) {
		_mw->showErrorMessage(txt);
	} else {
		QTextStream err(stderr);
		err << txt << endl;
	}
}
