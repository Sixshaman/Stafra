#include "Util.hpp"
#include <comdef.h>
#include <clocale>
#include <wrl/client.h>
#include <d3dcompiler.h>

Utils::DXException::DXException(HRESULT hr, const std::wstring& funcName, const std::wstring& filename, int32_t line)
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

std::wstring Utils::DXException::ToString() const
{
	_com_error err(mErrorCode);
	std::wstring msg = err.ErrorMessage();

	if(ToHR() == DXGI_ERROR_DEVICE_REMOVED)
	{
		msg = L"DXGI_ERROR_DEVICE_REMOVED detected (probably killed by TDR mechanism). Your graphics card isn't good enough.";
	}

	return L"\r\n\r\n\r\n" + mFuncName + L" failed in " + mFilename + L"; line " + std::to_wstring(mLineNumber) + L"; error: " + msg + L"\r\n\r\n\r\n";
}

HRESULT Utils::DXException::ToHR() const
{
	return mErrorCode;
}

const std::wstring Utils::GetShaderPath()
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

HRESULT Utils::LoadShaderFromFile(ID3D11Device* device, const std::wstring& path, ID3D11ComputeShader** shader)
{
	HRESULT hr = S_OK;
	Microsoft::WRL::ComPtr<ID3DBlob> shaderBlob;

	hr = D3DReadFileToBlob(path.c_str(), shaderBlob.GetAddressOf());
	if(FAILED(hr))
	{
		return hr;
	}

	hr = device->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, shader);
	if(FAILED(hr))
	{
		return hr;
	}

	return S_OK;
}

HRESULT Utils::LoadShaderFromFile(ID3D11Device* device, const std::wstring& path, ID3D11VertexShader** shader)
{
	HRESULT hr = S_OK;
	Microsoft::WRL::ComPtr<ID3DBlob> shaderBlob;

	hr = D3DReadFileToBlob(path.c_str(), shaderBlob.GetAddressOf());
	if(FAILED(hr))
	{
		return hr;
	}

	hr = device->CreateVertexShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, shader);
	if(FAILED(hr))
	{
		return hr;
	}

	return S_OK;
}

HRESULT Utils::LoadShaderFromFile(ID3D11Device* device, const std::wstring& path, ID3D11PixelShader** shader)
{
	HRESULT hr = S_OK;
	Microsoft::WRL::ComPtr<ID3DBlob> shaderBlob;

	hr = D3DReadFileToBlob(path.c_str(), shaderBlob.GetAddressOf());
	if(FAILED(hr))
	{
		return hr;
	}

	hr = device->CreatePixelShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, shader);
	if(FAILED(hr))
	{
		return hr;
	}

	return S_OK;
}
