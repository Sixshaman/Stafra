#include "FractalGen.hpp"
#include "Util.hpp"
#include <iostream>
#include <d3dcompiler.h>
#include <vector>
#include <fstream>
#include <libpng16/png.h>
#include <sstream>
#include "EqualityChecker.hpp"
#include "PNGSaver.hpp"
#include "InitialState.hpp"
#include "Downscaler.hpp"

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

	mDownscaler = std::make_unique<Downscaler>(mDevice.Get(), 1024, 1024);
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

	mDownscaler->DownscalePicture(mDeviceContext.Get(), mPrevStabilitySRV.Get(), mSizeLo, mSizeLo);

	//Step 2. Copy the fractal tex data to RAM
	uint32_t downscaledWidth  = 0;
	uint32_t downscaledHeight = 0;
	uint32_t rowPitch         = 0;
	std::vector<uint8_t> stabilityData;
	mDownscaler->CopyDownscaledTextureData(mDeviceContext.Get(), stabilityData, downscaledWidth, downscaledHeight, rowPitch);
	
	std::cout << "Actually saving...";
	PngSaver pngSaver;
	if(!pngSaver)
	{
		std::cout << "Error!" << std::endl;
		return;
	}

	pngSaver.SavePngImage(filename, downscaledWidth, downscaledHeight, rowPitch, stabilityData);
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
}

void FractalGen::CreateShaderData()
{
	Microsoft::WRL::ComPtr<ID3DBlob> shaderBlob;

	ThrowIfFailed(D3DReadFileToBlob((GetShaderPath() + L"StabilityNextStepCS.cso").c_str(), shaderBlob.GetAddressOf()));
	ThrowIfFailed(mDevice->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, mStabilityNextStepShader.GetAddressOf()));
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

		mSizeLo = newWidth;
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

void FractalGen::CheckDeviceLost()
{
	//throw;
}
