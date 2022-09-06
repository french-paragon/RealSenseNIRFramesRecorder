#include "v4l2camera.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <QDir>
#include <QTextStream>
#include <QDebug>

#define CLEAR(x) memset(&(x), 0, sizeof(x))

static int xioctl(int fd, int request, void *arg)
{
	int r;
	do r = ioctl (fd, request, arg);
	while (-1 == r && EINTR == errno);
	return r;
}

QVector<V4L2Camera::Descriptor> V4L2Camera::listAvailableCameras() {

	QDir dir("/sys/class/video4linux/");
	QString tmpl("/sys/class/video4linux/%1/%2");

	QStringList lst = dir.entryList();

	QVector<V4L2Camera::Descriptor> ret;
	ret.reserve(lst.size());

	for (QString const& entry : lst) {

		QString deviceDirTemplate = tmpl.arg(entry);

		QFile nameFile(deviceDirTemplate.arg("name"));
		bool ok = nameFile.open(QIODevice::ReadOnly);

		if (!ok) {
			continue;
		}

		QString name = QString::fromLocal8Bit(nameFile.readLine(255)).trimmed();

		nameFile.close();

		QFile indexFile(deviceDirTemplate.arg("index"));
		ok = indexFile.open(QIODevice::ReadOnly);

		if (!ok) {
			continue;
		}

		int index = indexFile.readAll().toInt(&ok);

		indexFile.close();

		if (!ok) {
			continue;
		}

		ret.push_back({name, index});
	}

	return ret;
}

V4L2Camera::V4L2Camera() :
	_descriptor({"Invalid",-1}),
	_file_descriptor(-1),
	_mode(Invalid),
	_n_buffers(-1),
	_isStarted(false)
{

	_imgShape = {0, 0, 0};
	_imgStride = {1, 1, 1};

	_colorSpace = "";
}

V4L2Camera::V4L2Camera(const Descriptor &descriptor) :
	_descriptor(descriptor),
	_mode(Invalid),
	_n_buffers(-1),
	_isStarted(false)
{


	_colorSpace = "";

	std::string devPath = QString("/dev/video%1").arg(_descriptor.index).toStdString();

	const char * cDevPath = devPath.c_str();

	struct stat st;

	if (-1 == stat(cDevPath, &st)) {
		_file_descriptor = -1;
		return;
	}

	if (!S_ISCHR(st.st_mode)) {
		_file_descriptor = -1;
		return;
	}

	_file_descriptor = open(cDevPath, O_RDWR);

	_imgShape = {0, 0, 0};
	_imgStride = {1, 1, 1};

	bool ok = init();

	if (!ok) {
		if (_file_descriptor >= 0) {
			close(_file_descriptor);
		}
		_file_descriptor = -1;
	}

}

V4L2Camera::~V4L2Camera() {
	if (_file_descriptor >= 0) {
		close(_file_descriptor);
	}


	if (_n_buffers > 0) {

		if (_mode == Stream) {
			deinit_streammode();
		} else if (_mode == Copy) {
			deinit_copymode();
		}

	}
}

bool V4L2Camera::start(Config const& config) {



	if (!isValid()) {
		return false;
	}

	if (!setConfig(config)) {
		return false;
	}

	if (_mode == Stream) {
		bool ok = start_streaming();

		if (!ok) {
			return false;
		}
	}

	_isStarted = true;
	return true;

}
bool V4L2Camera::treatNextFrame(std::function<void(Multidim::Array<uint8_t, 3>&)> const& callback) {

	if (!_isStarted) {
		return false;
	}

	for (;;) {
		fd_set filedescriptor_set;
		struct timeval tv;
		int status;

		FD_ZERO(&filedescriptor_set);
		FD_SET(_file_descriptor, &filedescriptor_set);

		/* Timeout. */
		tv.tv_sec = 2;
		tv.tv_usec = 0;

		status = select(_file_descriptor + 1, &filedescriptor_set, NULL, NULL, &tv);

		if (status == -1) {
			if (EINTR == errno) {
				continue;
			}
			return false;
		}

		if (status == 0) {
			return false;
		}

		if (read_frame(callback)) {
			return true;
		}

	}

	return true;
}

bool V4L2Camera::stop() {

	if (!isValid()) {
		return false;
	}

	if (_mode == Copy) {
		deinit_copymode();
	}

	if (_mode == Stream) {
		bool ok = stop_streaming();

		if (!ok) {
			return false;
		}
	}

	_isStarted = false;
	return true;
}

