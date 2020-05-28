#pragma once

//http://lightstrout.com/index.php/2019/05/31/stability-fractal/

#include <d3d11.h>

#include <Windows.h>
#include <wrl/client.h>
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include "..\Util.hpp"

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

	void SetVideoFrameWidth(uint32_t width);   //Sets the width of downscaled video frames
	void SetVideoFrameHeight(uint32_t height); //Sets the height of downscaled video frames

	void SetSpawnPeriod(uint32_t spawn);
	void SetUseSmooth(bool smooth);

	void ChangeSize(uint32_t newWidth, uint32_t newHeight); //Change the board size while keeping the initial state centered

	void                  InitDefaultClickRule();                                   //Changes the click rule to the default "cross" one
	Utils::BoardLoadError LoadClickRuleFromFile(const std::wstring& clickRuleFile); //Loads a click rule from a file
	void                  EditClickRule(float normalizedX, float normalizedY);      //Toggles a click rule bit at (clickRuleWidth * normalizedX, clickRuleHeight * normalizedY) on/off

	void                  Init4CornersBoard(uint32_t width, uint32_t height); //Resets the system with the default "4 corners" initial board 
	void                  Init4SidesBoard(uint32_t width, uint32_t height);   //Resets the system with "4 sides" initial board
	void                  InitCenterBoard(uint32_t width, uint32_t height);   //Resets the system with "center" initial board
	Utils::BoardLoadError LoadBoardFromFile(const std::wstring& boardFile);   //Resets the system with the initial board loaded from file

	void ResetComputingParameters(); //Prepares all data for the simulation
	void Tick();                     //A single step of the simulation

	void SaveCurrentVideoFrame(const std::wstring& videoFrameFile); //Saves small image optimized for a video frame
	void SaveCurrentStep(const std::wstring& stabilityFile);        //Saves full image, without downscaling
	void SaveClickRule(const std::wstring& clickRuleFile);          //Saves click rule

	uint32_t GetLastFrameNumber()                         const; //Returns the number of the last frame
	uint32_t GetDefaultSolutionPeriod(uint32_t boardSize) const; //Returns the (fake) solution period (if boardSize is 2^p - 1, then this function retuns 2^(p-1))

	uint32_t GetWidth()  const; //Returns the width of the board
	uint32_t GetHeight() const; //Returns the height of the board

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

	uint32_t mVideoFrameWidth;
	uint32_t mVideoFrameHeight;

	uint32_t mSpawnPeriod;

	bool mbUseSmoothTransform;
};