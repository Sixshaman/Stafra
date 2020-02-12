#include "StafraApp.hpp"
#include <sstream>
#include "..\Util.hpp"

StafraApp::StafraApp(): mSaveVideoFrames(false), mFinalFrameNumber(1)
{
	ThrowIfFailed(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)); //Shell functions (file save/open dialogs) don't like multithreaded environment, so use COINIT_APARTMENTTHREADED instead of COINIT_MULTITHREADED
}

StafraApp::~StafraApp()
{
}

void StafraApp::Init(const CommandLineArguments& cmdArgs)
{
	mFractalGen = std::make_unique<FractalGen>(mRenderer.get());
	
	ParseCmdArgs(cmdArgs);
	mFractalGen->ResetComputingParameters();

	if(mFinalFrameNumber == 0)
	{
		mFinalFrameNumber = mFractalGen->GetSolutionPeriod();
	}
}

std::wstring StafraApp::IntermediateStateString(uint32_t frameNumber) const
{
	const int zerosPadding = log10f((float)mFinalFrameNumber) + 1;

	std::wostringstream namestr;
	namestr.fill('0');
	namestr.width(zerosPadding);
	namestr << frameNumber;

	return namestr.str();
}

void StafraApp::ParseCmdArgs(const CommandLineArguments& cmdArgs)
{
	uint32_t powSize   = cmdArgs.PowSize();
	uint32_t boardSize = (1 << powSize) - 1;

	mFractalGen->SetDefaultBoardWidth(boardSize);
	mFractalGen->SetDefaultBoardHeight(boardSize);

	mFractalGen->SetVideoFrameWidth(1024);
	mFractalGen->SetVideoFrameHeight(1024);

	mFractalGen->SetSpawnPeriod(cmdArgs.SpawnPeriod());
	mFractalGen->SetUseSmooth(cmdArgs.SmoothTransform());

	mFinalFrameNumber = cmdArgs.FinalFrame();

	mSaveVideoFrames = cmdArgs.SaveVideoFrames();
	if(mSaveVideoFrames)
	{
		CreateDirectory(L"DiffStabil", nullptr);
	}

	if(!mFractalGen->LoadClickRuleFromFile(L"ClickRule.png"))
	{
		mFractalGen->InitDefaultClickRule();
	}

	if(!mFractalGen->LoadBoardFromFile(L"InitialBoard.png"))
	{
		mFractalGen->Init4CornersBoard();
	}
}