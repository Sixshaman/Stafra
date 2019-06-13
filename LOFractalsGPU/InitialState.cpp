#include "InitialState.hpp"
#include "Util.hpp"
#include <d3dcompiler.h>
#include "3rd party/WICTextureLoader.h"

InitialState::InitialState(ID3D11Device* device)
{
	LoadShaderData(device);
}

InitialState::~InitialState()
{
}

LoadError InitialState::LoadFromFile(ID3D11Device* device, ID3D11DeviceContext* dc, const std::wstring& filename, ID3D11UnorderedAccessView* initialBoardUAV, uint32_t& texWidth, uint32_t& texHeight)
{
	ThrowIfFailed(CoInitializeEx(nullptr, COINIT_MULTITHREADED));

	//Attempt to load the initial state
	Microsoft::WRL::ComPtr<ID3D11Texture2D>          initialTex;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> initialStateSRV;
	HRESULT hr = DirectX::CreateWICTextureFromFile(device, filename.c_str(), reinterpret_cast<ID3D11Resource**>(initialTex.GetAddressOf()), initialStateSRV.GetAddressOf());
	if(FAILED(hr))
	{
		return LoadError::ERROR_CANT_READ_FILE;
	}

	D3D11_TEXTURE2D_DESC texDesc;
	initialTex->GetDesc(&texDesc);

	if(((texDesc.Width + 1) & texDesc.Width) != 0) //Check if texDesc.Width is power of 2 minus 1
	{
		return LoadError::ERROR_WRONG_SIZE;
	}

	if(((texDesc.Height + 1) & texDesc.Height) != 0) //Check if texDesc.Width is power of 2 minus 1
	{
		return LoadError::ERROR_WRONG_SIZE;
	}

	texWidth  = texDesc.Width;
	texHeight = texDesc.Height;

	InitialStateTransform(dc, initialStateSRV.Get(), initialBoardUAV, texWidth, texHeight);
	return LoadError::LOAD_SUCCESS;
}

void InitialState::CreateDefault(ID3D11DeviceContext* dc, ID3D11UnorderedAccessView* initialBoardUAV, uint32_t texWidth, uint32_t texHeight)
{
	DefaultInitialState(dc, initialBoardUAV, texWidth, texHeight);
}

void InitialState::DefaultInitialState(ID3D11DeviceContext* dc, ID3D11UnorderedAccessView* initialBoardUAV, uint32_t width, uint32_t height)
{
	mCBufferParamsCopy.BoardSize.x = width;
	mCBufferParamsCopy.BoardSize.y = height;
	UpdateBuffer(mCBufferParams.Get(), mCBufferParamsCopy, dc);

	ID3D11Buffer* clear4CornersCbuffers[] = { mCBufferParams.Get() };
	dc->CSSetConstantBuffers(0, 1, clear4CornersCbuffers);

	ID3D11UnorderedAccessView* clear4CornersInitUAVs[] = { initialBoardUAV };
	dc->CSSetUnorderedAccessViews(0, 1, clear4CornersInitUAVs, nullptr);

	dc->CSSetShader(mClear4CornersShader.Get(), nullptr, 0);
	dc->Dispatch((uint32_t)(ceilf(width / 32.0f)), (uint32_t)(ceilf(height / 32.0f)), 1);

	ID3D11Buffer*          nullCbuffers[] = { nullptr };
	ID3D11UnorderedAccessView* nullUAVs[] = { nullptr };

	dc->CSSetConstantBuffers(0, 1, nullCbuffers);
	dc->CSSetUnorderedAccessViews(0, 1, nullUAVs, nullptr);
	dc->CSSetShader(nullptr, nullptr, 0);
}

void InitialState::InitialStateTransform(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* initialStateSRV, ID3D11UnorderedAccessView* initialBoardUAV, uint32_t width, uint32_t height)
{
	ID3D11ShaderResourceView* initialStateTransformSRVs[] = { initialStateSRV };
	dc->CSSetShaderResources(0, 1, initialStateTransformSRVs);

	ID3D11UnorderedAccessView* initialStateTransformUAVs[] = { initialBoardUAV };
	dc->CSSetUnorderedAccessViews(0, 1, initialStateTransformUAVs, nullptr);

	dc->CSSetShader(mInitialStateTransformShader.Get(), nullptr, 0);
	dc->Dispatch((uint32_t)(ceilf(width / 32.0f)), (uint32_t)(ceilf(height / 32.0f)), 1);

	ID3D11ShaderResourceView*  nullSRVs[] = { nullptr };
	ID3D11UnorderedAccessView* nullUAVs[] = { nullptr };

	dc->CSSetShaderResources(0, 1, nullSRVs);
	dc->CSSetUnorderedAccessViews(0, 1, nullUAVs, nullptr);
	dc->CSSetShader(nullptr, nullptr, 0);
}

void InitialState::LoadShaderData(ID3D11Device* device)
{
	Microsoft::WRL::ComPtr<ID3DBlob> shaderBlob;

	ThrowIfFailed(D3DReadFileToBlob((GetShaderPath() + L"Clear4CornersCS.cso").c_str(), shaderBlob.GetAddressOf()));
	ThrowIfFailed(device->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, mClear4CornersShader.GetAddressOf()));

	ThrowIfFailed(D3DReadFileToBlob((GetShaderPath() + L"InitialStateTransformCS.cso").c_str(), shaderBlob.GetAddressOf()));
	ThrowIfFailed(device->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, mInitialStateTransformShader.GetAddressOf()));

	D3D11_BUFFER_DESC cbDesc;
	cbDesc.Usage               = D3D11_USAGE_DYNAMIC;
	cbDesc.ByteWidth           = (sizeof(CBParamsStruct) + 0xff) & (~0xff);
	cbDesc.BindFlags           = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
	cbDesc.MiscFlags           = 0;
	cbDesc.StructureByteStride = 0;

	CBParamsStruct initData;
	initData.BoardSize = DirectX::XMUINT2(0, 0);

	D3D11_SUBRESOURCE_DATA cbData;
	cbData.pSysMem          = &initData;
	cbData.SysMemPitch      = 0;
	cbData.SysMemSlicePitch = 0;

	ThrowIfFailed(device->CreateBuffer(&cbDesc, &cbData, &mCBufferParams));
}