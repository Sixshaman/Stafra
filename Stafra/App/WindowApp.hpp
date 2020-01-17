#pragma once

#define OEMRESOURCE

#include <Windows.h>
#include <memory>
#include "Renderer.hpp"
#include "CommandLineArguments.hpp"
#include "..\Computing\FractalGen.hpp"

enum class PlayMode
{
	MODE_STOP,
	MODE_PAUSED,
	MODE_SINGLE_FRAME,
	MODE_CONTINUOUS_FRAMES
};

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

	std::wstring IntermediateStateString(uint32_t frameNumber) const;

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
	 
	PlayMode mPlayMode;

	float mNeedChangeClickRuleX;
	float mNeedChangeClickRuleY;

	bool mResizing;
	bool mNeedToReinitComputing;
	bool mSaveVideoFrames;
};