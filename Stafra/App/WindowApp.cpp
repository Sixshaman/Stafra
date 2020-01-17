#include "WindowApp.hpp"
#include <comdef.h>
#include <CommCtrl.h>
#include <algorithm>
#include <sstream>

#undef min
#undef max

namespace
{
	const int gPreviewAreaMinWidth  = 768;
	const int gPreviewAreaMinHeight = 768;

	const int gClickRuleImageWidth  = 32;
	const int gClickRuleImageHeight = 32;

	const int gClickRuleAreaWidth  = gClickRuleImageWidth  * 9;
	const int gClickRuleAreaHeight = gClickRuleImageHeight * 9;

	const int gMarginLeft = 5;
	const int gMarginTop  = 5;

	const int gMarginRight  = 5;
	const int gMarginBottom = 5;

	const int gSpacing = 5;

	const int gMinLeftSideWidth  = gPreviewAreaMinWidth;
	const int gMinLeftSideHeight = gPreviewAreaMinHeight;

	const int gMinRightSideWidth  = gClickRuleAreaWidth;
	const int gMinRightSideHeight = gClickRuleAreaHeight;
}

WindowApp::WindowApp(HINSTANCE hInstance, const CommandLineArguments& cmdArgs): mMainWindowHandle(nullptr), mPreviewAreaHandle(nullptr), mClickRuleAreaHandle(nullptr), 
                                                                                mRenderThreadHandle(nullptr), mRenderThreadRunning(false), mPlayMode(PlayMode::MODE_CONTINUOUS_FRAMES)
{
	InitializeCriticalSection(&mRenderThreadLock);

	CreateMainWindow(hInstance);
	CreateChildWindows(hInstance);

	mRenderer   = std::make_unique<Renderer>(mPreviewAreaHandle, mClickRuleAreaHandle);
	mFractalGen = std::make_unique<FractalGen>(mRenderer.get());

	LayoutChildWindows();
	UpdateRendererForPreview();

	ParseCmdArgs(cmdArgs);
	mNeedToReinitComputing = true;

	CreateBackgroundTaskThread();
}

WindowApp::~WindowApp()
{
	DeleteCriticalSection(&mRenderThreadLock);

	DestroyWindow(mPreviewAreaHandle);
	DestroyWindow(mClickRuleAreaHandle);
	DestroyWindow(mMainWindowHandle);
}

