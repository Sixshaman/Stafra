#include "PNGSaver.hpp"
#include "FileHandle.hpp"

RGBCOLOR::RGBCOLOR(): R(1.0f), G(1.0f), B(1.0f)
{
}

RGBCOLOR::RGBCOLOR(float r, float g, float b): R(r), G(g), B(b)
{
}

PngSaver::PngSaver(): mPngStruct(nullptr), mPngInfo(nullptr)
{
	mPngStruct = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if(mPngStruct)
	{
		mPngInfo = png_create_info_struct(mPngStruct);
	}

	if(!mPngInfo || !mPngStruct || setjmp(png_jmpbuf(mPngStruct)) != 0)
	{
		png_destroy_write_struct(&mPngStruct, &mPngInfo);
	}
}

PngSaver::~PngSaver()
{
	png_destroy_write_struct(&mPngStruct, &mPngInfo);
}

void PngSaver::SavePngImage(const std::wstring& filename, size_t width, size_t height, size_t rowPitch, RGBCOLOR colorScheme, const std::vector<uint8_t>& grayscaleData)
{
	if(grayscaleData.size() < rowPitch * height)
	{
		return;
	}

	if(width > rowPitch)
	{
		return;
	}

	std::vector<std::vector<png_byte>> imgRows(height);

	std::vector<png_bytep> rowPointers(height);
	for (size_t i = 0; i < height; i++)
	{
		std::vector<png_byte> row((size_t)width * 4);
		for (size_t j = 0; j < width; j++)
		{
			row[j * 4 + 0] = (uint8_t)(grayscaleData[i * rowPitch + j] * colorScheme.R);
			row[j * 4 + 1] = (uint8_t)(grayscaleData[i * rowPitch + j] * colorScheme.G);
			row[j * 4 + 2] = (uint8_t)(grayscaleData[i * rowPitch + j] * colorScheme.B);
			row[j * 4 + 3] = 255;
		}

		imgRows[i]     = row;
		rowPointers[i] = &imgRows[i][0];
	}

	FileHandle fout(filename, L"wb");
	if(!fout)
	{
		return;
	}

	png_init_io(mPngStruct, fout.GetFilePointer());

	png_set_IHDR(mPngStruct, mPngInfo,
		         width, height,
		         8,
		         PNG_COLOR_TYPE_RGBA,
		         PNG_INTERLACE_NONE,
		         PNG_COMPRESSION_TYPE_DEFAULT,
		         PNG_FILTER_TYPE_DEFAULT);

	png_write_info(mPngStruct, mPngInfo);

	png_write_image(mPngStruct, rowPointers.data());
	png_write_end(mPngStruct, nullptr);
}

bool PngSaver::operator!() const
{
	return mPngStruct == nullptr || mPngInfo == nullptr;
}