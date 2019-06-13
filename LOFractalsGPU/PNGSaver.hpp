#pragma once

#include <cstdio>
#include <png.h>
#include <string>
#include <vector>

class PngSaver
{
	class FileHandle
	{
	public:
		FileHandle(const std::string& filename);
		~FileHandle();
	
		FileHandle(const FileHandle&)           = delete;
		FileHandle operator=(const FileHandle&) = delete;

		FileHandle(const FileHandle&&)           = delete;
		FileHandle operator=(const FileHandle&&) = delete;

		FILE* GetFilePointer() const;
		bool operator!()       const;

	private:
		FILE* mFile;
	};

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