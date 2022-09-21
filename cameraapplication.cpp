#include "cameraapplication.h"

#include "cameraslist.h"
#include "cameragrabber.h"
#include "mainwindow.h"
#include "consolewatcher.h"
#include "remotesyncserver.h"
#include "remotesyncclient.h"
#include "remoteconnectionlist.h"
#include "v4l2camera.h"

#include <QApplication>
#include <QDateTime>
#include <QStandardPaths>
#include <QTextStream>
#include <QTimer>
#include <QTemporaryDir>

#include <QDebug>

#include <vlc/vlc.h>

#include "LibStevi/imageProcessing/colorConversions.h"

CameraApplication* CameraApplication::CurrentApp = nullptr;

CameraApplication* CameraApplication::GetCameraApp() {
	return CurrentApp;
}

CameraApplication::CameraApplication(int &argc, char **argv) :
	QObject(nullptr),
	_sessionTimingFile(nullptr),
	_rs(nullptr)
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
	_pingTimer = new QTimer(this);

	_imgsToSave = 0;

	_QtApp = getAppPointer(argc, argv);
	CurrentApp = this;

	_mw = nullptr;

	connect(this, &CameraApplication::triggerStopRecording, this, &CameraApplication::stopRecording, Qt::QueuedConnection);

	_vlc = libvlc_new (0, NULL);

	QString filePath = _tmp_dir.filePath("ding.wav");

	QFile::copy(":/resources/ding.wav", filePath);

	libvlc_media_t *m;
	m = libvlc_media_new_path(_vlc, filePath.toStdString().c_str());

	_media_player = libvlc_media_player_new_from_media (m);

	libvlc_media_release (m);

}

