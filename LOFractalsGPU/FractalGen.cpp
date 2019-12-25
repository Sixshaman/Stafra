#include "FractalGen.hpp"
#include "Util.hpp"
#include <iostream>
#include <d3dcompiler.h>
#include <vector>
#include <fstream>
#include <libpng16/png.h>
#include <sstream>
#include "EqualityChecker.hpp"
#include "FileMgmt/PNGSaver.hpp"
#include "FileMgmt/PNGOpener.hpp"
#include "StabilityCalculator.hpp"
#include "Downscaler.hpp"
#include <filesystem>

namespace
{
	const std::string gDefaultInitialStateLocation = "InitialState.png";
	const std::string gDefaultClickRuleLocation    = "ClickRule.png";
	const size_t gDefaultLoSize  = 1023;
	const size_t gDownscaledSize = 1024;
}

FractalGen::FractalGen(uint32_t pow2Size, uint32_t spawnPeriod): mSizeLo(1), mSpawnPeriod(spawnPeriod)
{
	if(pow2Size == 0)
	{
		if(std::filesystem::exists(gDefaultInitialStateLocation))
		{
			mSizeLo = SizeLoFromImageSize(gDefaultInitialStateLocation);
			if (mSizeLo == 0)
			{
				mSizeLo = gDefaultLoSize;
			}
			else
			{
				PrintInitialStateDimensions();
			}
		}
		else
		{
			mSizeLo = gDefaultLoSize;
			PrintDefaultInitialState();
		}
	}
	else
	{
		mSizeLo = (1ull << pow2Size) - 1;
	}

#if defined (DEBUG) || defined(_DEBUG)
	uint32_t flags = D3D11_CREATE_DEVICE_DEBUG;
#else
	uint32_t flags = 0;
#endif
     
	ThrowIfFailed(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, nullptr, 0, D3D11_SDK_VERSION, mDevice.GetAddressOf(), nullptr, mDeviceContext.GetAddressOf()));

	ThrowIfFailed(mDevice->SetExceptionMode(D3D11_RAISE_FLAG_DRIVER_INTERNAL_ERROR));

	mDownscaler          = std::make_unique<Downscaler>(mDevice.Get(), mSizeLo, mSizeLo, (uint32_t)gDownscaledSize, (uint32_t)gDownscaledSize);
	mStabilityCalculator = std::make_unique<StabilityCalculator>(mDevice.Get(), mSizeLo, mSizeLo, spawnPeriod);
}

FractalGen::~FractalGen()
{
}

void FractalGen::ComputeFractal(bool saveVideoFrames, bool bUseSmoothTransform, size_t enlonging)
{
	uint32_t solutionPeriod = SolutionPeriodFromSize(enlonging);

	int initTexInfo = mStabilityCalculator->InitTextures(mDevice.Get(), mDeviceContext.Get(), mSizeLo);
	if(initTexInfo & CUSTOM_CLICK_RULE)
	{
		PrintCustomClickRule();
	}
	else if(std::filesystem::exists(gDefaultClickRuleLocation))
	{
		PrintErrorClickRule();
	}

	for(uint32_t i = 0; i < solutionPeriod; i++)
	{
		mStabilityCalculator->StabilityNextStep(mDeviceContext.Get());

		PrintIntermediateState(i, solutionPeriod);
		if(saveVideoFrames)
		{
			std::string filename = IntermediateStateFilename(i, solutionPeriod);
			SaveFractalImage(filename, bUseSmoothTransform, true);
		}
	}

	bool eqState = mStabilityCalculator->CheckEquality(mDevice.Get(), mDeviceContext.Get());
	if(!eqState)
	{
		PrintEqualityState(eqState);
	}
}

void FractalGen::SaveFractalImage(const std::string& filename, bool useSmoothTransform, bool useDownscaling)
{
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
	
	PngSaver pngSaver;
	pngSaver.SavePngImage(filename, downscaledWidth, downscaledHeight, rowPitch, stabilityData);
	PrintFinishingWork();
}

uint32_t FractalGen::SolutionPeriodFromSize(size_t enlonging)
{
	//For any normal Lights Out game of size (2^n - 1) x (2^n - 1), the solution period is 2^(n - 1).  
	//For example, for the normal 127 x 127 Lights Out the period is 64, for the normal 255 x 255 Lights Out the period is 128 and so on.
	//However, for the custom click rules, spawn stability and many other things this formula doesn't work anymore. 
	//But since we need the default solution period anyway, part of it stays. To control the larger periods, I added enlonging multiplier.
	return enlonging * ((mSizeLo + 1) / 2);
}

uint32_t FractalGen::SizeLoFromImageSize(const std::string& imgFilename)
{
	size_t width  = 0;
	size_t height = 0;

	PngOpener opener;
	if(!opener || !opener.GetImageSize(imgFilename, width, height))
	{
		PrintErrorImageLoading();
		return 0;
	}

	if(width != height) //Only square images allowed
	{
		return 0;
	}

	if((width & (width + 1)) == 0) //Check if width is 2^k - 1
	{
		return width;
	}
	else
	{
		return 0;
	}
}

//----------------------------------------------------------------------------------------------------------------------------------------------

std::string FractalGen::IntermediateStateString(size_t stIndex, size_t stateCount)
{
	std::ostringstream namestr;
	namestr.fill('0');
	namestr.width((std::streamsize)log10f(stateCount) + 1);
	namestr << stIndex;

	return namestr.str();
}

std::string FractalGen::IntermediateStateFilename(size_t stIndex, size_t stateCount)
{
	return "DiffStabil\\Stabl" + IntermediateStateString(stIndex, stateCount) + ".png";
}

void FractalGen::PrintIntermediateState(size_t intermediateStr, size_t stateCount)
{
	std::cout << IntermediateStateString(intermediateStr, stateCount) << "/" << stateCount << "..." << std::endl;
}

void FractalGen::PrintEqualityState(bool equalityState)
{
	if(!equalityState)
	{
		std::cout << "Warning: oh no! The first and the final states are unequal" << std::endl;
	}
}

void FractalGen::PrintFinishingWork()
{
	std::cout << "Yaaay!" << std::endl;
}

void FractalGen::PrintErrorImageLoading()
{
	std::cout << "Incorrect initial state file. Acceptable files are .pngs with sizes (2^k - 1) x (2^k - 1)" << std::endl;
}

void FractalGen::PrintInitialStateDimensions()
{
	std::cout << "Found initial state! Size: " << mSizeLo << " x " << mSizeLo << std::endl;
}

void FractalGen::PrintDefaultInitialState()
{
	std::cout << "Image size not specified and no initial state found. Default 1024x1024 4 corners state will be used." << std::endl;
}

void FractalGen::PrintCustomClickRule()
{
	std::cout << "Custom click rule loaded! Warning: custom click rules may be slow." << std::endl;
}

void FractalGen::PrintErrorClickRule()
{
	std::cout << "Custom click rule loading error. Please ensure the click rule file dimensions are exactly 32x32." << std::endl;
}
