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
	mFractalGen->SetSpawnPeriod(mSpawnPeriod);
	mFractalGen->SetUseSmooth(mUseSmoothTransform);

	mLogger->WriteToLog(L"Spawn period: " + std::to_wstring(mSpawnPeriod));

	while(mFractalGen->GetLastFrameNumber() != mFinalFrameNumber)
	{
		ComputeFractalTick();
		if(mRenderer->ConsumeNeedRedraw())
		{
			mRenderer->DrawPreview(); //Flushes the device context
		}

		if(mSaveVideoFrames)
		{
			std::wstring frameNumberStr     = IntermediateStateString(mFractalGen->GetLastFrameNumber());
			std::wstring videoFrameFilename = L"DiffStabil\\Stabl" + frameNumberStr + L".png";

			SaveCurrentVideoFrame(videoFrameFilename);
		}
	}

	SaveStability(L"Stability.png");
}

void ConsoleApp::Init(const CommandLineArguments& cmdArgs)
{
	StafraApp::Init(cmdArgs);
}

void ConsoleApp::InitRenderer(const CommandLineArguments& args)
{
	mRenderer = std::make_unique<Renderer>(args.GpuIndex());
}

void ConsoleApp::InitLogger(const CommandLineArguments& args)
{
	mLogger = std::make_unique<ConsoleLogger>();
}