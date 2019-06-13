#include "FractalGen.hpp"
#include "Util.hpp"
#include <iostream>
#include <d3dcompiler.h>
#include <vector>
#include <fstream>
#include <algorithm>
#include <libpng16/png.h>
#include <sstream>
#include "EqualityChecker.hpp"
#include "PNGSaver.hpp"
#include "InitialState.hpp"

static const uint32_t downscaledSize = 1024;

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

void FractalGen::ComputeFractal(bool saveVideoFrames)
{
	InitTextures();

	uint32_t solutionPeriod = (mSizeLo + 1) / 2; //True for any normal Lights Out puzzle of size 2^n - 1
	for (uint32_t i = 0; i < solutionPeriod; i++)
	{
		std::ostringstream namestr;
		namestr.fill('0');
		namestr.width((std::streamsize)log10f(solutionPeriod) + 1);
		namestr << i;

		std::cout << namestr.str() << "/" << solutionPeriod << "..." << std::endl;
		StabilityNextStep();

		CheckDeviceLost();

		if(saveVideoFrames)
		{
			SaveFractalImage("DiffStabil\\Stabl" + namestr.str() + ".png");
		}
	}

	std::cout << "Ensuring equality...";
	EqualityChecker checker(mDevice.Get(), mSizeLo, mSizeLo);
	if(checker.CheckEquality(mDeviceContext.Get(), mPrevBoardSRV.Get(), mInitialBoardSRV.Get()))
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

	ThrowIfFailed(mDevice->CreateTexture2D(&boardTexDesc, nullptr, prevStabilityTex.GetAddressOf()));
	ThrowIfFailed(mDevice->CreateTexture2D(&boardTexDesc, nullptr, currStabilityTex.GetAddressOf()));

	ThrowIfFailed(mDevice->CreateTexture2D(&boardTexDesc, nullptr, prevBoardTex.GetAddressOf()));
	ThrowIfFailed(mDevice->CreateTexture2D(&boardTexDesc, nullptr, currBoardTex.GetAddressOf()));

	ThrowIfFailed(mDevice->CreateTexture2D(&boardTexDesc, nullptr, initialBoardTex.GetAddressOf()));

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

	D3D11_UNORDERED_ACCESS_VIEW_DESC boardUavDesc;
	boardUavDesc.Format             = DXGI_FORMAT_R8_UINT;
	boardUavDesc.ViewDimension      = D3D11_UAV_DIMENSION_TEXTURE2D;
	boardUavDesc.Texture2D.MipSlice = 0;

	ThrowIfFailed(mDevice->CreateUnorderedAccessView(prevStabilityTex.Get(), &boardUavDesc, mPrevStabilityUAV.GetAddressOf()));
	ThrowIfFailed(mDevice->CreateUnorderedAccessView(currStabilityTex.Get(), &boardUavDesc, mCurrStabilityUAV.GetAddressOf()));

	ThrowIfFailed(mDevice->CreateUnorderedAccessView(prevBoardTex.Get(), &boardUavDesc, mPrevBoardUAV.GetAddressOf()));
	ThrowIfFailed(mDevice->CreateUnorderedAccessView(currBoardTex.Get(), &boardUavDesc, mCurrBoardUAV.GetAddressOf()));

	ThrowIfFailed(mDevice->CreateUnorderedAccessView(initialBoardTex.Get(), &boardUavDesc, mInitialBoardUAV.GetAddressOf()));

	D3D11_TEXTURE2D_DESC finalPictureTexDesc;
	finalPictureTexDesc.Width              = mSizeLo;
	finalPictureTexDesc.Height             = mSizeLo;
	finalPictureTexDesc.Format             = DXGI_FORMAT_R32_FLOAT;
	finalPictureTexDesc.Usage              = D3D11_USAGE_DEFAULT;
	finalPictureTexDesc.BindFlags          = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_RENDER_TARGET;
	finalPictureTexDesc.CPUAccessFlags     = 0;
	finalPictureTexDesc.ArraySize          = 1;
	finalPictureTexDesc.MipLevels          = 1;
	finalPictureTexDesc.SampleDesc.Count   = 1;
	finalPictureTexDesc.SampleDesc.Quality = 0;
	finalPictureTexDesc.MiscFlags          = D3D11_RESOURCE_MISC_GENERATE_MIPS;

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

	ThrowIfFailed(D3DReadFileToBlob((GetShaderPath() + L"FinalStateTransformCS.cso").c_str(), shaderBlob.GetAddressOf()));
	ThrowIfFailed(mDevice->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, mFinalStateTransformShader.GetAddressOf()));

	ThrowIfFailed(D3DReadFileToBlob((GetShaderPath() + L"StabilityNextStepCS.cso").c_str(), shaderBlob.GetAddressOf()));
	ThrowIfFailed(mDevice->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, mStabilityNextStepShader.GetAddressOf()));

	ThrowIfFailed(D3DReadFileToBlob((GetShaderPath() + L"DownscaleBigPictureVS.cso").c_str(), shaderBlob.GetAddressOf()));
	ThrowIfFailed(mDevice->CreateVertexShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, mDownscaleVertexShader.GetAddressOf()));

	ThrowIfFailed(D3DReadFileToBlob((GetShaderPath() + L"DownscaleBigPicturePS.cso").c_str(), shaderBlob.GetAddressOf()));
	ThrowIfFailed(mDevice->CreatePixelShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, mDownscalePixelShader.GetAddressOf()));

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

	uint32_t newWidth  = 0;
	uint32_t newHeight = 0;

	InitialState is(mDevice.Get());
	if(is.LoadFromFile(mDevice.Get(), mDeviceContext.Get(), L"InitialState.png", mInitialBoardUAV.Get(), newWidth, newHeight) == LoadError::LOAD_SUCCESS)
	{
		std::cout << "Initial state file loaded!";
	}
	else
	{
		std::cout << "Error loading InitialState.png. Default \"4 corners\" state will be used." << std::endl;
		is.CreateDefault(mDeviceContext.Get(), mInitialBoardUAV.Get(), mSizeLo, mSizeLo);
	}

	Microsoft::WRL::ComPtr<ID3D11Texture2D> initialBoardTex;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> currBoardTex;

	mInitialBoardSRV->GetResource(reinterpret_cast<ID3D11Resource**>(initialBoardTex.GetAddressOf()));
	mCurrBoardSRV->GetResource(reinterpret_cast<ID3D11Resource**>(currBoardTex.GetAddressOf()));

	mDeviceContext->CopyResource(currBoardTex.Get(), initialBoardTex.Get());

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

void FractalGen::CheckDeviceLost()
{
	//throw;
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
