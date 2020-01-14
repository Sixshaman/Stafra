#pragma once

#define OEMRESOURCE

#include <Windows.h>
#include <memory>
#include "Renderer.hpp"
#include "CommandLineArguments.hpp"
#include "..\Computing\FractalGen.hpp"

class WindowApp
{
public:
	WindowApp(HINSTANCE hInstance, const CommandLineArguments& cmdArgs);
	~WindowApp();

	int Run();

private:
	void CreateMainWindow(HINSTANCE hInstance);
	void CreateChildWindows(HINSTANCE hInstance);

	void CreateBackgroundTaskThread();
	void CloseBackgroundTaskThread();

	void LayoutChildWindows();
	void CalculateMinWindowSize();

	void UpdateRendererForPreview();

	void ParseCmdArgs(const CommandLineArguments& cmdArgs);

	static LRESULT CALLBACK AppProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

	static DWORD WINAPI RenderThread(LPVOID lpParam);

private:
	std::unique_ptr<FractalGen> mFractalGen;
	std::unique_ptr<Renderer>   mRenderer;

	HWND mMainWindowHandle;
	HWND mPreviewAreaHandle;
	HWND mClickRuleAreaHandle;

	int mMinWindowWidth;
	int mMinWindowHeight;

	HANDLE           mRenderThreadHandle;
	CRITICAL_SECTION mRenderThreadLock;
	bool             mRenderThreadRunning;
	 
	bool mResizing;
	bool mPaused;
	bool mNeedToCalculateNextFrame;
	bool mNeedToReinitComputing;
};