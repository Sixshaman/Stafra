#include "Util.hpp"
#include <comdef.h>

DXException::DXException(HRESULT hr, const std::string& funcName, const std::string& filename, int32_t line)
{
	mErrorCode  = hr;
	mFuncName   = funcName;
	mFilename   = filename;
	mLineNumber = line;
}

std::string DXException::ToString() const
{
	_com_error err(mErrorCode);
	std::string msg = err.ErrorMessage();

	auto msgName = err.Description();

	return mFuncName + " failed in " + mFilename + "; line " + std::to_string(mLineNumber) + "; error: " + msg;
}

HRESULT DXException::ToHR() const
{
	return mErrorCode;
}

const std::wstring GetShaderPath()
{
#if defined(DEBUG) || defined(_DEBUG)
	return LR"(Shaders\Debug\)";
#else
	return LR"(Shaders\Release\)";
#endif
}