int WindowApp::Run()
{
	BOOL msgRes = TRUE;
	MSG  msg    = {0};
	while(msgRes)
	{
		msgRes = GetMessage(&msg, nullptr, 0, 0);
		if(msgRes > 0)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}

LRESULT CALLBACK WindowApp::AppProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	switch(message)
	{
	case WM_CREATE:
	{
		LPCREATESTRUCT createStruct = (LPCREATESTRUCT)(lparam);
		WindowApp*             that = (WindowApp*)(createStruct->lpCreateParams);

		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)(that));
		break;
	}
	case WM_CLOSE:
	{
		WindowApp* that = (WindowApp*)(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		if(that)
		{
			that->CloseBackgroundTaskThread();
		}

		PostQuitMessage(0);
		break;
	}
	case WM_PAINT:
	{
		WindowApp* that = (WindowApp*)(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		if(that && that->mRenderer)
		{
			that->mRenderer->NeedRedraw();
		}
		break;
	}
	case WM_KEYUP:
	{
		WindowApp* that = (WindowApp*)(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		switch (wparam)
		{
		case 'R':
			that->mNeedToReinitComputing = true;
			that->mRenderer->NeedRedraw();
			break;
		case 'P':
			if(that->mPlayMode == PlayMode::MODE_PAUSED)
			{
				that->mPlayMode = PlayMode::MODE_CONTINUOUS_FRAMES;
			}
			else
			{
				that->mPlayMode = PlayMode::MODE_PAUSED;
			}
			break;
		case 'N':
			if(that->mPlayMode == PlayMode::MODE_PAUSED)
			{
				that->mPlayMode = PlayMode::MODE_SINGLE_FRAME;
			}
			break;
		default:
			break;
		}
		break;
	}
	case WM_ENTERSIZEMOVE:
	{
		WindowApp* that = (WindowApp*)(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		if (that)
		{
			that->mResizing = true;
		}
		break;
	}
	case WM_EXITSIZEMOVE:
	{
		WindowApp* that = (WindowApp*)(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		if(that)
		{
			that->mResizing = false;
			that->UpdateRendererForPreview();
		}
		break;
	}
	case WM_SIZE:
	{
		WindowApp* that = (WindowApp*)(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		if (that && that->mRenderer)
		{
			if (wparam == SIZE_RESTORED)
			{
				that->LayoutChildWindows();
				if(!that->mResizing)
				{
					that->UpdateRendererForPreview();
				}
			}
			else if(wparam == SIZE_MAXIMIZED)
			{
				that->LayoutChildWindows();
				that->UpdateRendererForPreview();
			}
			else
			{
			}
		}
		break;
	}
	case WM_GETMINMAXINFO:
	{
		WindowApp* that = (WindowApp*)(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		if(that)
		{
			LPMINMAXINFO minmaxInfo = (LPMINMAXINFO)(lparam);
			minmaxInfo->ptMinTrackSize.x = that->mMinWindowWidth;
			minmaxInfo->ptMinTrackSize.y = that->mMinWindowHeight;
			return 0;
		}
		break;
	}
	default:
	{
		break;
	}
	}

	return DefWindowProc(hwnd, message, wparam, lparam);
}

DWORD WINAPI WindowApp::RenderThread(LPVOID lpParam)
{
	WindowApp* that = (WindowApp*)(lpParam);
	that->mRenderThreadRunning = true;

	while(that->mRenderThreadRunning)
	{
		EnterCriticalSection(&that->mRenderThreadLock);

		if(that->mNeedToReinitComputing)
		{
			that->mFractalGen->ResetComputingParameters(L"InitialState.png", L"ClickRule.png", L"Restriction.png");
			that->mNeedToReinitComputing = false;
		}

		if(that->mRenderer->GetNeedRedraw())
		{
			that->mRenderer->DrawPreview();
		}

		if(that->mRenderer->GetNeedRedrawClickRule())
		{
			that->mRenderer->DrawClickRule();
		}

		if(that->mPlayMode == PlayMode::MODE_SINGLE_FRAME || that->mPlayMode == PlayMode::MODE_CONTINUOUS_FRAMES)
		{
			that->mFractalGen->Tick();

			if(that->mPlayMode == PlayMode::MODE_SINGLE_FRAME)
			{
				that->mPlayMode = PlayMode::MODE_PAUSED;
			}

			if(that->mSaveVideoFrames)
			{
				std::wstring frameNumberStr = that->IntermediateStateString(that->mFractalGen->GetCurrentFrame());
				that->mFractalGen->SaveCurrentVideoFrame(L"DiffStabil\\Stabl" + frameNumberStr + L".png");
			}

			if(that->mFractalGen->GetCurrentFrame() == that->mFractalGen->GetSolutionPeriod())
			{
				that->mFractalGen->SaveCurrentStep(L"Stability.png");
				that->mPlayMode = PlayMode::MODE_PAUSED;
			}
		} 

		LeaveCriticalSection(&that->mRenderThreadLock);
	}

	return 0;
}

void WindowApp::CreateMainWindow(HINSTANCE hInstance)
{
	LPCSTR windowClassName = "StabilityFractalClass";
	
	WNDCLASSEX wc;
	wc.cbSize        = sizeof(WNDCLASSEX);
	wc.style         = CS_HREDRAW | CS_VREDRAW | CS_PARENTDC;                                                                               
	wc.lpfnWndProc   = WindowApp::AppProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = nullptr;
	wc.hIconSm       = nullptr;
	wc.hCursor       = (HCURSOR)LoadImage(nullptr, MAKEINTRESOURCE(OCR_NORMAL), IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED);;
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName  = nullptr;
	wc.lpszClassName = windowClassName;

	if(!RegisterClassEx(&wc))
	{
		throw new std::exception(_com_error(GetLastError()).ErrorMessage());
	}

	CalculateMinWindowSize();

	DWORD wndStyleEx = 0;
	DWORD wndStyle   = WS_OVERLAPPEDWINDOW;

	RECT clientRect;
	clientRect.left   = 100;
	clientRect.right  = clientRect.left + mMinWindowWidth;
	clientRect.top    = 100;
	clientRect.bottom = clientRect.top + mMinWindowHeight;
	AdjustWindowRectEx(&clientRect, wndStyle, FALSE, wndStyleEx);

	mMinWindowWidth  = clientRect.right  - clientRect.left;
	mMinWindowHeight = clientRect.bottom - clientRect.top;

	mMainWindowHandle = CreateWindowEx(wndStyleEx, windowClassName, "Stability fractal", wndStyle, clientRect.left, clientRect.top, mMinWindowWidth, mMinWindowHeight, nullptr, nullptr, hInstance, this);

	UpdateWindow(mMainWindowHandle);
	ShowWindow(mMainWindowHandle, SW_SHOW);
}

void WindowApp::CreateChildWindows(HINSTANCE hInstance)
{
	mPreviewAreaHandle   = CreateWindowEx(0, WC_STATIC, "Preview",   WS_CHILD, 0, 0, gPreviewAreaMinWidth, gPreviewAreaMinHeight, mMainWindowHandle, nullptr, hInstance, nullptr);
	mClickRuleAreaHandle = CreateWindowEx(0, WC_STATIC, "ClickRule", WS_CHILD, 0, 0, gClickRuleAreaWidth,  gClickRuleAreaHeight,  mMainWindowHandle, nullptr, hInstance, nullptr);

	UpdateWindow(mPreviewAreaHandle);
	ShowWindow(mPreviewAreaHandle, SW_SHOW);

	UpdateWindow(mClickRuleAreaHandle);
	ShowWindow(mClickRuleAreaHandle, SW_SHOW);
}

void WindowApp::CreateBackgroundTaskThread()
{
	mRenderThreadHandle = CreateThread(nullptr, 0, RenderThread, this, 0, nullptr);
}

void WindowApp::CloseBackgroundTaskThread()
{
	EnterCriticalSection(&mRenderThreadLock);
	mRenderThreadRunning = false;
	LeaveCriticalSection(&mRenderThreadLock);

	WaitForSingleObject(mRenderThreadHandle, 3000);
	CloseHandle(mRenderThreadHandle);
}

void WindowApp::LayoutChildWindows()
{
	RECT wndRect;
	GetClientRect(mMainWindowHandle, &wndRect);

	int wndWidth  = wndRect.right  - wndRect.left;
	int wndHeight = wndRect.bottom - wndRect.top;

	int rightSideWidth  = gClickRuleAreaWidth;
	int rightSideHeight = gClickRuleAreaHeight;

	int previewHeight = wndHeight - gMarginTop - gMarginBottom;
	int previewWidth  = wndWidth  - rightSideWidth - gSpacing - gMarginLeft - gMarginRight;

	int sizePreview = std::min(previewWidth, previewHeight);

	SetWindowPos(mPreviewAreaHandle,   HWND_TOP, gMarginLeft,                          gMarginTop,                                      sizePreview,         sizePreview,          0);
	SetWindowPos(mClickRuleAreaHandle, HWND_TOP, gMarginLeft + sizePreview + gSpacing, gMarginTop + sizePreview - gClickRuleAreaHeight, gClickRuleAreaWidth, gClickRuleAreaHeight, 0);
}

void WindowApp::CalculateMinWindowSize()
{
	mMinWindowWidth  = gMarginLeft + gMinLeftSideWidth + gSpacing + gMinRightSideWidth + gMarginRight;
	mMinWindowHeight = gMarginTop  + std::max(gMinLeftSideHeight, gMinRightSideHeight) + gMarginBottom;
}

void WindowApp::UpdateRendererForPreview()
{
	EnterCriticalSection(&mRenderThreadLock);

	RECT previewAreaRect;
	GetClientRect(mPreviewAreaHandle, &previewAreaRect);

	int previewWidth  = previewAreaRect.right - previewAreaRect.left;
	int previewHeight = previewAreaRect.bottom - previewAreaRect.top;
	mRenderer->ResizePreviewArea(previewWidth, previewHeight);

	LeaveCriticalSection(&mRenderThreadLock);
}

void WindowApp::ParseCmdArgs(const CommandLineArguments& cmdArgs)
{
	uint32_t powSize   = cmdArgs.PowSize();
	uint32_t boardSize = (1 << powSize) - 1;

	mFractalGen->SetDefaultBoardWidth(boardSize);
	mFractalGen->SetDefaultBoardHeight(boardSize);

	mFractalGen->SetVideoFrameWidth(1024);
	mFractalGen->SetVideoFrameHeight(1024);

	mFractalGen->SetSpawnPeriod(cmdArgs.SpawnPeriod());
	mFractalGen->SetUseSmooth(cmdArgs.SmoothTransform());

	mSaveVideoFrames = cmdArgs.SaveVideoFrames();
}

std::wstring WindowApp::IntermediateStateString(uint32_t frameNumber) const
{
	std::wostringstream namestr;
	namestr.fill('0');
	namestr.width(7); //Good enough
	namestr << frameNumber;

	return namestr.str();
}