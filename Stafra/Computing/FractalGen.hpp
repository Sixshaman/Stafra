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

class Boards;
class ClickRules;
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

	void SetSpawnPeriod(uint32_t spawn);
	void SetUseSmooth(bool smooth);

	void InitDefaultClickRule();
	bool LoadClickRuleFromFile(const std::wstring& clickRuleFile);
	void EditClickRule(float normalizedX, float normalizedY);

	void Init4CornersBoard();
	void Init4SidesBoard();
	bool LoadBoardFromFile(const std::wstring& boardFile);

	void ResetComputingParameters();
	void Tick();

	void SaveCurrentVideoFrame(const std::wstring& videoFrameFile); //Saves small image optimized for a video frame
	void SaveCurrentStep(const std::wstring& stabilityFile);        //Saves full image, without downscaling
	void SaveClickRule(const std::wstring& clickRuleFile);          //Saves click rule

	uint32_t GetLastFrameNumber()   const;
	uint32_t GetSolutionPeriod() const;

private:
	Renderer* mRenderer; //Non-owning observer pointer

	std::unique_ptr<StabilityCalculator> mStabilityCalculator;

	std::unique_ptr<Downscaler>       mDownscaler;
	std::unique_ptr<FinalTransformer> mFinalTransformer;
	std::unique_ptr<EqualityChecker>  mEqualityChecker;

	std::unique_ptr<ClickRules> mClickRules;
	std::unique_ptr<Boards>     mBoards;

	std::unique_ptr<BoardLoader> mBoardLoader;
	std::unique_ptr<BoardSaver>  mBoardSaver;

	uint32_t mDefaultBoardWidth;
	uint32_t mDefaultBoardHeight;

	uint32_t mVideoFrameWidth;
	uint32_t mVideoFrameHeight;

	uint32_t mSpawnPeriod;

	bool mbUseSmoothTransform;
};