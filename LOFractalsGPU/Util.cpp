#include "Util.hpp"
#include <comdef.h>
#include <clocale>

DXException::DXException(HRESULT hr, const std::string& funcName, const std::string& filename, int32_t line)
{
	mErrorCode  = hr;
	mFuncName   = funcName;
	mFilename   = filename;
	mLineNumber = line;

	char locale[256];
	size_t cnt = 0;
	getenv_s(&cnt, locale, "LANG");

	setlocale(LC_ALL, locale);
}

std::string DXException::ToString() const
{
	_com_error err(mErrorCode);
	std::string msg = err.ErrorMessage();

	if(ToHR() == DXGI_ERROR_DEVICE_REMOVED)
	{
		msg = "DXGI_ERROR_DEVICE_REMOVED detected (probably killed by TDR mechanism). Your graphics card isn't good enough.";
	}

	return "\r\n\r\n\r\n" + mFuncName + " failed in " + mFilename + "; line " + std::to_string(mLineNumber) + "; error: " + msg + "\r\n\r\n\r\n";
}

HRESULT DXException::ToHR() const
{
	return mErrorCode;
}

const std::wstring GetShaderPath()
{
	wchar_t path[2048];
	GetModuleFileNameW(nullptr, path, 2048);

	wchar_t drive[512];
	wchar_t dir[1024];
	wchar_t filename[256];
	wchar_t ext[256];
	_wsplitpath_s(path, drive, dir, filename, ext);

#if defined(DEBUG) || defined(_DEBUG)
	return std::wstring(drive) + std::wstring(dir) + LR"(Shaders\Debug\)";
#else
	return std::wstring(drive) + std::wstring(dir) + LR"(Shaders\Release\)";
#endif
}
