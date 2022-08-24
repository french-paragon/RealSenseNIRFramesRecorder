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
#include <QKeyEvent>
#include <QDebug>

#include "cameraslist.h"
#include "cameraapplication.h"

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

	_cam_lst = nullptr;

	connect(ui->actionStart_acquisition, &QAction::triggered, this, &MainWindow::onCameraLaunched);
	connect(ui->actionStop_camera, &QAction::triggered, this, &MainWindow::onCameraPaused);
	connect(ui->actionShot, &QAction::triggered, this, &MainWindow::onShot);

	ui->exportDirField->setText(CameraApplication::GetCameraApp()->exportDir());
	connect(ui->chooseDirButton, &QPushButton::clicked, this, &MainWindow::selectExportDir);

	setFocusPolicy(Qt::StrongFocus);

}

MainWindow::~MainWindow()
{
	delete ui;
	delete _pxmLeft;
	delete _pxmRight;
}

void MainWindow::setFrames(ImageFrame frameLeft, ImageFrame frameRight) {

	QPixmap pxLeft = imageFrameToQPixmap(frameLeft);
	QPixmap pxRight = imageFrameToQPixmap(frameRight);

	_pxmLeft->setPixmap(pxLeft);
	_pxmRight->setPixmap(pxRight);

}

void MainWindow::setCameraList(CamerasList* lst) {

	if (_cam_lst != nullptr) {
		disconnect(ui->refreshButton, &QPushButton::clicked, _cam_lst, &CamerasList::refreshCamerasList);
	}

	_cam_lst = lst;
	connect(ui->refreshButton, &QPushButton::clicked, lst, &CamerasList::refreshCamerasList);
	ui->camerasListView->setModel(lst);
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
	if (event->key() == Qt::Key_Space) {
		onShot();
	} else {
		QMainWindow::keyPressEvent(event);
	}
}

void MainWindow::onCameraLaunched() {

	ui->actionStart_acquisition->setEnabled(false);
	ui->actionStop_camera->setEnabled(true);
	ui->actionShot->setEnabled(true);

	int row = ui->camerasListView->currentIndex().row();

	CameraApplication::GetCameraApp()->startRecording(row);

}
void MainWindow::onCameraPaused() {
	endGrabber();
}
void MainWindow::onShot() {

	CameraApplication::GetCameraApp()->saveFrames(1);
}

void MainWindow::showErrorMessage(QString txt) {
	QMessageBox::critical(this, "An error happened during acquisition", txt);
	endGrabber();
}
void MainWindow::endGrabber() {

	ui->actionStart_acquisition->setEnabled(true);
	ui->actionStop_camera->setEnabled(false);
	ui->actionShot->setEnabled(false);

	CameraApplication::GetCameraApp()->stopRecording();

}

void MainWindow::selectExportDir() {
	QString dir = QFileDialog::getExistingDirectory(this, "Get image export directory", CameraApplication::GetCameraApp()->exportDir());

	if (!dir.isEmpty()) {
		if (QDir(dir).exists()) {
			CameraApplication::GetCameraApp()->setExportDir(dir);
			ui->exportDirField->setText(dir);
		}
	}
}

QPixmap imageFrameToQPixmap(const ImageFrame &f)
{

	int w = f.width();
	int h = f.height();

	if (f.imgType() == ImageFrame::GRAY_8) {
		QImage ret((uchar*) &f.grayscale8()->atUnchecked(0,0), w, h, w, QImage::Format_Grayscale8);
		return QPixmap::fromImage(ret);
	}

	if (f.imgType() == ImageFrame::GRAY_16) {
		Multidim::Array<uint16_t, 2>* original = f.grayscale16();
		Multidim::Array<uint8_t, 2> grayMap(original->shape(), original->strides());

		qint64 mean = 0;
		for (int i = 0; i < original->shape()[0]; i++) {
			for (int j = 0; j < original->shape()[1]; j++) {
				grayMap.atUnchecked(i,j) = static_cast<uint8_t>(original->valueUnchecked(i,j)/256);
				mean += grayMap.atUnchecked(i,j);
			}
		}

		QImage ret((uchar*) &grayMap.atUnchecked(0,0), w, h, w, QImage::Format_Grayscale8);
		QPixmap px = QPixmap::fromImage(ret.copy());
		return px.scaled(w/2,h/2);
	}

	if (f.imgType() == ImageFrame::RGBA_8) {

		if (f.rgba8()->shape()[2] == 3) { //RGB
			QImage ret((uchar*) &f.rgba8()->atUnchecked(0,0,0), w, h, w*3, QImage::Format_RGB888);
			return QPixmap::fromImage(ret);
		}

		if (f.rgba8()->shape()[2] == 4) { //RGBA
			QImage ret((uchar*) &f.rgba8()->atUnchecked(0,0,0), w, h, w*4, QImage::Format_RGBA8888);
			return QPixmap::fromImage(ret);
		}
	}

	throw std::runtime_error("Unsupported frame format !");
}
