#include "PNGOpener.hpp"
#include "FileHandle.hpp"

PngOpener::PngOpener(): mPngStruct(nullptr), mPngInfo(nullptr)
{
	mPngStruct = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (mPngStruct)
	{
		mPngInfo = png_create_info_struct(mPngStruct);
	}

	if(!mPngInfo || !mPngStruct || setjmp(png_jmpbuf(mPngStruct)) != 0)
	{
		png_destroy_read_struct(&mPngStruct, &mPngInfo, nullptr);
	}
}

PngOpener::~PngOpener()
{
	png_destroy_read_struct(&mPngStruct, &mPngInfo, nullptr);
}

bool PngOpener::GetImageSize(const std::wstring& filename, size_t& width, size_t& height)
{
	FileHandle fin(filename, L"rb");
	if (!fin)
	{
		width  = 0;
		height = 0;
		return false;
	}

	png_init_io(mPngStruct, fin.GetFilePointer());

	png_read_info(mPngStruct, mPngInfo);

	png_uint_32 pngWidth       = 0;
	png_uint_32 pngHeight      = 0;
	int         pngBitDepth    = 0;
	int         pngColorType   = 0;
	int         pngInterlace   = 0;
	int         pngCompression = 0;
	int         pngFilter      = 0;
	if(!png_get_IHDR(mPngStruct, mPngInfo, &pngWidth, &pngHeight, &pngBitDepth, &pngColorType, &pngInterlace, &pngCompression, &pngFilter))
	{
		width  = 0;
		height = 0;
		return false;
	}

	width  = pngWidth;
	height = pngHeight;
	return true;
}

bool PngOpener::operator!() const
{
	return mPngStruct == nullptr || mPngInfo == nullptr;
}