QString V4L2Camera::colorSpace() const {
	return _colorSpace;
}

bool V4L2Camera::init() {

	struct v4l2_capability cap;

	QTextStream err(stderr);

	if (-1 == xioctl(_file_descriptor, VIDIOC_QUERYCAP, &cap)) {
		if (EINVAL == errno) {
			err << _descriptor.index << " " << _descriptor.name << " is not a valid V4L2 device" << Qt::endl;
			return false;
		} else {
			return false;
		}
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		err << _descriptor.index << " " << _descriptor.name << " is not a V4L2 capture device" << Qt::endl;
		return false;
	}

	if (cap.capabilities & V4L2_CAP_STREAMING) {
		_mode = Stream;
	} else if (cap.capabilities & V4L2_CAP_READWRITE) {
		_mode = Copy;
	}

	return true;
}

bool V4L2Camera::setConfig(Config const& config) {

	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;
	struct v4l2_streamparm parm;

	/* Select video input, video standard and tune here. */

	CLEAR(cropcap);

	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (0 == xioctl(_file_descriptor, VIDIOC_CROPCAP, &cropcap)) {
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect; /* reset to default */

		if (-1 == xioctl(_file_descriptor, VIDIOC_S_CROP, &crop)) {
			switch (errno) {
			case EINVAL:
				/* Cropping not supported. */
				break;
			default:
				/* Errors ignored. */
				break;
			}
		}
	} else {
		/* Errors ignored. */
	}


	CLEAR(fmt);
	CLEAR(parm);

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	/* get original setting */
	if (-1 == xioctl(_file_descriptor, VIDIOC_G_FMT, &fmt)) {
		return false;
	}

	if (-1 == xioctl(_file_descriptor, VIDIOC_G_PARM, &parm)) {
		return false;
	}

	if (config.pixelFormat.size() == 4) { //4bytes code

		uint32_t formatCode = fmt.fmt.pix.pixelformat;
		std::memcpy(&formatCode, config.pixelFormat.data(), 4);

		fmt.fmt.pix.pixelformat = formatCode;
	}

	fmt.fmt.pix.width = config.frameSize.width;
	fmt.fmt.pix.height = config.frameSize.height;

	parm.parm.capture.timeperframe.numerator = config.fps.numerator;
	parm.parm.capture.timeperframe.denominator = config.fps.denominator;

	/* set configuration settings */
	if (-1 == xioctl(_file_descriptor, VIDIOC_S_FMT, &fmt)) {
		return false;
	}
	if (-1 == xioctl(_file_descriptor, VIDIOC_S_PARM, &parm)) {
		return false;
	}

	if (!set_viewArray(fmt.fmt.pix.height, fmt.fmt.pix.width, fmt.fmt.pix.colorspace, fmt.fmt.pix.pixelformat)) {
		return false;
	}

	switch (_mode) {
	case Copy:
		return init_copymode(fmt.fmt.pix.sizeimage);
	case Stream:
		return init_streammode();
	default:
		break;
	}

	return false;
}

bool V4L2Camera::init_copymode(int bufferSize) {

	_buffers = reinterpret_cast<imageBuffer*>(calloc(1, sizeof(imageBuffer)));

	if (!_buffers) {
		return false;
	}

	_buffers[0].length = bufferSize;
	_buffers[0].start = malloc(bufferSize);

	if (_buffers[0].start == nullptr) {
		return false;
	}

	return true;
}

void V4L2Camera::deinit_copymode() {

	free(_buffers[0].start);
	free(_buffers);
}

bool V4L2Camera::init_streammode() {

	struct v4l2_requestbuffers req;

	QTextStream err(stderr);

	CLEAR(req);

	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (-1 == xioctl(_file_descriptor, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			err << _descriptor.index << " " << _descriptor.name << " does not support memory mapping" << Qt::endl;
			return false;
		} else {
			return false;
		}
	}

	if (req.count < 2) {
		err << "Insufficient buffer memory on " << _descriptor.index << " " << _descriptor.name;
			exit(EXIT_FAILURE);
		}

	_buffers = reinterpret_cast<imageBuffer*>(calloc(req.count, sizeof(imageBuffer)));
	_n_buffers = req.count;

	if (!_buffers) {
		err << "Out of memory" << Qt::endl;
		return false;
	}

	for (uint32_t n_buffers = 0; n_buffers < req.count; ++n_buffers) {
		struct v4l2_buffer buf;

		CLEAR(buf);

		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory      = V4L2_MEMORY_MMAP;
		buf.index       = n_buffers;

		if (-1 == xioctl(_file_descriptor, VIDIOC_QUERYBUF, &buf)) {
			return false;
		}

		_buffers[n_buffers].length = buf.length;
		_buffers[n_buffers].start = mmap(NULL,
										buf.length,
										PROT_READ | PROT_WRITE,
										MAP_SHARED,
										_file_descriptor,
										buf.m.offset);

		if (MAP_FAILED == _buffers[n_buffers].start) {
			return false;
		}
	}

	return true;
}

