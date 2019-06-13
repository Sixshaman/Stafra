#include "FractalGen.hpp"
#include "Util.hpp"
#include <iostream>
#include <d3dcompiler.h>
#include <vector>
#include <fstream>
#include <algorithm>
#include <libpng16/png.h>
#include <sstream>
#include "3rd party/WICTextureLoader.h"
#include "PNGSaver.hpp"

static const uint32_t downscaledSize = 1024;

template<typename CBufType>
inline void UpdateBuffer(ID3D11Buffer* destBuf, CBufType& srcBuf, ID3D11DeviceContext* dc)
{
	D3D11_MAPPED_SUBRESOURCE mappedbuffer;
	ThrowIfFailed(dc->Map(destBuf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedbuffer));

	CBufType* data = reinterpret_cast<CBufType*>(mappedbuffer.pData);

	memcpy(data, &srcBuf, sizeof(CBufType));

	dc->Unmap(destBuf, 0);
}

FractalGen::FractalGen(uint32_t pow2Size): mSizeLo(1)
{
	mSizeLo = (1ull << pow2Size) - 1;

#if defined (DEBUG) || defined(_DEBUG)
	uint32_t flags = D3D11_CREATE_DEVICE_DEBUG;
#else
	uint32_t flags = 0;
#endif
     
	ThrowIfFailed(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, nullptr, 0, D3D11_SDK_VERSION, mDevice.GetAddressOf(), nullptr, mDeviceContext.GetAddressOf()));

	ThrowIfFailed(mDevice->SetExceptionMode(D3D11_RAISE_FLAG_DRIVER_INTERNAL_ERROR));

	CreateTextures();
	CreateShaderData();
}

FractalGen::~FractalGen()
{
}

void FractalGen::ComputeFractal()
{
	uint32_t solutionPeriod = (mSizeLo + 1) / 2; //True for any normal Lights Out puzzle of size 2^n - 1

	InitTextures();

	for (uint32_t i = 0; i < solutionPeriod; i++)
	{
		std::ostringstream namestr;
		namestr.fill('0');
		namestr.width((std::streamsize)log10f(solutionPeriod) + 1);
		namestr << i;

		std::cout << namestr.str() << "/" << solutionPeriod << "..." << std::endl;
		StabilityNextStep();

		CheckDeviceLost();

		SaveFractalImage("DiffStabil\\Stabl" + namestr.str() + ".png");
	}

	if(CompareWithInitialState())
	{
		std::cout << "Nice!" << std::endl;
	}
	else
	{
		std::cout << "Oh no!" << std::endl;
	}
}

void FractalGen::SaveFractalImage(const std::string& filename)
{
	std::cout << "Data copying..." << std::endl;

	//Step 0. Create downscaled image
	DownscalePicture(downscaledSize);

	//Step 1. Copy the fractal tex into the staging copy tex
	mDeviceContext->CopyResource(mDownscaledStabilityTexCopy.Get(), mDownscaledStabilityTex.Get());

	//Step 2. Copy the fractal tex data to RAM
	std::vector<uint8_t> stabilityData;
	CopyStabilityTextureData(stabilityData);

	uint32_t rowPitch = stabilityData.size() / downscaledSize;
	
	std::cout << "Actually saving...";
	PngSaver pngSaver;
	if(!pngSaver)
	{
		std::cout << "Error!" << std::endl;
		return;
	}

	pngSaver.SavePngImage(filename, downscaledSize, downscaledSize, rowPitch, stabilityData);
	std::cout << "Yaaay!" << std::endl;
}

