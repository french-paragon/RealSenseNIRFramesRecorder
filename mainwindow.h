#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QDir>

#include "./imageframe.h"

class QGraphicsScene;
class QGraphicsPixmapItem;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class CamerasList;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = nullptr);
	~MainWindow();

	void setFrames(ImageFrame frameLeft, ImageFrame frameRight);

	void setCameraList(CamerasList* lst);

	void showErrorMessage(QString txt);

private:

protected:

	void keyPressEvent(QKeyEvent *event) override;

	void onCameraLaunched();
	void onCameraPaused();
	void onShot();

	void endGrabber();

	void selectExportDir();

	Ui::MainWindow *ui;

	QGraphicsScene* _sceneImgLeft;
	QGraphicsPixmapItem* _pxmLeft;

	QGraphicsScene* _sceneImgRight;
	QGraphicsPixmapItem* _pxmRight;

	CamerasList* _cam_lst;

};

QPixmap imageFrameToQPixmap(const ImageFrame &f);

#endif // MAINWINDOW_H