void V4L2Camera::deinit_streammode() {

	if (_mode == Stream) {
		for (uint32_t i = 0; i < _n_buffers; i++) {
			munmap(_buffers[i].start, _buffers[i].length);
		}
	}

	free(_buffers);
	_n_buffers = 0;

}

bool V4L2Camera::set_viewArray(int height, int width, int colorSpaceCode, int colorFormat) {

	QTextStream err(stderr);

	if (colorSpaceCode == V4L2_COLORSPACE_JPEG) {
		err << "Colorspace is jpg, which is not supported" << Qt::endl;
	}

	int c;

	QByteArray colorFormatCode(reinterpret_cast<char*>(&colorFormat), 4);
	qDebug() << "configuring camera with colorspace" << colorFormatCode;

	_colorSpace = QString::fromLocal8Bit(colorFormatCode);

	if (colorFormat == V4L2_PIX_FMT_YUYV or colorFormat == V4L2_PIX_FMT_YVYU) {
		c = 2;
	} else if (colorFormat == V4L2_PIX_FMT_BGR24 or colorFormat == V4L2_PIX_FMT_RGB24 or colorFormat == V4L2_PIX_FMT_XBGR32) {
		c = 3;
	} else if (colorFormat == V4L2_PIX_FMT_ABGR32 or colorFormat == V4L2_PIX_FMT_ARGB32) {
		c = 4;
	} else {
		err << "Color format " << colorFormatCode << " is not supported" << Qt::endl;
		return false;
	}

	_imgShape = {height, width, c};
	_imgStride = {c*width, c, 1};

	return true;
}


bool V4L2Camera::start_streaming() {

	if (_mode != Stream) {
		return true;
	}

	unsigned int i;
	enum v4l2_buf_type type;

	for (i = 0; i < _n_buffers; ++i) {
		struct v4l2_buffer buf;

		CLEAR(buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;

		if (-1 == xioctl(_file_descriptor, VIDIOC_QBUF, &buf)) {
			return false;
		}

		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (-1 == xioctl(_file_descriptor, VIDIOC_STREAMON, &type)) {
			return false;
		}
	}

	return true;

}
bool V4L2Camera::stop_streaming() {

	if (_mode != Stream) {

		return true;
	}

	unsigned int i;
	enum v4l2_buf_type type;

	for (i = 0; i < _n_buffers; ++i) {

		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (-1 == xioctl(_file_descriptor, VIDIOC_STREAMOFF, &type)) {
			return false;
		}
	}

	deinit_streammode();

	return true;
}

bool V4L2Camera::read_frame(std::function<void(Multidim::Array<uint8_t, 3> &)> const& callback) {

	struct v4l2_buffer buf;

	switch (_mode) {
	case Copy:
	{
		if (-1 == read(_file_descriptor, _buffers[0].start, _buffers[0].length)) {
			return false;
		}

		Multidim::Array<uint8_t,3> img(reinterpret_cast<uint8_t*>(_buffers[0].start), _imgShape, _imgStride, false);
		callback(img);

		break;
	}
	case Stream:
	{
		CLEAR(buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;

		if (-1 == xioctl(_file_descriptor, VIDIOC_DQBUF, &buf)) { //dequeue buffer
			return false;
		}


		assert(buf.index < _n_buffers);

		Multidim::Array<uint8_t,3> img(reinterpret_cast<uint8_t*>(_buffers[buf.index].start), _imgShape, _imgStride, false);
		callback(img);

		if (-1 == xioctl(_file_descriptor, VIDIOC_QBUF, &buf)) { //queue buffer
			return false;
		}

		break;
	}
	default:
		return false;
	}

	return true;

}
