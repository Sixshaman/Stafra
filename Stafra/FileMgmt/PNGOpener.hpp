#pragma once

#include <libpng16/png.h>
#include <string>
#include <vector>

class PngOpener //This class is needed just to get PNG image size
{
public:
	PngOpener();
	~PngOpener();

	PngOpener(const PngOpener&) = delete;
	PngOpener operator=(const PngOpener&) = delete;

	PngOpener(const PngOpener&&) = delete;
	PngOpener operator=(const PngOpener&&) = delete;

	bool GetImageSize(const std::string& filename, size_t& width, size_t& height);

	bool operator!() const;

private:
	png_structp mPngStruct;
	png_infop   mPngInfo;
};