void FractalGen::CreateTextures()
{
	D3D11_TEXTURE2D_DESC boardTexDesc;
	boardTexDesc.Width              = mSizeLo;
	boardTexDesc.Height             = mSizeLo;
	boardTexDesc.Format             = DXGI_FORMAT_R8_UINT;
	boardTexDesc.Usage              = D3D11_USAGE_DEFAULT;
	boardTexDesc.BindFlags          = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	boardTexDesc.CPUAccessFlags     = 0;
	boardTexDesc.ArraySize          = 1;
	boardTexDesc.MipLevels          = 1;
	boardTexDesc.SampleDesc.Count   = 1;
	boardTexDesc.SampleDesc.Quality = 0;
	boardTexDesc.MiscFlags          = 0;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> prevStabilityTex = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> currStabilityTex = nullptr;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> prevBoardTex = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> currBoardTex = nullptr;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> initialBoardTex = nullptr; //For comparison

	Microsoft::WRL::ComPtr<ID3D11Texture2D> prevEqualityTex = nullptr; //For comparison
	Microsoft::WRL::ComPtr<ID3D11Texture2D> nextEqualityTex = nullptr; //For comparison

	ThrowIfFailed(mDevice->CreateTexture2D(&boardTexDesc, nullptr, prevStabilityTex.GetAddressOf()));
	ThrowIfFailed(mDevice->CreateTexture2D(&boardTexDesc, nullptr, currStabilityTex.GetAddressOf()));

	ThrowIfFailed(mDevice->CreateTexture2D(&boardTexDesc, nullptr, prevBoardTex.GetAddressOf()));
	ThrowIfFailed(mDevice->CreateTexture2D(&boardTexDesc, nullptr, currBoardTex.GetAddressOf()));

	ThrowIfFailed(mDevice->CreateTexture2D(&boardTexDesc, nullptr, initialBoardTex.GetAddressOf()));

	ThrowIfFailed(mDevice->CreateTexture2D(&boardTexDesc, nullptr, prevEqualityTex.GetAddressOf()));
	ThrowIfFailed(mDevice->CreateTexture2D(&boardTexDesc, nullptr, nextEqualityTex.GetAddressOf()));

	D3D11_SHADER_RESOURCE_VIEW_DESC boardSrvDesc;
	boardSrvDesc.Format                    = DXGI_FORMAT_R8_UINT;
	boardSrvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
	boardSrvDesc.Texture2D.MipLevels       = 1;
	boardSrvDesc.Texture2D.MostDetailedMip = 0;

	ThrowIfFailed(mDevice->CreateShaderResourceView(prevStabilityTex.Get(), &boardSrvDesc, mPrevStabilitySRV.GetAddressOf()));
	ThrowIfFailed(mDevice->CreateShaderResourceView(currStabilityTex.Get(), &boardSrvDesc, mCurrStabilitySRV.GetAddressOf()));

	ThrowIfFailed(mDevice->CreateShaderResourceView(prevBoardTex.Get(), &boardSrvDesc, mPrevBoardSRV.GetAddressOf()));
	ThrowIfFailed(mDevice->CreateShaderResourceView(currBoardTex.Get(), &boardSrvDesc, mCurrBoardSRV.GetAddressOf()));

	ThrowIfFailed(mDevice->CreateShaderResourceView(initialBoardTex.Get(), &boardSrvDesc, mInitialBoardSRV.GetAddressOf()));

	ThrowIfFailed(mDevice->CreateShaderResourceView(prevEqualityTex.Get(), &boardSrvDesc, mPrevEqualitySRV.GetAddressOf()));
	ThrowIfFailed(mDevice->CreateShaderResourceView(nextEqualityTex.Get(), &boardSrvDesc, mNextEqualitySRV.GetAddressOf()));

	D3D11_UNORDERED_ACCESS_VIEW_DESC boardUavDesc;
	boardUavDesc.Format             = DXGI_FORMAT_R8_UINT;
	boardUavDesc.ViewDimension      = D3D11_UAV_DIMENSION_TEXTURE2D;
	boardUavDesc.Texture2D.MipSlice = 0;

	ThrowIfFailed(mDevice->CreateUnorderedAccessView(prevStabilityTex.Get(), &boardUavDesc, mPrevStabilityUAV.GetAddressOf()));
	ThrowIfFailed(mDevice->CreateUnorderedAccessView(currStabilityTex.Get(), &boardUavDesc, mCurrStabilityUAV.GetAddressOf()));

	ThrowIfFailed(mDevice->CreateUnorderedAccessView(prevBoardTex.Get(), &boardUavDesc, mPrevBoardUAV.GetAddressOf()));
	ThrowIfFailed(mDevice->CreateUnorderedAccessView(currBoardTex.Get(), &boardUavDesc, mCurrBoardUAV.GetAddressOf()));

	ThrowIfFailed(mDevice->CreateUnorderedAccessView(initialBoardTex.Get(), &boardUavDesc, mInitialBoardUAV.GetAddressOf()));

	ThrowIfFailed(mDevice->CreateUnorderedAccessView(prevEqualityTex.Get(), &boardUavDesc, mPrevEqualityUAV.GetAddressOf()));
	ThrowIfFailed(mDevice->CreateUnorderedAccessView(nextEqualityTex.Get(), &boardUavDesc, mNextEqualityUAV.GetAddressOf()));

	D3D11_BUFFER_DESC equalityBufferDesc;
	equalityBufferDesc.ByteWidth           = sizeof(uint32_t);
	equalityBufferDesc.Usage               = D3D11_USAGE_DEFAULT;
	equalityBufferDesc.BindFlags           = D3D11_BIND_UNORDERED_ACCESS;
	equalityBufferDesc.CPUAccessFlags      = 0;
	equalityBufferDesc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	equalityBufferDesc.StructureByteStride = sizeof(uint32_t);

	ThrowIfFailed(mDevice->CreateBuffer(&equalityBufferDesc, nullptr, &mEqualityBuffer));

	D3D11_UNORDERED_ACCESS_VIEW_DESC equalityUavDesc;
	equalityUavDesc.Format              = DXGI_FORMAT_UNKNOWN;
	equalityUavDesc.ViewDimension       = D3D11_UAV_DIMENSION_BUFFER;
	equalityUavDesc.Buffer.FirstElement = 0;
	equalityUavDesc.Buffer.NumElements  = 1;
	equalityUavDesc.Buffer.Flags        = 0;

	ThrowIfFailed(mDevice->CreateUnorderedAccessView(mEqualityBuffer.Get(), &equalityUavDesc, mEqualityBufferUAV.GetAddressOf()));

	D3D11_BUFFER_DESC equalityBufferCopyDesc;
	equalityBufferCopyDesc.ByteWidth           = sizeof(uint32_t);
	equalityBufferCopyDesc.Usage               = D3D11_USAGE_STAGING;
	equalityBufferCopyDesc.BindFlags           = 0;
	equalityBufferCopyDesc.CPUAccessFlags      = D3D11_CPU_ACCESS_READ;
	equalityBufferCopyDesc.MiscFlags           = 0;
	equalityBufferCopyDesc.StructureByteStride = 0;

	ThrowIfFailed(mDevice->CreateBuffer(&equalityBufferCopyDesc, nullptr, &mEqualityBufferCopy));

	D3D11_TEXTURE2D_DESC finalPictureTexDesc;
	finalPictureTexDesc.Width              = mSizeLo;
	finalPictureTexDesc.Height             = mSizeLo;
	finalPictureTexDesc.Format             = DXGI_FORMAT_R32_FLOAT;
	finalPictureTexDesc.Usage              = D3D11_USAGE_DEFAULT;
	finalPictureTexDesc.BindFlags          = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	finalPictureTexDesc.CPUAccessFlags     = 0;
	finalPictureTexDesc.ArraySize          = 1;
	finalPictureTexDesc.MipLevels          = 1;
	finalPictureTexDesc.SampleDesc.Count   = 1;
	finalPictureTexDesc.SampleDesc.Quality = 0;
	finalPictureTexDesc.MiscFlags          = 0;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> finalTex = nullptr;
	ThrowIfFailed(mDevice->CreateTexture2D(&finalPictureTexDesc, nullptr, finalTex.GetAddressOf()));

	D3D11_SHADER_RESOURCE_VIEW_DESC finalSrvDesc;
	finalSrvDesc.Format                    = DXGI_FORMAT_R32_FLOAT;
	finalSrvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
	finalSrvDesc.Texture2D.MipLevels       = 1;
	finalSrvDesc.Texture2D.MostDetailedMip = 0;

	ThrowIfFailed(mDevice->CreateShaderResourceView(finalTex.Get(), &finalSrvDesc, mFinalStateSRV.GetAddressOf()));

	D3D11_UNORDERED_ACCESS_VIEW_DESC finalUavDesc;
	finalUavDesc.Format             = DXGI_FORMAT_R32_FLOAT;
	finalUavDesc.ViewDimension      = D3D11_UAV_DIMENSION_TEXTURE2D;
	finalUavDesc.Texture2D.MipSlice = 0;

	ThrowIfFailed(mDevice->CreateUnorderedAccessView(finalTex.Get(), &finalUavDesc, mFinalStateUAV.GetAddressOf()));

	D3D11_TEXTURE2D_DESC downscaledPictureTexDesc;
	downscaledPictureTexDesc.Width              = downscaledSize;
	downscaledPictureTexDesc.Height             = downscaledSize;
	downscaledPictureTexDesc.Format             = DXGI_FORMAT_R32_FLOAT;
	downscaledPictureTexDesc.Usage              = D3D11_USAGE_DEFAULT;
	downscaledPictureTexDesc.BindFlags          = D3D11_BIND_RENDER_TARGET;
	downscaledPictureTexDesc.CPUAccessFlags     = 0;
	downscaledPictureTexDesc.ArraySize          = 1;
	downscaledPictureTexDesc.MipLevels          = 1;
	downscaledPictureTexDesc.SampleDesc.Count   = 1;
	downscaledPictureTexDesc.SampleDesc.Quality = 0;
	downscaledPictureTexDesc.MiscFlags          = 0;

	ThrowIfFailed(mDevice->CreateTexture2D(&downscaledPictureTexDesc, nullptr, mDownscaledStabilityTex.GetAddressOf()));
	
	D3D11_RENDER_TARGET_VIEW_DESC downscaledRTVDesc;
	downscaledRTVDesc.Format             = DXGI_FORMAT_R32_FLOAT;
	downscaledRTVDesc.ViewDimension      = D3D11_RTV_DIMENSION_TEXTURE2D;
	downscaledRTVDesc.Texture2D.MipSlice = 0;

	ThrowIfFailed(mDevice->CreateRenderTargetView(mDownscaledStabilityTex.Get(), &downscaledRTVDesc, mDownscaledStateRTV.GetAddressOf()));

	D3D11_TEXTURE2D_DESC downscaledPictureTexCopyDesc;
	downscaledPictureTexCopyDesc.Width              = downscaledSize;
	downscaledPictureTexCopyDesc.Height             = downscaledSize;
	downscaledPictureTexCopyDesc.Format             = DXGI_FORMAT_R32_FLOAT;
	downscaledPictureTexCopyDesc.Usage              = D3D11_USAGE_STAGING;
	downscaledPictureTexCopyDesc.BindFlags          = 0;
	downscaledPictureTexCopyDesc.CPUAccessFlags     = D3D11_CPU_ACCESS_READ;
	downscaledPictureTexCopyDesc.ArraySize          = 1;
	downscaledPictureTexCopyDesc.MipLevels          = 1;
	downscaledPictureTexCopyDesc.SampleDesc.Count   = 1;
	downscaledPictureTexCopyDesc.SampleDesc.Quality = 0;
	downscaledPictureTexCopyDesc.MiscFlags          = 0;

	ThrowIfFailed(mDevice->CreateTexture2D(&downscaledPictureTexCopyDesc, nullptr, mDownscaledStabilityTexCopy.GetAddressOf()));
}

