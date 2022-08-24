#include "mainwindow.h"

#include "cameraapplication.h"

int main(int argc, char *argv[])
{
	CameraApplication a(argc, argv);
	return a.exec();
}
