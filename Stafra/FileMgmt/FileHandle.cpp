#include "FileHandle.hpp"

FileHandle::FileHandle(const std::wstring& filename, const std::wstring& readMode): mFile(nullptr)
{
	errno_t fileOpenErr = _wfopen_s(&mFile, filename.c_str(), readMode.c_str());
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