void FractalGen::CreateShaderData()
{
	Microsoft::WRL::ComPtr<ID3DBlob> shaderBlob;

#if defined(DEBUG) || defined(_DEBUG)
	std::wstring ShaderPath = LR"(Shaders\Debug\)";
#else
	std::wstring ShaderPath = LR"(Shaders\Release\)";
#endif

	ThrowIfFailed(D3DReadFileToBlob((ShaderPath + L"Clear4CornersCS.cso").c_str(), shaderBlob.GetAddressOf()));
	ThrowIfFailed(mDevice->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, mClear4CornersShader.GetAddressOf()));

	ThrowIfFailed(D3DReadFileToBlob((ShaderPath + L"InitialStateTransformCS.cso").c_str(), shaderBlob.GetAddressOf()));
	ThrowIfFailed(mDevice->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, mInitialStateTransformShader.GetAddressOf()));

	ThrowIfFailed(D3DReadFileToBlob((ShaderPath + L"FinalStateTransformCS.cso").c_str(), shaderBlob.GetAddressOf()));
	ThrowIfFailed(mDevice->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, mFinalStateTransformShader.GetAddressOf()));

	ThrowIfFailed(D3DReadFileToBlob((ShaderPath + L"CompareBoardsStartCS.cso").c_str(), shaderBlob.GetAddressOf()));
	ThrowIfFailed(mDevice->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, mCompareBoardsStartShader.GetAddressOf()));

	ThrowIfFailed(D3DReadFileToBlob((ShaderPath + L"CompareBoardsShrinkCS.cso").c_str(), shaderBlob.GetAddressOf()));
	ThrowIfFailed(mDevice->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, mCompareBoardsShrinkShader.GetAddressOf()));

	ThrowIfFailed(D3DReadFileToBlob((ShaderPath + L"CompareBoardsFinalCS.cso").c_str(), shaderBlob.GetAddressOf()));
	ThrowIfFailed(mDevice->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, mCompareBoardsFinalShader.GetAddressOf()));

	ThrowIfFailed(D3DReadFileToBlob((ShaderPath + L"StabilityNextStepCS.cso").c_str(), shaderBlob.GetAddressOf()));
	ThrowIfFailed(mDevice->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, mStabilityNextStepShader.GetAddressOf()));

	ThrowIfFailed(D3DReadFileToBlob((ShaderPath + L"DownscaleBigPictureVS.cso").c_str(), shaderBlob.GetAddressOf()));
	ThrowIfFailed(mDevice->CreateVertexShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, mDownscaleVertexShader.GetAddressOf()));

	ThrowIfFailed(D3DReadFileToBlob((ShaderPath + L"DownscaleBigPicturePS.cso").c_str(), shaderBlob.GetAddressOf()));
	ThrowIfFailed(mDevice->CreatePixelShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, mDownscalePixelShader.GetAddressOf()));

	D3D11_BUFFER_DESC cbDesc;
	cbDesc.Usage               = D3D11_USAGE_DYNAMIC;
	cbDesc.ByteWidth           = (sizeof(CBParamsStruct) + 0xff) & (~0xff);
	cbDesc.BindFlags           = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
	cbDesc.MiscFlags           = 0;
	cbDesc.StructureByteStride = 0;

	CBParamsStruct initData;
	initData.BoardSize    = DirectX::XMUINT2(mSizeLo, mSizeLo);
	initData.RealTileSize = DirectX::XMUINT2(0, 0);

	D3D11_SUBRESOURCE_DATA cbData;
	cbData.pSysMem          = &initData;
	cbData.SysMemPitch      = 0;
	cbData.SysMemSlicePitch = 0;

	ThrowIfFailed(mDevice->CreateBuffer(&cbDesc, &cbData, &mCBufferParams));

	D3D11_SAMPLER_DESC samDesc;
	ZeroMemory(&samDesc, sizeof(D3D11_SAMPLER_DESC));
	samDesc.AddressU      = D3D11_TEXTURE_ADDRESS_CLAMP;
	samDesc.AddressV      = D3D11_TEXTURE_ADDRESS_CLAMP;
	samDesc.AddressW      = D3D11_TEXTURE_ADDRESS_CLAMP;
	samDesc.Filter        = D3D11_FILTER_ANISOTROPIC;
	samDesc.MaxAnisotropy = 4;
	
	ThrowIfFailed(mDevice->CreateSamplerState(&samDesc, mBestSamplerEver.GetAddressOf()));

	mViewport.TopLeftX = 0.0f;
	mViewport.TopLeftY = 0.0f;
	mViewport.Width    = (float)downscaledSize;
	mViewport.Height   = (float)downscaledSize;
	mViewport.MinDepth = 0.0f;
	mViewport.MaxDepth = 1.0f;

	D3D11_VIEWPORT viewports[] = {mViewport};
	mDeviceContext->RSSetViewports(1, viewports);
}

