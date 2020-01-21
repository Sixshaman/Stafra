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

	int OnMenuItem(uint32_t menuItem);
	int OnHotkey(uint32_t hotkey);

	void Tick();

	LRESULT CALLBACK AppProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

	DWORD WINAPI RenderThread();

private:
	std::unique_ptr<FractalGen> mFractalGen;
	std::unique_ptr<Renderer>   mRenderer;

	HWND mMainWindowHandle;
	HWND mPreviewAreaHandle;
	HWND mClickRuleAreaHandle;

	DWORD  mRenderThreadID;
	HANDLE mRenderThreadHandle;
	HANDLE mCreateRenderThreadEvent;

	CRITICAL_SECTION mRenderThreadLock;

	int mMinWindowWidth;
	int mMinWindowHeight;
	 
	PlayMode mPlayMode;

	bool mResizing;
	bool mSaveVideoFrames;
};