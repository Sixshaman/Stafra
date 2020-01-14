#pragma once

//http://lightstrout.com/index.php/2019/05/31/stability-fractal/

#include <d3d11.h>

#include <Windows.h>
#include <wrl/client.h>
#include <cstdint>
#include <string>
#include <vector>
#include <memory>

class Renderer;

class StabilityCalculator;
class EqualityChecker;
class Downscaler;
class FinalTransformer;

class InitialStates;
class BoardLoader;
class BoardSaver;

class FractalGen
{
public:
	FractalGen(Renderer* renderer);
	~FractalGen();

	void SetDefaultBoardWidth(uint32_t width);
	void SetDefaultBoardHeight(uint32_t height);

	void SetVideoFrameWidth(uint32_t width);
	void SetVideoFrameHeight(uint32_t height);

	void DoComputingPreparations(const std::wstring& initialBoardFile, const std::wstring& clickRuleFile, const std::wstring& restrictionFile);
	void Tick(uint32_t spawnPeriod, const std::wstring& videoFrameFile);
	void FinishComputing(uint32_t spawnPeriod, const std::wstring& stabilityFile);

	uint32_t GetSolutionPeriod() const;

private:
	Renderer* mRenderer; //Non-owning observer pointer

	std::unique_ptr<StabilityCalculator> mStabilityCalculator;

	std::unique_ptr<Downscaler>       mDownscaler;
	std::unique_ptr<FinalTransformer> mFinalTransformer;
	std::unique_ptr<EqualityChecker>  mEqualityChecker;

	std::unique_ptr<InitialStates> mInitialStates;
	std::unique_ptr<BoardLoader>   mBoardLoader;
	std::unique_ptr<BoardSaver>    mBoardSaver;

	uint32_t mDefaultBoardWidth;
	uint32_t mDefaultBoardHeight;

	uint32_t mVideoFrameWidth;
	uint32_t mVideoFrameHeight;
};