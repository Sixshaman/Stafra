#pragma once
#include <Windows.h>
#include <string>

class DXException
{
public:
	DXException(HRESULT hr, const std::string& funcName, const std::string& filename, int32_t line);
	std::string ToString() const;

	HRESULT ToHR() const;

private:
	HRESULT      mErrorCode;
	std::string  mFuncName;
	std::string  mFilename;
	int32_t      mLineNumber;
};

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                 \
{                                                        \
	HRESULT __hr = (x);                                  \
	if(FAILED(__hr))                                     \
	{                                                    \
		throw DXException(__hr, #x, __FILE__, __LINE__); \
	}                                                    \
}
#endif