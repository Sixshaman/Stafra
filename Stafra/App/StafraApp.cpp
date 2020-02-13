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

	if(!LoadClickRuleFromFile(L"ClickRule.png"))
	{
		InitDefaultClickRule();
	}

	if(!LoadBoardFromFile(L"InitialBoard.png"))
	{
		InitBoard4Corners();
	}
}

void StafraApp::ComputeFractalTick()
{
	std::wstring currFrameNumberStr = IntermediateStateString(mFractalGen->GetLastFrameNumber() + 1);
	mLogger->WriteToLog(L"Computing the frame " + currFrameNumberStr + L"/" + std::to_wstring(mFinalFrameNumber) + L"...");

	mFractalGen->Tick();
}

void StafraApp::SaveCurrentVideoFrame(const std::wstring& filename)
{
	mLogger->WriteToLog(L"Saving the video frame " + filename + L"...");
	mFractalGen->SaveCurrentVideoFrame(filename);
}

void StafraApp::SaveStability(const std::wstring& filename)
{
	mLogger->WriteToLog(L"Saving the stability state " + filename + L"...");
	mFractalGen->SaveCurrentStep(filename);
}

bool StafraApp::LoadBoardFromFile(const std::wstring& filename)
{
	mLogger->WriteToLog(L"Loading the board from " + filename + L"...");

	Utils::BoardLoadError loadErr = mFractalGen->LoadBoardFromFile(filename);
	switch (loadErr)
	{
	case Utils::BoardLoadError::ERROR_CANT_READ_FILE:
	{
		mLogger->WriteToLog(L"Cannot read file!");
		return false;
	}
	case Utils::BoardLoadError::ERROR_WRONG_SIZE:
	{
		mLogger->WriteToLog(L"Incorrect board dimensions! Acceptable dimensions: 1x1, 3x3, 7x7, 15x15, 31x31, 63x63, 127x127, 255x255, 511x511, 1023x1023, 2047x2047, 4095x4095, 8191x8191, 16383x16383.");
		return false;
	}
	case Utils::BoardLoadError::ERROR_INVALID_ARGUMENT:
	{
		mLogger->WriteToLog(L"Unknown error!");
		return false;
	}
	default:
	{
		return true;
	}
	}
}

void StafraApp::InitBoard4Corners()
{
	mLogger->WriteToLog(L"Initializing default state: 4 CORNERS...");
	mFractalGen->Init4CornersBoard();
}

bool StafraApp::LoadClickRuleFromFile(const std::wstring& filename)
{
	mLogger->WriteToLog(L"Loading the click rule from " + filename + L"...");

	Utils::BoardLoadError loadErr = mFractalGen->LoadClickRuleFromFile(filename);
	switch (loadErr)
	{
	case Utils::BoardLoadError::ERROR_CANT_READ_FILE:
	{
		mLogger->WriteToLog(L"Cannot read file!");
		return false;
	}
	case Utils::BoardLoadError::ERROR_WRONG_SIZE:
	{
		mLogger->WriteToLog(L"Incorrect click rule dimensions! Acceptable dimensions: 32x32.");
		return false;
	}
	case Utils::BoardLoadError::ERROR_INVALID_ARGUMENT:
	{
		mLogger->WriteToLog(L"Unknown error!");
		return false;
	}
	default:
	{
		return true;
	}
	}
}

void StafraApp::InitDefaultClickRule()
{
	mLogger->WriteToLog(L"Initializing default click rule...");
	mFractalGen->InitDefaultClickRule();
}