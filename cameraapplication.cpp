#include "cameraapplication.h"

#include "cameraslist.h"
#include "cameragrabber.h"
#include "mainwindow.h"
#include "consolewatcher.h"
#include "remotesyncserver.h"

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

	_imgFolder.setPath(QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).first());

	_lst = new CamerasList(this);
	_lst->refreshCamerasList();

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

	return _QtApp->exec();
}

void CameraApplication::setExportDir(QString const& outPath) {

	QDir out(outPath);
	QTextStream outstream(stdout);

	if (out.exists()) {

		if (out.canonicalPath() == _imgFolder.canonicalPath()) {
			if (_mw == nullptr) {
				outstream << "Output folder was already " << _imgFolder.canonicalPath() << Qt::endl;
			}
		} else {
			_imgFolder = out;
			Q_EMIT outFolderChanged(out.path());
			if (_mw == nullptr) {
				outstream << "Output folder set to " << out.path() << Qt::endl;
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

void CameraApplication::startRecording(int row) {

	if (row < 0 or row >= _lst->rowCount()) {
		if (_mw == nullptr) {
			QTextStream out(stdout);
			out << "Failed to start recording, non existing camera " << row << Qt::endl;
		}

		return;
	}

	if (_mw == nullptr) {
		QTextStream out(stdout);
		out << "Start recording with camera " << _lst->data(_lst->index(row)).toString() << Qt::endl;
	}

	std::string sn = _lst->serialNumber(row);

	rs2::config config;
	config.enable_device(sn);

	_img_grab = new CameraGrabber(this);
	_img_grab->setConfig(config);
	connect(_img_grab, &CameraGrabber::framesReady, this, &CameraApplication::receiveFrames, Qt::DirectConnection);
	connect(_img_grab, &CameraGrabber::acquisitionEndedWithError, this, &CameraApplication::manageAcquisitionError);

	_img_grab->start();
}
void CameraApplication::saveFrames(int nFrames) {

	if (nFrames > 0) {
		_saveAcessControl.lock();
		_imgsToSave += nFrames;
		_saveAcessControl.unlock();
	}
}
void CameraApplication::stopRecording() {

	if (_mw == nullptr) {
		QTextStream out(stdout);
		out << "Acquisition terminated !" << Qt::endl;
	}

	_img_grab->finish();
	disconnect(_img_grab, &CameraGrabber::framesReady, this, &CameraApplication::receiveFrames);
	disconnect(_img_grab, &CameraGrabber::acquisitionEndedWithError, this, &CameraApplication::manageAcquisitionError);
	_img_grab->wait();
	_img_grab->deleteLater();

	_img_grab = nullptr;
}

bool CameraApplication::isRecording() const {
	return _img_grab != nullptr;
}
bool CameraApplication::isRecordingToDisk() const {
	return isRecording() and _imgsToSave;
}

void CameraApplication::receiveFrames(ImageFrame frameLeft, ImageFrame frameRight) {

	if (_imgsToSave > 0) {
		QDateTime date = QDateTime::currentDateTimeUtc();
		QString timestamp =date.toString("yyyy_MM_dd_hh_mm_ss_zzz");
		QString leftFramePath = _imgFolder.filePath(timestamp + "_left.png");
		QString rightFramePath = _imgFolder.filePath(timestamp + "_right.png");

		frameLeft.save(leftFramePath);
		frameRight.save(rightFramePath);

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

		connect(_cw, &ConsoleWatcher::startRecordTriggered, this, &CameraApplication::startRecording);
		connect(_cw, &ConsoleWatcher::saveImgsTriggered, this, &CameraApplication::saveFrames);
		connect (_cw, &ConsoleWatcher::stopRecordTriggered, this, &CameraApplication::stopRecording);

		connect (_cw, &ConsoleWatcher::listCamerasTriggered, this, [this] () {
			QTextStream out(stdout);

			for (int i = 0; i < _lst->rowCount(); i++) {
				out << "Local cameras:" << Qt::endl;
				out << "\t" << i << ". " << _lst->data(_lst->index(i)).toString() << Qt::endl;
			}
		});

		_cw->run();
	}
}

void CameraApplication::configureApplicationServer() {

	if (_isServer) {
		_rs = new RemoteSyncServer(this);


		connect(_rs, &RemoteSyncServer::setSaveFolder, this, &CameraApplication::setExportDir);

		connect(_rs, &RemoteSyncServer::startRecording, this, &CameraApplication::startRecording);
		connect(_rs, &RemoteSyncServer::saveImagesRecording, this, &CameraApplication::saveFrames);
		connect (_rs, &RemoteSyncServer::stopRecording, this, &CameraApplication::stopRecording);

		_rs->listen(QHostAddress::Any, RemoteSyncServer::preferredPort);
	}

}

void CameraApplication::manageAcquisitionError(QString txt) {
	if (_mw != nullptr) {
		_mw->showErrorMessage(txt);
	} else {
		QTextStream err(stderr);
		err << txt << Qt::endl;
	}
}
