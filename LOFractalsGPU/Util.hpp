#pragma once

#include <Windows.h>
#include <d3d11.h>
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

const std::wstring GetShaderPath();

template<typename CBufType>
inline void UpdateBuffer(ID3D11Buffer* destBuf, CBufType& srcBuf, ID3D11DeviceContext* dc)
{
	D3D11_MAPPED_SUBRESOURCE mappedbuffer;
	ThrowIfFailed(dc->Map(destBuf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedbuffer));

	CBufType* data = reinterpret_cast<CBufType*>(mappedbuffer.pData);

	memcpy(data, &srcBuf, sizeof(CBufType));

	dc->Unmap(destBuf, 0);
}