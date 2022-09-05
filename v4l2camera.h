#ifndef V4L2CAMERA_H
#define V4L2CAMERA_H

#include <QString>
#include <QVector>
#include <string>

#include <MultidimArrays/MultidimArrays.h>

class V4L2Camera
{
public:

	struct Descriptor
	{
		QString name;
		int index;
	};

	static QVector<Descriptor> listAvailableCameras();

	V4L2Camera();
	V4L2Camera(Descriptor const& descriptor);
	~V4L2Camera();

	inline bool isValid() { return _file_descriptor >= 0; }

	bool start();
	bool treatNextFrame(std::function<void(Multidim::Array<uint8_t, 3> &)> const& callback);
	bool stop();

	QString colorSpace() const;
protected:

	bool init();
	bool init_copymode(int bufferSize);
	bool init_streammode();
	bool init_viewArray(int height, int width, int colorSpaceCode, int colorFormat);

	bool start_streaming();
	bool stop_streaming();

	bool read_frame(std::function<void(Multidim::Array<uint8_t, 3> &)> const& callback);

	enum TransfertMode {
		Unknown,
		Stream,
		Copy,
		Invalid
	};

	struct imageBuffer {
		void   *start;
		size_t  length;
	};

	struct imageBuffer *_buffers;

	Descriptor _descriptor;
	int _file_descriptor;

	TransfertMode _mode;
	unsigned int _n_buffers;

	bool _isStarted;
	QString _colorSpace;

	Multidim::Array<uint8_t, 3>::ShapeBlock _imgShape;
	Multidim::Array<uint8_t, 3>::ShapeBlock _imgStride;
};

#endif // V4L2CAMERA_H
