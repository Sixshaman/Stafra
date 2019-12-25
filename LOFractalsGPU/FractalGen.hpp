#pragma once

//http://lightstrout.com/index.php/2019/05/31/stability-fractal/

#include <d3d11.h>

#include <Windows.h>
#include <wrl/client.h>
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include "Downscaler.hpp"
#include "StabilityCalculator.hpp"

class FractalGen
{
public:
	FractalGen(uint32_t pow2Size, uint32_t spawnPeriod);
	~FractalGen();

	void ComputeFractal(bool SaveVideoFrames, bool bUseSmoothTransform, size_t enlonging);
	void SaveFractalImage(const std::string& filename, bool useSmoothTransform, bool useDownscaling = false);

private:
	uint32_t SolutionPeriodFromSize(size_t enlonging);
	uint32_t SizeLoFromImageSize(const std::string& imgFilename);

	std::string IntermediateStateString(size_t stIndex, size_t stateCount);
	std::string IntermediateStateFilename(size_t stIndex, size_t stateCount);

private:
	void PrintIntermediateState(size_t intermediateStr, size_t stateCount);
	void PrintEqualityState(bool equalityState);
	void PrintFinishingWork();
	void PrintErrorImageLoading();
	void PrintInitialStateDimensions();
	void PrintDefaultInitialState();
	void PrintCustomClickRule();
	void PrintErrorClickRule();

private:
	Microsoft::WRL::ComPtr<ID3D11Device>        mDevice;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> mDeviceContext;

	std::unique_ptr<Downscaler>          mDownscaler;
	std::unique_ptr<StabilityCalculator> mStabilityCalculator;

	uint32_t mSizeLo;
	uint32_t mSpawnPeriod;
};