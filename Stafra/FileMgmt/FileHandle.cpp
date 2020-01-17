#include "FileHandle.hpp"

FileHandle::FileHandle(const std::string& filename, const std::string& readMode) : mFile(nullptr)
{
	errno_t fileOpenErr = fopen_s(&mFile, filename.c_str(), readMode.c_str());
}

FileHandle::~FileHandle()
{
	if(mFile)
	{
		fclose(mFile);
	}
}

FILE* FileHandle::GetFilePointer() const
{
	return mFile;
}

bool FileHandle::operator!() const
{
	return mFile == nullptr;
}