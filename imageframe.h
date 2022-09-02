#ifndef IMAGEFRAME_H
#define IMAGEFRAME_H

#include <MultidimArrays/MultidimArrays.h>
#include <memory>

#include <QString>

class ImageFrame
{
public:
	enum ImgType {
		GRAY_8,
		GRAY_16,
		GRAY_F32,
		RGBA_8,
		INVALID
	};

	ImageFrame();
	ImageFrame(uint8_t* data, Multidim::Array<uint8_t, 2>::ShapeBlock shape, Multidim::Array<uint8_t, 2>::ShapeBlock stride, bool copy = true);
	ImageFrame(uint16_t* data, Multidim::Array<uint16_t, 2>::ShapeBlock shape, Multidim::Array<uint16_t, 2>::ShapeBlock stride, bool copy = true);
	ImageFrame(float* data, Multidim::Array<float, 2>::ShapeBlock shape, Multidim::Array<float, 2>::ShapeBlock stride, bool copy = true);
	ImageFrame(uint8_t* data, Multidim::Array<uint8_t, 3>::ShapeBlock shape, Multidim::Array<uint8_t, 3>::ShapeBlock stride, bool copy = true);

	ImageFrame(const QString &fileName);

	ImageFrame(ImageFrame const& other);

	~ImageFrame();

	inline ImgType imgType() const { return _type; }
	inline bool isValid() const { return _type != INVALID; }

	inline int height() const {
		switch (_type) {
		case GRAY_8:
			return _grayscale8->shape()[0];
		case GRAY_16:
			return _grayscale16->shape()[0];
		case GRAY_F32:
			return _grayscalef32->shape()[0];
		case RGBA_8:
			return _rgba8->shape()[0];
		default:
			return 0;
		}
	}

	inline int width() const {
		switch (_type) {
		case GRAY_8:
			return _grayscale8->shape()[1];
		case GRAY_16:
			return _grayscale16->shape()[1];
		case GRAY_F32:
			return _grayscalef32->shape()[1];
		case RGBA_8:
			return _rgba8->shape()[1];
		default:
			return 0;
		}
	}

	inline int channels() const {
		switch (_type) {
		case RGBA_8:
			return _rgba8->shape()[2];
		default:
			return 1;
		}
	}

	inline Multidim::Array<uint8_t, 2>* grayscale8() const {
		if (_type == GRAY_8) {
			return _grayscale8.get();
		}
		return nullptr;
	}

	inline Multidim::Array<uint16_t, 2>* grayscale16() const {
		if (_type == GRAY_16) {
			return _grayscale16.get();
		}
		return nullptr;
	}

	inline Multidim::Array<float, 2>* grayscalef32() const {
		if (_type == GRAY_F32) {
			return _grayscalef32.get();
		}
		return nullptr;
	}

	inline Multidim::Array<uint8_t, 3>* rgba8() const {
		if (_type == RGBA_8) {
			return _rgba8.get();
		}
		return nullptr;
	}

	bool save(QString const& filePath) const;


protected:

	ImgType _type;

	std::shared_ptr<Multidim::Array<uint8_t, 2>> _grayscale8;
	std::shared_ptr<Multidim::Array<uint16_t, 2>> _grayscale16;
	std::shared_ptr<Multidim::Array<float, 2>> _grayscalef32;
	std::shared_ptr<Multidim::Array<uint8_t, 3>> _rgba8;

};

#endif // IMAGEFRAME_H
