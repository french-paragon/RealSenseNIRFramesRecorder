#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QThread>
#include <QPushButton>
#include <QMessageBox>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QStandardPaths>
#include <QFileDialog>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	ui->actionStart_acquisition->setEnabled(true);
	ui->actionStop_camera->setEnabled(false);
	ui->actionShot->setEnabled(false);

	_sceneImgLeft = new QGraphicsScene(this);
	_sceneImgRight = new QGraphicsScene(this);

	_pxmLeft = new QGraphicsPixmapItem();
	_pxmRight = new QGraphicsPixmapItem();

	_sceneImgLeft->addItem(_pxmLeft);
	_sceneImgRight->addItem(_pxmRight);

	ui->imgLeftView->setScene(_sceneImgLeft);
	ui->imgRightView->setScene(_sceneImgRight);

	_lst = new CamerasList(this);
	_lst->refreshCamerasList();
	ui->camerasListView->setModel(_lst);

	connect(ui->refreshButton, &QPushButton::clicked, _lst, &CamerasList::refreshCamerasList);
	connect(ui->actionStart_acquisition, &QAction::triggered, this, &MainWindow::onCameraLaunched);
	connect(ui->actionStop_camera, &QAction::triggered, this, &MainWindow::onCameraPaused);
	connect(ui->actionShot, &QAction::triggered, this, &MainWindow::onShot);

	_imgFolder.setPath(QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).first());
	ui->exportDirField->setText(_imgFolder.path());
	connect(ui->chooseDirButton, &QPushButton::clicked, this, &MainWindow::selectExportDir);

}

MainWindow::~MainWindow()
{
	delete ui;
	delete _pxmLeft;
	delete _pxmRight;
}

void MainWindow::receiveFrames(QImage frameLeft, QImage frameRight) {

	if (_saveImgs) {
		QString timestamp = QDateTime::currentDateTime().toString("yyyy_MM_hh_mm_ss_zz");
		QString leftFramePath = _imgFolder.filePath(timestamp + "_left.png");
		QString rightFramePath = _imgFolder.filePath(timestamp + "_right.png");

		frameLeft.save(leftFramePath);
		frameRight.save(rightFramePath);

		_pxmLeft->setPixmap(QPixmap());
		_pxmRight->setPixmap(QPixmap());

		_saveAcessControl.lock();
		_saveImgs = false;
		_saveAcessControl.unlock();
		ui->actionShot->setEnabled(true);

	} else {
		_pxmLeft->setPixmap(QPixmap::fromImage(frameLeft));
		_pxmRight->setPixmap(QPixmap::fromImage(frameRight));
	}

}

void MainWindow::onCameraLaunched() {

	ui->actionStart_acquisition->setEnabled(false);
	ui->actionStop_camera->setEnabled(true);
	ui->actionShot->setEnabled(true);

	int row = ui->camerasListView->currentIndex().row();

	std::string sn = _lst->serialNumber(row);

	rs2::config config;
	config.enable_device(sn);

	_img_grab = new CameraGrabber(this);
	_img_grab->setConfig(config);
	connect(_img_grab, &CameraGrabber::framesReady, this, &MainWindow::receiveFrames);
	connect(_img_grab, &CameraGrabber::acquisitionEndedWithError, this, &MainWindow::showAcquisitionError);

	_img_grab->start();

}
void MainWindow::onCameraPaused() {
	endGrabber();
}
void MainWindow::onShot() {
	_saveAcessControl.lock();
	_saveImgs = true;
	_saveAcessControl.unlock();
	ui->actionShot->setEnabled(false);
}

void MainWindow::showAcquisitionError(QString txt) {
	QMessageBox::critical(this, "An error happened during acquisition", txt);
	endGrabber();
}
void MainWindow::endGrabber() {

	ui->actionStart_acquisition->setEnabled(true);
	ui->actionStop_camera->setEnabled(false);
	ui->actionShot->setEnabled(false);

	_img_grab->finish();
	_img_grab->wait();
	disconnect(_img_grab, &CameraGrabber::framesReady, this, &MainWindow::receiveFrames);
	disconnect(_img_grab, &CameraGrabber::acquisitionEndedWithError, this, &MainWindow::showAcquisitionError);
	_img_grab->deleteLater();

	_img_grab = nullptr;

}

void MainWindow::selectExportDir() {
	QString dir = QFileDialog::getExistingDirectory(this, "Get image export directory", _imgFolder.path());

	if (!dir.isEmpty()) {
		if (QDir(dir).exists()) {
			_imgFolder.setPath(dir);
			ui->exportDirField->setText(dir);
		}
	}
}
