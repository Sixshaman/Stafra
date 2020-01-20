#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <string>

namespace Utils
{
	class DXException
	{
	public:
		DXException(HRESULT hr, const std::wstring& funcName, const std::wstring& filename, int32_t line);
		std::wstring ToString() const;

		HRESULT ToHR() const;

	private:
		HRESULT       mErrorCode;
		std::wstring  mFuncName;
		std::wstring  mFilename;
		int32_t       mLineNumber;
	};

	const std::wstring GetShaderPath();

	HRESULT LoadShaderFromFile(ID3D11Device* device, const std::wstring& path, ID3D11ComputeShader** shader);
	HRESULT LoadShaderFromFile(ID3D11Device* device, const std::wstring& path, ID3D11VertexShader**  shader);
	HRESULT LoadShaderFromFile(ID3D11Device* device, const std::wstring& path, ID3D11PixelShader**   shader);
}

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                          \
{                                                                 \
	HRESULT __hr = (x);                                           \
	if(FAILED(__hr))                                              \
	{                                                             \
		throw Utils::DXException(__hr, L#x, __FILEW__, __LINE__); \
	}                                                             \
}
#endif

namespace Utils
{
	template<typename CBufType>
	inline void UpdateBuffer(ID3D11Buffer* destBuf, CBufType& srcBuf, ID3D11DeviceContext* dc)
	{
		D3D11_MAPPED_SUBRESOURCE mappedbuffer;
		ThrowIfFailed(dc->Map(destBuf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedbuffer));

		CBufType* data = reinterpret_cast<CBufType*>(mappedbuffer.pData);

		memcpy(data, &srcBuf, sizeof(CBufType));

		dc->Unmap(destBuf, 0);
	}
}