#include "..\Util.hpp"
#include "ConsoleApp.hpp"
#include "ConsoleLogger.hpp"
#include <iostream>

ConsoleApp::ConsoleApp(const CommandLineArguments& cmdArgs)
{
	Init(cmdArgs);
}

ConsoleApp::~ConsoleApp()
{
}

void ConsoleApp::ComputeFractal()
{
	while(true)
	{
		std::wstring currFrameNumberStr = IntermediateStateString(mFractalGen->GetLastFrameNumber() + 1);
		mLogger->WriteToLog(L"Computing the frame " + currFrameNumberStr + L"/" + std::to_wstring(mFinalFrameNumber) + L"...");

		mFractalGen->Tick();
		if(mRenderer->ConsumeNeedRedraw())
		{
			mRenderer->DrawPreview(); //Flushes the device context
		}

		if(mSaveVideoFrames)
		{
			std::wstring videoFrameName = L"DiffStabil\\Stabl" + currFrameNumberStr + L".png";
			mLogger->WriteToLog(L"Saving the video frame " + videoFrameName + L"...");

			mFractalGen->SaveCurrentVideoFrame(videoFrameName);
		}

		if(mFractalGen->GetLastFrameNumber() == mFinalFrameNumber)
		{
			std::wstring stablFilename = L"Stability.png";
			mLogger->WriteToLog(L"Saving the stability state " + stablFilename + L"...");

			mFractalGen->SaveCurrentStep(stablFilename);
			break;
		}
	}
}

void ConsoleApp::Init(const CommandLineArguments& cmdArgs)
{
	mRenderer = std::make_unique<Renderer>();
	mLogger   = std::make_unique<ConsoleLogger>();
	StafraApp::Init(cmdArgs);
}