void FractalGen::InitTextures()
{
	std::cout << "Initializing...";

	UINT clearVal[] = {1, 1, 1, 1};
	mDeviceContext->ClearUnorderedAccessViewUint(mCurrStabilityUAV.Get(), clearVal);

	//Check if initial state file exists and has correct data; if not, fall back to the default state
	DWORD fileAttribs = GetFileAttributes("InitialState.png");
	if(fileAttribs == INVALID_FILE_ATTRIBUTES || fileAttribs & FILE_ATTRIBUTE_DIRECTORY)
	{
		std::cout << "InitialState.png not found. Default \"4 corners\" state will be used." << std::endl;
		CreateDefaultInitialBoard();
	}
	else
	{
		std::cout << "Initial state file found! Trying to load...";

		bool loadRes = LoadInitialState();
		if(!loadRes)
		{
			std::cout << "ERROR! Falling back to default \"4 corners\" state..." << std::endl;
			CreateDefaultInitialBoard();
		}
		else
		{
			std::cout << "Yay!" << std::endl;
		}
	}

	std::swap(mCurrStabilitySRV, mPrevStabilitySRV);
	std::swap(mCurrStabilityUAV, mPrevStabilityUAV);
	std::swap(mCurrBoardSRV,     mPrevBoardSRV);
	std::swap(mCurrBoardUAV,     mPrevBoardUAV);

	std::cout << "Initializing Finished!" << std::endl;
}

