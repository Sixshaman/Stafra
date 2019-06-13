#include "PNGSaver.hpp"

PngSaver::FileHandle::FileHandle(const std::string& filename): mFile(nullptr)
{
	fopen_s(&mFile, filename.c_str(), "wb");
}

PngSaver::FileHandle::~FileHandle()
{
	if(mFile) 
	{
		fclose(mFile);
	}
}

FILE* PngSaver::FileHandle::GetFilePointer() const
{
	return mFile;
}

bool PngSaver::FileHandle::operator!() const
{
	return mFile != nullptr;
}

//==================================================================================================================================================

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

void PngSaver::SavePngImage(const std::string& filename, size_t width, size_t height, size_t rowPitch, const std::vector<uint8_t>& grayscaleData)
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
			row[j * 4 + 0] = grayscaleData[i * rowPitch + j];
			row[j * 4 + 1] = 0;
			row[j * 4 + 2] = grayscaleData[i * rowPitch + j];
			row[j * 4 + 3] = 255;
		}

		imgRows[i]     = row;
		rowPointers[i] = &imgRows[i][0];
	}

	FileHandle fout(filename);
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
	return mPngStruct != nullptr && mPngInfo != nullptr;
}
