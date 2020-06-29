#pragma once

#define OEMRESOURCE

#include <Windows.h>
#include <memory>
#include "Renderer.hpp"
#include "CommandLineArguments.hpp"
#include "..\Computing\FractalGen.hpp"
#include "Logger.hpp"

enum class ResetBoardModeApp
{
	RESET_4_CORNERS,
	RESET_4_SIDES,
	RESET_CENTER,
	RESET_CUSTOM_IMAGE
};

class StafraApp
{
public:
	StafraApp();
	virtual ~StafraApp();

protected:
	void Init(const CommandLineArguments& cmdArgs);

	virtual void InitRenderer(const CommandLineArguments& args) = 0;
	virtual void InitLogger(const CommandLineArguments& args)   = 0;

	std::wstring IntermediateStateString(uint32_t frameNumber) const;

	void ComputeFractalTick();
	void SaveCurrentVideoFrame(const std::wstring& filename);
	void SaveStability(const std::wstring& filename);

	bool LoadBoardFromFile(const std::wstring& filename);
	void InitBoard(uint32_t boardWidth, uint32_t boardHeight);

	bool LoadClickRuleFromFile(const std::wstring& filename);
	void InitDefaultClickRule();

private:
	void ParseCmdArgs(const CommandLineArguments& cmdArgs);

protected:
	std::unique_ptr<FractalGen> mFractalGen;
	std::unique_ptr<Renderer>   mRenderer;
	std::unique_ptr<Logger>     mLogger;

	ResetBoardModeApp mResetMode;

	bool mSaveVideoFrames;

	uint32_t mFinalFrameNumber;
	uint32_t mSpawnPeriod;
};