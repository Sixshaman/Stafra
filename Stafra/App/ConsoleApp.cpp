#include "..\Util.hpp"
#include "ConsoleApp.hpp"
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
		mFractalGen->Tick();

		std::wstring frameNumberStr = IntermediateStateString(mFractalGen->GetCurrentFrame());
		std::wcout << frameNumberStr << std::endl;

		if(mSaveVideoFrames)
		{
			mFractalGen->SaveCurrentVideoFrame(L"DiffStabil\\Stabl" + frameNumberStr + L".png");
		}

		if(mFractalGen->GetCurrentFrame() == mFractalGen->GetSolutionPeriod())
		{
			mFractalGen->SaveCurrentStep(L"Stability.png");
			break;
		}
	}
}

void ConsoleApp::Init(const CommandLineArguments& cmdArgs)
{
	mRenderer = std::make_unique<Renderer>();
	StafraApp::Init(cmdArgs);
}