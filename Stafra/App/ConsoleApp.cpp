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
	mRenderer = std::make_unique<Renderer>();
	mLogger   = std::make_unique<ConsoleLogger>();
	StafraApp::Init(cmdArgs);
}