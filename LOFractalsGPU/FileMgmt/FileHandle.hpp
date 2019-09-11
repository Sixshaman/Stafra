#pragma once

#include <string>
#include <cstdio>

class FileHandle
{
public:
	FileHandle(const std::string& filename, const std::string& readMode = "wb");
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