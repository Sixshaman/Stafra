#pragma once

#include <png.h>
#include <string>
#include <vector>

class PngSaver //This class is needed to save PNG data to file
{
public:
	PngSaver();
	~PngSaver();

	PngSaver(const PngSaver&)           = delete;
	PngSaver operator=(const PngSaver&) = delete;

	PngSaver(const PngSaver&&)           = delete;
	PngSaver operator=(const PngSaver&&) = delete;

	void SavePngImage(const std::string& filename, size_t width, size_t height, size_t rowPitch, const std::vector<uint8_t>& grayscaleData);

	bool operator!() const;

private:
	png_structp mPngStruct;
	png_infop   mPngInfo;
};