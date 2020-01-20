#pragma once

#include <libpng16/png.h>
#include <string>
#include <vector>

struct RGBCOLOR
{
	float R;
	float G;
	float B;

	RGBCOLOR();
	RGBCOLOR(float r, float g, float b);
};

class PngSaver //This class is needed to save PNG data to file
{
public:
	PngSaver();
	~PngSaver();

	PngSaver(const PngSaver&)           = delete;
	PngSaver operator=(const PngSaver&) = delete;

	PngSaver(const PngSaver&&)           = delete;
	PngSaver operator=(const PngSaver&&) = delete;

	void SavePngImage(const std::wstring& filename, size_t width, size_t height, size_t rowPitch, RGBCOLOR colorScheme, const std::vector<uint8_t>& grayscaleData);

	bool operator!() const;

private:
	png_structp mPngStruct;
	png_infop   mPngInfo;
};