void FractalGen::StabilityNextStep()
{
	ID3D11ShaderResourceView*  stabilityNextStepSRVs[] = { mPrevBoardSRV.Get(), mPrevStabilitySRV.Get() };
	ID3D11UnorderedAccessView* stabilityNextStepUAVs[] = { mCurrBoardUAV.Get(), mCurrStabilityUAV.Get() };

	mDeviceContext->CSSetShaderResources(0, 2, stabilityNextStepSRVs);
	mDeviceContext->CSSetUnorderedAccessViews(0, 2, stabilityNextStepUAVs, nullptr);

	mDeviceContext->CSSetShader(mStabilityNextStepShader.Get(), nullptr, 0);
	mDeviceContext->Dispatch((uint32_t)(ceilf(mSizeLo / 32.0f)), (uint32_t)(ceilf(mSizeLo / 32.0f)), 1);

	ID3D11ShaderResourceView*  nullSRVs[] = { nullptr, nullptr };
	ID3D11UnorderedAccessView* nullUAVs[] = { nullptr, nullptr };

	mDeviceContext->CSSetShaderResources(0, 2, nullSRVs);
	mDeviceContext->CSSetUnorderedAccessViews(0, 2, nullUAVs, nullptr);
	mDeviceContext->CSSetShader(nullptr, nullptr, 0);

	std::swap(mCurrStabilitySRV, mPrevStabilitySRV);
	std::swap(mCurrStabilityUAV, mPrevStabilityUAV);
	std::swap(mCurrBoardSRV,     mPrevBoardSRV);
	std::swap(mCurrBoardUAV,     mPrevBoardUAV);
}

