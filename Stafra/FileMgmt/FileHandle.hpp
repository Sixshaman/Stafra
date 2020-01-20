#pragma once

#include <string>
#include <cstdio>

class FileHandle
{
public:
	FileHandle(const std::wstring& filename, const std::wstring& readMode = L"wb");
	~FileHandle();

	FileHandle(const FileHandle&) = delete;
	FileHandle operator=(const FileHandle&) = delete;

	FileHandle(const FileHandle&&) = delete;
	FileHandle operator=(const FileHandle&&) = delete;

	FILE* GetFilePointer() const;
	bool operator!()       const;

private:
	FILE* mFile;
};