CameraApplication::~CameraApplication() {

	libvlc_media_player_release (_media_player);
	libvlc_release (_vlc);

	if (_mw != nullptr) {
		delete _mw;
	}

	if (_rs != nullptr) {
		delete _rs;
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

	if (_sessionTimingFile != nullptr) {
		_sessionTimingFile->close();
		_sessionTimingFile = nullptr;
	}

	QDateTime date = QDateTime::currentDateTimeUtc();
	QString timestamp =date.toString("yyyy_MM_dd_hh_mm_ss_zzz");

	QString sessionTimingFileName = _imgFolder.filePath("session_" + timestamp + "_timing_infos.csv");

	_sessionTimingFile = new QFile(sessionTimingFileName);

	if (!_sessionTimingFile->open(QIODevice::WriteOnly)) {
		qDebug() << "Failed to create file" << sessionTimingFileName;
		delete _sessionTimingFile;
		_sessionTimingFile = nullptr;
	} else {
		QTextStream fStream(_sessionTimingFile);
		fStream << "peerName" << " [ peer address ]," << "query_time_ms" << ',' << "server_time_ms" << ',' << "answer_time_ms" << endl;
	}

	connect(_pingTimer, &QTimer::timeout, this, &CameraApplication::pingAll);
	_pingTimer->start(10000);

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

		V4L2Camera::Descriptor descr = {"v4l2", _lst->v4l2DeviceId(row)};
		V4L2Camera::Config config;

		_img_grab->setV4l2descr(descr);
		_img_grab->setV4L2Config(config);
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

void CameraApplication::saveInterval(int nFrames, int msec) {

	if (nFrames <= 0) {
		return;
	}

	if (msec < 100) {
		saveFrames(nFrames);
		return;
	}

	libvlc_media_player_stop (_media_player);
	libvlc_media_player_play (_media_player);

	for (int i = 0; i < nFrames; i++) {

		QThread::currentThread()->msleep(msec);

		saveFrames(1);

		libvlc_media_player_stop (_media_player);
		libvlc_media_player_play (_media_player);
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

	if (_sessionTimingFile != nullptr) {
		_sessionTimingFile->close();
		_sessionTimingFile = nullptr;
	}

	disconnect(_pingTimer, &QTimer::timeout, this, &CameraApplication::pingAll);
	_pingTimer->stop();

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

void CameraApplication::exportRecording() {

	exportRecorded();

	for (int i = 0; i < _remoteConnections->rowCount(); i++) {
		_remoteConnections->getConnectionAtRow(i)->triggerExport();
	}

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

				bool ok = false;
				QString outPath = _imgFolder.filePath(info.baseName() + ".png");

				if (frame.additionalInfos().contains(ImageFrame::colorSpaceKey)) {
					QString colorSpace = frame.additionalInfos()[ImageFrame::colorSpaceKey];

					if (colorSpace == "YUYV") {
						Multidim::Array<uint8_t,3> rgb = StereoVision::ImageProcessing::yuyv2rgb<uint8_t,3>(*frame.multichannels8());
						ImageFrame converted(&rgb.atUnchecked(0,0,0), rgb.shape(), rgb.strides(), false);
						ok = converted.save(outPath);
					} else if (colorSpace == "YVYU") {
						Multidim::Array<uint8_t,3> rgb = StereoVision::ImageProcessing::yvyu2rgb<uint8_t,3>(*frame.multichannels8());
						ImageFrame converted(&rgb.atUnchecked(0,0,0), rgb.shape(), rgb.strides(), false);
						ok = converted.save(outPath);
					} else if (colorSpace == "YUV") {
						Multidim::Array<uint8_t,3> rgb = StereoVision::ImageProcessing::yuv2rgb<uint8_t,3>(*frame.multichannels8());
						ImageFrame converted(&rgb.atUnchecked(0,0,0), rgb.shape(), rgb.strides(), false);
						ok = converted.save(outPath);
					}
				}

				if (!ok) {
					ok = frame.save(outPath);
				}

				if (ok) {
					out << "Exported " << file << endl;
					QFile f(_imgFolder.filePath(file));
					f.remove();
					QFile fi(_imgFolder.filePath(file+".infos"));
					fi.remove();
				} else {
					out << "Could not export " << file << endl;
				}
			}
		}
	}
	out << "Exports done !" << endl;
}
void CameraApplication::setInfraRedPatternOnSession(bool on) {
	setInfraRedPatternOn(on);
}

void CameraApplication::setInfraRedPatternOn(bool on) {

	if (_img_grab != nullptr) {
		_img_grab->setInfraRedPatternOn(on);
	}

	for (int i = 0; i < _remoteConnections->rowCount(); i++) {
		_remoteConnections->getConnectionAtRow(i)->setInfraRedPatternOn(on);
	}
}

void CameraApplication::connectToRemote(QString host) {

	RemoteSyncClient* remote = new RemoteSyncClient(this);
	bool ok = remote->connectToHost(host, RemoteSyncServer::preferredPort);

	if (ok) {
		connect(remote, &RemoteSyncClient::timingInfoReceived,
				this, &CameraApplication::timingInfoReceived);
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

void CameraApplication::pingAll() {
	for (int i = 0; i < _remoteConnections->rowCount(); i++) {
		RemoteSyncClient* connection = _remoteConnections->getConnectionAtRow(i);
		connection->checkConnectionTime();
	}
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
		connect(_cw, &ConsoleWatcher::saveImgsIntervalTriggered, this, &CameraApplication::saveInterval);
		connect (_cw, &ConsoleWatcher::stopRecordTriggered, this, &CameraApplication::stopRecordSession);
		connect (_cw, &ConsoleWatcher::exportRecordTriggered, this, &CameraApplication::exportRecording);
		connect (_cw, &ConsoleWatcher::setIrPatternTriggered, this, &CameraApplication::setInfraRedPatternOnSession);

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
		_serverThread = new QThread(this);
		_serverThread->start();

		_rs = new RemoteSyncServer(nullptr);
		_rs->moveToThread(_serverThread);

		connect(_rs, &RemoteSyncServer::setSaveFolder, this, &CameraApplication::setExportDir, Qt::QueuedConnection);

		connect(_rs, &RemoteSyncServer::startRecording, this, &CameraApplication::startRecording, Qt::QueuedConnection);
		connect(_rs, &RemoteSyncServer::saveImagesRecording, this, &CameraApplication::saveLocalFrames, Qt::QueuedConnection);
		connect (_rs, &RemoteSyncServer::stopRecording, this, &CameraApplication::stopRecording, Qt::QueuedConnection);
		connect (_rs, &RemoteSyncServer::setInfraRedPatternOn, this, &CameraApplication::setInfraRedPatternOn, Qt::QueuedConnection);
		connect (_rs, &RemoteSyncServer::exportRecorded, this, &CameraApplication::exportRecorded, Qt::QueuedConnection);

		connect(this, &CameraApplication::serverAboutToStart, _rs, [this] () {

		_rs->listen(QHostAddress::Any, RemoteSyncServer::preferredPort);

		QTextStream out(stdout);
		out << "RealSense NIR Frame recorder - server mode" << "\n";
		out << QDateTime::currentDateTime().toString() << "\n";
		out << "Started listening on port " << _rs->serverPort() << endl;
		},
		Qt::QueuedConnection);

		Q_EMIT serverAboutToStart();
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

void CameraApplication::timingInfoReceived(QString peerName,
						QString peerAddr,
						qint64 sent_ms,
						qint64 server_ms,
						qint64 now_ms) {
	if (_sessionTimingFile != nullptr) {
		QTextStream fStream(_sessionTimingFile);
		fStream << peerName << " [" << peerAddr << "], " << sent_ms << ',' << server_ms << ',' << now_ms << endl;
	} else {


		QTextStream out(stdout);

		out << "Measure connection time answer received from "
			<< peerName
			<< " [" << peerAddr << "]"
			<< "!\n\t";

		out << "Submission time [ms] :" << sent_ms << "\n\t";
		out << "Reception time [ms] :" << now_ms << "\n\t";
		out << "time delta [ms] :" << (now_ms - sent_ms) << "\n\t";
		out << "Server time [ms] :" << server_ms << endl;
	}
}