bool FractalGen::CompareWithInitialState()
{
	std::cout << "Ensuring equality...";

	//Step 1. Create boolean texture with per pixel compare states
	ID3D11ShaderResourceView*  compareStartSRVs[] = { mPrevBoardSRV.Get(), mInitialBoardSRV.Get() };
	ID3D11UnorderedAccessView* compareStartUAVs[] = { mNextEqualityUAV.Get() };

	mDeviceContext->CSSetShaderResources(0, 2, compareStartSRVs);
	mDeviceContext->CSSetUnorderedAccessViews(0, 1, compareStartUAVs, nullptr);

	mDeviceContext->CSSetShader(mCompareBoardsStartShader.Get(), nullptr, 0);
	mDeviceContext->Dispatch((uint32_t)(ceilf(mSizeLo / 32.0f)), (uint32_t)(ceilf(mSizeLo / 32.0f)), 1);

	ID3D11Buffer*          nullCbuffers[] = { nullptr };
	ID3D11ShaderResourceView*  nullSRVs[] = { nullptr, nullptr };
	ID3D11UnorderedAccessView* nullUAVs[] = { nullptr };

	mDeviceContext->CSSetShaderResources(0, 2, nullSRVs);
	mDeviceContext->CSSetUnorderedAccessViews(0, 1, nullUAVs, nullptr);

	std::swap(mPrevEqualitySRV, mNextEqualitySRV);
	std::swap(mPrevEqualityUAV, mNextEqualityUAV);

	//Step 2. Shrink compare texture to < 32x32 tile
	uint32_t sizeCur = mSizeLo;
	mDeviceContext->CSSetShader(mCompareBoardsShrinkShader.Get(), nullptr, 0);
	while(sizeCur > 32)
	{
		ID3D11ShaderResourceView*  compareShrinkSRVs[] = { mPrevEqualitySRV.Get() };
		ID3D11UnorderedAccessView* compareShrinkUAVs[] = { mNextEqualityUAV.Get() };

		mDeviceContext->CSSetShaderResources(0, 1, compareShrinkSRVs);
		mDeviceContext->CSSetUnorderedAccessViews(0, 1, compareShrinkUAVs, nullptr);

		mCBufferParamsCopy.BoardSize.x = sizeCur;
		mCBufferParamsCopy.BoardSize.y = sizeCur;
		UpdateBuffer(mCBufferParams.Get(), mCBufferParamsCopy, mDeviceContext.Get());

		ID3D11Buffer* compareShrinkCBuffers[] = { mCBufferParams.Get() };
		mDeviceContext->CSSetConstantBuffers(0, 1, compareShrinkCBuffers);

		uint32_t shrankSize = (uint32_t)(ceilf(sizeCur / 32.0f));
		mDeviceContext->Dispatch(shrankSize, shrankSize, 1);

		mDeviceContext->CSSetShaderResources(0, 1, nullSRVs);
		mDeviceContext->CSSetUnorderedAccessViews(0, 1, nullUAVs, nullptr);
		mDeviceContext->CSSetConstantBuffers(0, 1, nullCbuffers);

		std::swap(mPrevEqualitySRV, mNextEqualitySRV);
		std::swap(mPrevEqualityUAV, mNextEqualityUAV);

		sizeCur = shrankSize;
	}

	//Step 3. Finally shrink the last remaining tile
	ID3D11ShaderResourceView*  compareFinalSRVs[] = { mPrevEqualitySRV.Get() };
	ID3D11UnorderedAccessView* compareFinalUAVs[] = { mEqualityBufferUAV.Get() };

	mDeviceContext->CSSetShaderResources(0, 1, compareFinalSRVs);
	mDeviceContext->CSSetUnorderedAccessViews(0, 1, compareFinalUAVs, nullptr);

	mCBufferParamsCopy.RealTileSize.x = sizeCur;
	mCBufferParamsCopy.RealTileSize.y = sizeCur;
	UpdateBuffer(mCBufferParams.Get(), mCBufferParamsCopy, mDeviceContext.Get());

	ID3D11Buffer* compareFinalCBuffer[] = { mCBufferParams.Get() };
	mDeviceContext->CSSetConstantBuffers(0, 1, compareFinalCBuffer);

	mDeviceContext->CSSetShader(mCompareBoardsFinalShader.Get(), nullptr, 0);
	mDeviceContext->Dispatch(1, 1, 1);

	mDeviceContext->CSSetShaderResources(0, 1, nullSRVs);
	mDeviceContext->CSSetUnorderedAccessViews(0, 1, nullUAVs, nullptr);
	mDeviceContext->CSSetConstantBuffers(0, 1, nullCbuffers);

	mDeviceContext->CSSetShader(nullptr, nullptr, 0);

	mDeviceContext->CopyResource(mEqualityBufferCopy.Get(), mEqualityBuffer.Get());

	uint32_t compareRes = GetEqualityBufferData();
	return (compareRes != 0);
}

