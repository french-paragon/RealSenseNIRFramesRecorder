#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "cameraslist.h"
#include "cameragrabber.h"

#include <QDir>

class QGraphicsScene;
class QGraphicsPixmapItem;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = nullptr);
	~MainWindow();

private:

protected:

	void onCameraLaunched();
	void onCameraPaused();
	void onShot();

	void showAcquisitionError(QString txt);
	void endGrabber();

	void receiveFrames(QImage frameLeft, QImage frameRight);

	void selectExportDir();

	Ui::MainWindow *ui;

	CamerasList* _lst;

	CameraGrabber* _img_grab;

	QGraphicsScene* _sceneImgLeft;
	QGraphicsPixmapItem* _pxmLeft;

	QGraphicsScene* _sceneImgRight;
	QGraphicsPixmapItem* _pxmRight;

	QDir _imgFolder;
	QMutex _saveAcessControl;
	bool _saveImgs;

};
#endif // MAINWINDOW_H
