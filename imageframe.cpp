#include "imageframe.h"

#include "LibStevi/io/image_io.h"

ImageFrame::ImageFrame() :
	_type(INVALID),
	_grayscale8(),
	_grayscale16(),
	_grayscalef32(),
	_rgba8()
{

}
ImageFrame::ImageFrame(uint8_t *data, Multidim::Array<uint8_t, 2>::ShapeBlock shape, Multidim::Array<uint8_t, 2>::ShapeBlock stride, bool copy) :
	_type(GRAY_8),
	_grayscale8(),
	_grayscale16(),
	_grayscalef32(),
	_rgba8()
{
	_grayscale8 = std::make_shared<Multidim::Array<uint8_t, 2>>(data, shape, stride, copy);
}
ImageFrame::ImageFrame(uint16_t* data, Multidim::Array<uint16_t, 2>::ShapeBlock shape, Multidim::Array<uint16_t, 2>::ShapeBlock stride, bool copy) :
	_type(GRAY_16),
	_grayscale8(),
	_grayscale16(),
	_grayscalef32(),
	_rgba8()
{
	_grayscale16 = std::make_shared<Multidim::Array<uint16_t, 2>>(data, shape, stride, copy);
}
ImageFrame::ImageFrame(float* data, Multidim::Array<float, 2>::ShapeBlock shape, Multidim::Array<float, 2>::ShapeBlock stride, bool copy) :
	_type(GRAY_F32),
	_grayscale8(),
	_grayscale16(),
	_grayscalef32(),
	_rgba8()
{
	_grayscalef32 = std::make_shared<Multidim::Array<float, 2>>(data, shape, stride, copy);
}
ImageFrame::ImageFrame(uint8_t* data, Multidim::Array<uint8_t, 3>::ShapeBlock shape, Multidim::Array<uint8_t, 3>::ShapeBlock stride, bool copy) :
	_type(RGBA_8),
	_grayscale8(),
	_grayscale16(),
	_grayscalef32(),
	_rgba8()
{
	_rgba8 = std::make_shared<Multidim::Array<uint8_t, 3>>(data, shape, stride, copy);
}

ImageFrame::ImageFrame(QString const& fileName) :
	_type(INVALID),
	_grayscale8(),
	_grayscale16(),
	_grayscalef32(),
	_rgba8()
{

	if (fileName.endsWith(".stevimg")) {

		if (StereoVision::IO::stevImgFileMatchTypeAndDim<uint8_t, 2>(fileName.toStdString())) {
			Multidim::Array<uint8_t, 2> img = StereoVision::IO::readStevimg<uint8_t, 2>(fileName.toStdString());
			if (!img.empty()) {
				_grayscale8 = std::make_shared<Multidim::Array<uint8_t, 2>>();
				*_grayscale8 = std::move(img);
				_type = GRAY_8;
			}
		}

		if (StereoVision::IO::stevImgFileMatchTypeAndDim<uint16_t, 2>(fileName.toStdString())) {
			Multidim::Array<uint16_t, 2> img = StereoVision::IO::readStevimg<uint16_t, 2>(fileName.toStdString());
			if (!img.empty()) {
				_grayscale16 = std::make_shared<Multidim::Array<uint16_t, 2>>();
				*_grayscale16 = std::move(img);
				_type = GRAY_16;
			}
        }

        if (StereoVision::IO::stevImgFileMatchTypeAndDim<float, 2>(fileName.toStdString())) {
            Multidim::Array<float, 2> img = StereoVision::IO::readStevimg<float, 2>(fileName.toStdString());
            if (!img.empty()) {
                _grayscalef32 = std::make_shared<Multidim::Array<float, 2>>();
                *_grayscalef32 = std::move(img);
                _type = GRAY_F32;
            }
        }

        if (StereoVision::IO::stevImgFileMatchTypeAndDim<uint8_t, 3>(fileName.toStdString())) {
            Multidim::Array<uint8_t, 3> img = StereoVision::IO::readStevimg<uint8_t, 3>(fileName.toStdString());
            if (!img.empty()) {
                _rgba8 = std::make_shared<Multidim::Array<uint8_t, 3>>();
                *_rgba8 = std::move(img);
                _type = RGBA_8;
            }
        }

	}

	if (_type == INVALID) {
		Multidim::Array<uint8_t, 3> img = StereoVision::IO::readImage<uint8_t>(fileName.toStdString());
		if (!img.empty() and img.shape()[2] == 3) {
			_rgba8 = std::make_shared<Multidim::Array<uint8_t, 3>>();
			*_rgba8 = std::move(img);
			_type = RGBA_8;
		}
	}

}

ImageFrame::ImageFrame(ImageFrame const& other) :
	_type(other._type),
	_grayscale8(other._grayscale8),
	_grayscale16(other._grayscale16),
	_grayscalef32(other._grayscalef32),
	_rgba8(other._rgba8)
{

}


ImageFrame::~ImageFrame() {

} 

bool ImageFrame::save(QString const& filePath) const {

	if (_type == GRAY_8) {
		return StereoVision::IO::writeImage<uint8_t, uint8_t>(filePath.toStdString(), _grayscale8->subView(Multidim::DimSlice(), Multidim::DimSlice()));
	}

	if (_type == GRAY_16) {
		return StereoVision::IO::writeImage<uint16_t, uint16_t>(filePath.toStdString(), _grayscale16->subView(Multidim::DimSlice(), Multidim::DimSlice()));
	}

	if (_type == GRAY_F32) {
		return StereoVision::IO::writeImage<float, float>(filePath.toStdString(), _grayscalef32->subView(Multidim::DimSlice(), Multidim::DimSlice()));
	}

	if (_type == RGBA_8) {
		return StereoVision::IO::writeImage<uint8_t, uint8_t>(filePath.toStdString(), _rgba8->subView(Multidim::DimSlice(), Multidim::DimSlice(), Multidim::DimSlice()));
	}

	return false;
}