void FractalGen::CopyStabilityTextureData(std::vector<uint8_t>& stabilityData)
{
	stabilityData.clear();
	std::vector<float> downscaledData;

	D3D11_MAPPED_SUBRESOURCE mappedTex;
	ThrowIfFailed(mDeviceContext->Map(mDownscaledStabilityTexCopy.Get(), 0, D3D11_MAP_READ, 0, &mappedTex));

	float* data = reinterpret_cast<float*>(mappedTex.pData);
	downscaledData.resize((size_t)mappedTex.RowPitch * downscaledSize / sizeof(float));

	std::copy(data, data + downscaledData.size(), downscaledData.begin());
	mDeviceContext->Unmap(mDownscaledStabilityTexCopy.Get(), 0);

	stabilityData.resize(downscaledData.size());
	std::transform(downscaledData.begin(), downscaledData.end(), stabilityData.begin(), [](float val){return (uint8_t)(val * 255.0f);});
}

uint32_t FractalGen::GetEqualityBufferData()
{
	D3D11_MAPPED_SUBRESOURCE mappedBuf;
	ThrowIfFailed(mDeviceContext->Map(mEqualityBufferCopy.Get(), 0, D3D11_MAP_READ, 0, &mappedBuf));

	uint32_t* data = reinterpret_cast<uint32_t*>(mappedBuf.pData);
	uint32_t result = *data;

	mDeviceContext->Unmap(mEqualityBufferCopy.Get(), 0);

	return result;
}

void FractalGen::CreateDefaultInitialBoard()
{
	mCBufferParamsCopy.BoardSize.x = mSizeLo;
	mCBufferParamsCopy.BoardSize.y = mSizeLo;
	UpdateBuffer(mCBufferParams.Get(), mCBufferParamsCopy, mDeviceContext.Get());

	ID3D11Buffer* clear4CornersCbuffers[] = { mCBufferParams.Get() };
	mDeviceContext->CSSetConstantBuffers(0, 1, clear4CornersCbuffers);

	ID3D11UnorderedAccessView* clear4CornersInitUAVs[] = { mInitialBoardUAV.Get() };
	mDeviceContext->CSSetUnorderedAccessViews(0, 1, clear4CornersInitUAVs, nullptr);

	mDeviceContext->CSSetShader(mClear4CornersShader.Get(), nullptr, 0);
	mDeviceContext->Dispatch((uint32_t)(ceilf(mSizeLo / 32.0f)), (uint32_t)(ceilf(mSizeLo / 32.0f)), 1);

	ID3D11UnorderedAccessView* clear4CornersUAVs[] = { mCurrBoardUAV.Get() };
	mDeviceContext->CSSetUnorderedAccessViews(0, 1, clear4CornersUAVs, nullptr);

	mDeviceContext->CSSetShader(mClear4CornersShader.Get(), nullptr, 0);
	mDeviceContext->Dispatch((uint32_t)(ceilf(mSizeLo / 32.0f)), (uint32_t)(ceilf(mSizeLo / 32.0f)), 1);

	ID3D11Buffer*          nullCbuffers[] = { nullptr };
	ID3D11UnorderedAccessView* nullUAVs[] = { nullptr };

	mDeviceContext->CSSetConstantBuffers(0, 1, nullCbuffers);
	mDeviceContext->CSSetUnorderedAccessViews(0, 1, nullUAVs, nullptr);
	mDeviceContext->CSSetShader(nullptr, nullptr, 0);
}

