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
#include "StabilityCalculator.hpp"
#include "Downscaler.hpp"

FractalGen::FractalGen(uint32_t pow2Size, uint32_t spawnPeriod): mSizeLo(1), mSpawnPeriod(spawnPeriod)
{
	mSizeLo = (1ull << pow2Size) - 1;

#if defined (DEBUG) || defined(_DEBUG)
	uint32_t flags = D3D11_CREATE_DEVICE_DEBUG;
#else
	uint32_t flags = 0;
#endif
     
	ThrowIfFailed(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, nullptr, 0, D3D11_SDK_VERSION, mDevice.GetAddressOf(), nullptr, mDeviceContext.GetAddressOf()));

	ThrowIfFailed(mDevice->SetExceptionMode(D3D11_RAISE_FLAG_DRIVER_INTERNAL_ERROR));

	mDownscaler          = std::make_unique<Downscaler>(mDevice.Get(), mSizeLo, mSizeLo, 1024, 1024);
	mStabilityCalculator = std::make_unique<StabilityCalculator>(mDevice.Get(), mSizeLo, mSizeLo, spawnPeriod);
}

FractalGen::~FractalGen()
{
}

void FractalGen::ComputeFractal(bool saveVideoFrames, size_t enlonging)
{
	int resInit = mStabilityCalculator->InitTextures(mDevice.Get(), mDeviceContext.Get());

	if(resInit & CUSTOM_INITIAL_BOARD)
	{
		std::cout << "Custom initial board loaded!" << std::endl;
	}
	else
	{
		std::cout << "Default initial board will be used..." << std::endl;
	}

	if(resInit & CUSTOM_CLICK_RULE)
	{
		std::cout << "Custom click rule loaded!" << std::endl;
	}
	else
	{
		std::cout << "Default click rule will be used..." << std::endl;
	}

	uint32_t solutionPeriod = (mSizeLo + 1) / 2; //True for any normal Lights Out puzzle of size 2^n - 1

	solutionPeriod *= enlonging;
	for (uint32_t i = 0; i < solutionPeriod; i++)
	{
		std::ostringstream namestr;
		namestr.fill('0');
		namestr.width((std::streamsize)log10f(solutionPeriod) + 1);
		namestr << i;

		std::cout << namestr.str() << "/" << solutionPeriod << "..." << std::endl;
		mStabilityCalculator->StabilityNextStep(mDeviceContext.Get());

		if(saveVideoFrames)
		{
			SaveFractalImage("DiffStabil\\Stabl" + namestr.str() + ".png", true);
		}
	}

	std::cout << "Ensuring equality...";
	if(mStabilityCalculator->CheckEquality(mDevice.Get(), mDeviceContext.Get()))
	{
		std::cout << "Nice!" << std::endl;
	}
	else
	{
		std::cout << "Oh no!" << std::endl;
	}
}

void FractalGen::SaveFractalImage(const std::string& filename, bool useSmoothTransform, bool useDownscaling)
{
	std::cout << "Data copying..." << std::endl;

	uint32_t downscaledWidth = 0;
	uint32_t downscaledHeight = 0;
	uint32_t rowPitch = 0;
	std::vector<uint8_t> stabilityData;

	uint32_t spawnPeriod = 0;
	if(useSmoothTransform)
	{
		spawnPeriod = mSpawnPeriod;
	}

	if(useDownscaling)
	{
		mDownscaler->DownscalePicture(mDeviceContext.Get(), mStabilityCalculator->GetLastStabilityState(), spawnPeriod, mSizeLo, mSizeLo);
		mDownscaler->CopyDownscaledTextureData(mDeviceContext.Get(), stabilityData, downscaledWidth, downscaledHeight, rowPitch);
	}
	else
	{
		Downscaler bigDownscaler(mDevice.Get(), mSizeLo, mSizeLo, mSizeLo, mSizeLo);
		bigDownscaler.DownscalePicture(mDeviceContext.Get(), mStabilityCalculator->GetLastStabilityState(), spawnPeriod, mSizeLo, mSizeLo);
		bigDownscaler.CopyDownscaledTextureData(mDeviceContext.Get(), stabilityData, downscaledWidth, downscaledHeight, rowPitch);
	}
	
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