bool FractalGen::LoadInitialState()
{
	ThrowIfFailed(CoInitializeEx(nullptr, COINIT_MULTITHREADED));

	//Attempt to load the initial state
	Microsoft::WRL::ComPtr<ID3D11Texture2D>          initialTex;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> initialStateSRV;
	HRESULT hr = DirectX::CreateWICTextureFromFile(mDevice.Get(), L"InitialState.png", reinterpret_cast<ID3D11Resource**>(initialTex.GetAddressOf()), initialStateSRV.GetAddressOf());
	if(FAILED(hr))
	{
		return false;
	}

	D3D11_TEXTURE2D_DESC texDesc;
	initialTex->GetDesc(&texDesc);

	if(texDesc.Width != texDesc.Height)
	{
		std::cout << "Wrong size!" << " ";
		return false;
	}

	if(((texDesc.Width + 1) & texDesc.Width) != 0) //Check if texDesc.Width is power of 2 minus 1
	{
		std::cout << "Wrong size!" << " ";
		return false;
	}

	std::cout << "Good! Changing board size...";
	mSizeLo = texDesc.Width; //Previous one doesn't matter anymore

	//Create the initial board from the initial state
	ID3D11ShaderResourceView* initialStateTransformSRVs[] = { initialStateSRV.Get() };
	mDeviceContext->CSSetShaderResources(0, 1, initialStateTransformSRVs);

	ID3D11UnorderedAccessView* initialStateTransformUAVs[] = { mInitialBoardUAV.Get() };
	mDeviceContext->CSSetUnorderedAccessViews(0, 1, initialStateTransformUAVs, nullptr);

	mDeviceContext->CSSetShader(mInitialStateTransformShader.Get(), nullptr, 0);
	mDeviceContext->Dispatch((uint32_t)(ceilf(mSizeLo / 32.0f)), (uint32_t)(ceilf(mSizeLo / 32.0f)), 1);

	ID3D11UnorderedAccessView* initialTransformUAVs[] = { mCurrBoardUAV.Get() };
	mDeviceContext->CSSetUnorderedAccessViews(0, 1, initialTransformUAVs, nullptr);

	mDeviceContext->CSSetShader(mInitialStateTransformShader.Get(), nullptr, 0);
	mDeviceContext->Dispatch((uint32_t)(ceilf(mSizeLo / 32.0f)), (uint32_t)(ceilf(mSizeLo / 32.0f)), 1);

	ID3D11ShaderResourceView*  nullSRVs[] = { nullptr };
	ID3D11UnorderedAccessView* nullUAVs[] = { nullptr };

	mDeviceContext->CSSetShaderResources(0, 1, nullSRVs);
	mDeviceContext->CSSetUnorderedAccessViews(0, 1, nullUAVs, nullptr);
	mDeviceContext->CSSetShader(nullptr, nullptr, 0);

	return true;
}

void FractalGen::CheckDeviceLost()
{
	GetEqualityBufferData(); //Sufficient enough
}

void FractalGen::DownscalePicture(uint32_t newWidth)
{
	ID3D11ShaderResourceView*  finalStateTransformSRVs[] = {mPrevStabilitySRV.Get()};
	ID3D11UnorderedAccessView* finalStateTransformUAVs[] = {mFinalStateUAV.Get()};

	mDeviceContext->CSSetShaderResources(0, 1, finalStateTransformSRVs);
	mDeviceContext->CSSetUnorderedAccessViews(0, 1, finalStateTransformUAVs, nullptr);

	mDeviceContext->CSSetShader(mFinalStateTransformShader.Get(), nullptr, 0);
	mDeviceContext->Dispatch((uint32_t)(ceilf(mSizeLo / 32.0f)), (uint32_t)(ceilf(mSizeLo / 32.0f)), 1);

	ID3D11ShaderResourceView* nullSRVs[]  = { nullptr };
	ID3D11UnorderedAccessView* nullUAVs[] = { nullptr };

	mDeviceContext->CSSetShaderResources(0, 1, nullSRVs);
	mDeviceContext->CSSetUnorderedAccessViews(0, 1, nullUAVs, nullptr);
	mDeviceContext->CSSetShader(nullptr, nullptr, 0);

	mDeviceContext->GenerateMips(mFinalStateSRV.Get());

	ID3D11Buffer* vertexBuffers[] = {nullptr};
	UINT          strides[]       = {0};
	UINT          offsets[]       = {0};

	mDeviceContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
	mDeviceContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);

	mDeviceContext->IASetInputLayout(nullptr);
	mDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	mDeviceContext->VSSetShader(mDownscaleVertexShader.Get(), nullptr, 0);

	ID3D11ShaderResourceView* downscaleSRVs[] = {mFinalStateSRV.Get()};
	mDeviceContext->PSSetShaderResources(0, 1, downscaleSRVs);

	ID3D11SamplerState* samplers[] = { mBestSamplerEver.Get() };
	mDeviceContext->PSSetSamplers(0, 1, samplers);

	mDeviceContext->PSSetShader(mDownscalePixelShader.Get(), nullptr, 0);

	ID3D11RenderTargetView* downscaleRtvs[] = { mDownscaledStateRTV.Get() };
	mDeviceContext->OMSetRenderTargets(1, downscaleRtvs, nullptr);

	FLOAT clearColor[] = {0.0f, 0.0f, 0.0f, 0.0f};
	mDeviceContext->ClearRenderTargetView(mDownscaledStateRTV.Get(), clearColor);

	mDeviceContext->Draw(4, 0);

	mDeviceContext->VSSetShader(nullptr, nullptr, 0);
	mDeviceContext->PSSetShader(nullptr, nullptr, 0);

	ID3D11SamplerState* nullSamplers[] = {mBestSamplerEver.Get()};
	mDeviceContext->PSSetSamplers(0, 1, nullSamplers);
	mDeviceContext->PSSetShaderResources(0, 1, nullSRVs);

	ID3D11RenderTargetView* nullRTV[] = {nullptr};
	mDeviceContext->OMSetRenderTargets(1, nullRTV, nullptr);
}
