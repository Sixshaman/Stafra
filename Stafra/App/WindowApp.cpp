#include "WindowApp.hpp"
#include <comdef.h>
#include <CommCtrl.h>
#include <algorithm>
#include <sstream>
#include <Windowsx.h>
#include "FileDialog.hpp"
#include "..\Util.hpp"

#undef min
#undef max

#define MENU_SAVE_CLICK_RULE  1001
#define MENU_OPEN_CLICK_RULE  1002
#define MENU_OPEN_BOARD       1003
#define MENU_SAVE_BOARD       1004
#define MENU_OPEN_RESTRICTION 1005

#define RENDER_THREAD_EXIT            WM_APP + 1
#define RENDER_THREAD_REINIT          WM_APP + 2
#define RENDER_THREAD_CLICK_RULE      WM_APP + 3
#define RENDER_THREAD_LOAD_CLICK_RULE WM_APP + 4
#define RENDER_THREAD_SAVE_CLICK_RULE WM_APP + 5

namespace
{
	const int gMarginLeft = 5;
	const int gMarginTop = 5;

	const int gMarginRight = 5;
	const int gMarginBottom = 5;

	const int gSpacing = 5;

	const int gPreviewAreaMinWidth  = 768;
	const int gPreviewAreaMinHeight = 768;

	const int gClickRuleImageWidth  = 32;
	const int gClickRuleImageHeight = 32;

	const int gClickRuleAreaWidth  = gClickRuleImageWidth  * 9;
	const int gClickRuleAreaHeight = gClickRuleImageHeight * 9;

	const int gMinLogAreaWidth  = gClickRuleAreaWidth * 3;
	const int gMinLogAreaHeight = gPreviewAreaMinHeight - gClickRuleAreaHeight - gSpacing;

	const int gMinLeftSideWidth  = gPreviewAreaMinWidth;
	const int gMinLeftSideHeight = gPreviewAreaMinHeight;

	const int gMinRightSideWidth  = gMinLogAreaWidth;
	const int gMinRightSideHeight = gClickRuleAreaHeight + gSpacing + gMinLogAreaHeight;
}

WindowApp::WindowApp(HINSTANCE hInstance, const CommandLineArguments& cmdArgs): mMainWindowHandle(nullptr), mPreviewAreaHandle(nullptr), mClickRuleAreaHandle(nullptr), 
                                                                                mRenderThreadHandle(nullptr), mCreateRenderThreadEvent(nullptr), mPlayMode(PlayMode::MODE_CONTINUOUS_FRAMES)
{
	ThrowIfFailed(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)); //Shell functions (file save/open dialogs) don't like multithreaded environment, so use COINIT_APARTMENTTHREADED instead of COINIT_MULTITHREADED

	InitializeCriticalSection(&mRenderThreadLock);

	CreateMainWindow(hInstance);
	CreateChildWindows(hInstance);

	mRenderer   = std::make_unique<Renderer>(mPreviewAreaHandle, mClickRuleAreaHandle);
	mFractalGen = std::make_unique<FractalGen>(mRenderer.get());

	LayoutChildWindows();
	UpdateRendererForPreview();

	ParseCmdArgs(cmdArgs);
	mFractalGen->ResetComputingParameters(L"InitialState.png", L"Restriction.png");

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
	if(!mRenderer) //Still in the initialization process
	{
		return DefWindowProc(hwnd, message, wparam, lparam);
	}

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
		CloseBackgroundTaskThread();
		PostQuitMessage(0);
		break;
	}
	case WM_PAINT:
	{
		mRenderer->NeedRedraw();
		break;
	}
	case WM_KEYUP:
	{
		OnHotkey(wparam);
		break;
	}
	case WM_ENTERSIZEMOVE:
	{
		mResizing = true;
		break;
	}
	case WM_EXITSIZEMOVE:
	{
		mResizing = false;
		UpdateRendererForPreview();
		break;
	}
	case WM_COMMAND:
	{
		if(HIWORD(wparam) == 0)
		{
			return OnMenuItem(LOWORD(wparam));
		}
		break;
	}
	case WM_LBUTTONDOWN:
	{
		int xClick = GET_X_LPARAM(lparam);
		int yClick = GET_Y_LPARAM(lparam);

		RECT clickRuleRect;
		GetWindowRect(mClickRuleAreaHandle, &clickRuleRect);

		POINT pt;
		pt.x = xClick;
		pt.y = yClick;
		ClientToScreen(mMainWindowHandle, &pt);

		if(PtInRect(&clickRuleRect, pt))
		{
			if(mPlayMode == PlayMode::MODE_STOP)
			{
				LPARAM lparamThread = MAKELPARAM(pt.x, pt.y);
				PostThreadMessage(mRenderThreadID, RENDER_THREAD_CLICK_RULE, 0, lparamThread);
			}
		}
		return 0;
	}
	case WM_RBUTTONDOWN:
	{
		int xClick = GET_X_LPARAM(lparam);
		int yClick = GET_Y_LPARAM(lparam);

		RECT clickRuleRect;
		GetWindowRect(mClickRuleAreaHandle, &clickRuleRect);

		RECT previewRect;
		GetWindowRect(mPreviewAreaHandle, &previewRect);

		POINT pt;
		pt.x = xClick;
		pt.y = yClick;
		ClientToScreen(mMainWindowHandle, &pt);

		if(PtInRect(&clickRuleRect, pt))
		{
			UINT clickRuleMenuFlags = MF_BYCOMMAND | MF_STRING;
			if(mPlayMode != PlayMode::MODE_STOP)
			{
				clickRuleMenuFlags = clickRuleMenuFlags | MF_DISABLED;
			}

			HMENU popupMenu = CreatePopupMenu();
			InsertMenu(popupMenu, 0, clickRuleMenuFlags, MENU_OPEN_CLICK_RULE, L"Open click rule...");
			InsertMenu(popupMenu, 0, clickRuleMenuFlags, MENU_SAVE_CLICK_RULE, L"Save click rule...");
			TrackPopupMenu(popupMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON | TPM_NOANIMATION, pt.x, pt.y, 0, mMainWindowHandle, nullptr);
		}
		else if(PtInRect(&previewRect, pt))
		{
			UINT boardLoadMenuFlags = MF_BYCOMMAND | MF_STRING;
			UINT boardSaveMenuFlags = MF_BYCOMMAND | MF_STRING;
			if(mPlayMode != PlayMode::MODE_STOP)
			{
				boardLoadMenuFlags = boardLoadMenuFlags | MF_DISABLED;
				if(mPlayMode != PlayMode::MODE_PAUSED)
				{
					boardSaveMenuFlags = boardSaveMenuFlags | MF_DISABLED;
				}
			}

			HMENU popupMenu = CreatePopupMenu();
			InsertMenu(popupMenu, 0, boardLoadMenuFlags, MENU_OPEN_BOARD, L"Open board...");
			InsertMenu(popupMenu, 0, boardSaveMenuFlags, MENU_SAVE_BOARD, L"Save stability...");
			TrackPopupMenu(popupMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON | TPM_NOANIMATION, pt.x, pt.y, 0, mMainWindowHandle, nullptr);
		}
		return 0;
	}
	case WM_SIZE:
	{
		if(wparam == SIZE_RESTORED)
		{
			LayoutChildWindows();
			if(!mResizing)
			{
				UpdateRendererForPreview();
			}
		}
		else if(wparam == SIZE_MAXIMIZED)
		{
			LayoutChildWindows();
			UpdateRendererForPreview();
		}
		else
		{
		}
		break;
	}
	case WM_GETMINMAXINFO:
	{
		LPMINMAXINFO minmaxInfo = (LPMINMAXINFO)(lparam);
		minmaxInfo->ptMinTrackSize.x = mMinWindowWidth;
		minmaxInfo->ptMinTrackSize.y = mMinWindowHeight;
		return 0;
	}
	default:
	{
		break;
	}
	}

	return DefWindowProc(hwnd, message, wparam, lparam);
}

DWORD WINAPI WindowApp::RenderThread()
{
	MSG threadMsg = {0};
	PeekMessage(&threadMsg, nullptr, WM_USER, WM_USER, PM_NOREMOVE); //This creates the thread message queue
	SetEvent(mCreateRenderThreadEvent);

	bool bThreadRunning = true;
	while(bThreadRunning)
	{
		EnterCriticalSection(&mRenderThreadLock);

		PeekMessage(&threadMsg, nullptr, WM_APP, 0xBFFF, PM_REMOVE);

		switch(threadMsg.message)
		{
		case RENDER_THREAD_EXIT:
		{
			bThreadRunning = false;
			break;
		}
		case RENDER_THREAD_REINIT:
		{
			mFractalGen->ResetComputingParameters(L"InitialState.png", L"Restriction.png");
			break;
		}
		case RENDER_THREAD_CLICK_RULE:
		{
			int x = LOWORD(threadMsg.lParam);
			int y = HIWORD(threadMsg.lParam);

			RECT clickRuleRect;
			GetWindowRect(mClickRuleAreaHandle, &clickRuleRect);

			int xClickRule = x - clickRuleRect.left;
			int yClickRule = y - clickRuleRect.top;

			mFractalGen->EditClickRule((float)xClickRule / (float)(clickRuleRect.right - clickRuleRect.left), (float)yClickRule / (float)(clickRuleRect.bottom - clickRuleRect.top));
			break;
		}
		case RENDER_THREAD_SAVE_CLICK_RULE:
		{
			//Catch the pointer
			std::unique_ptr<std::wstring> clickRuleFilenamePtr(reinterpret_cast<std::wstring*>(threadMsg.lParam));
			mFractalGen->SaveClickRule(*clickRuleFilenamePtr);
			break;
		}
		case RENDER_THREAD_LOAD_CLICK_RULE:
		{
			//Catch the pointer
			std::unique_ptr<std::wstring> clickRuleFilenamePtr(reinterpret_cast<std::wstring*>(threadMsg.lParam));
			mFractalGen->LoadClickRuleFromFile(*clickRuleFilenamePtr);
			break;
		}
		default:
			break;
		}

		if(mRenderer->GetNeedRedraw())
		{
			mRenderer->DrawPreview();
		}

		if(mRenderer->GetNeedRedrawClickRule())
		{
			mRenderer->DrawClickRule();
		}

		if(mPlayMode == PlayMode::MODE_SINGLE_FRAME || mPlayMode == PlayMode::MODE_CONTINUOUS_FRAMES)
		{
			mFractalGen->Tick();

			if(mPlayMode == PlayMode::MODE_SINGLE_FRAME)
			{
				mPlayMode = PlayMode::MODE_PAUSED;
			}

			if(mSaveVideoFrames)
			{
				std::wstring frameNumberStr = IntermediateStateString(mFractalGen->GetCurrentFrame());
				mFractalGen->SaveCurrentVideoFrame(L"DiffStabil\\Stabl" + frameNumberStr + L".png");
			}

			if(mFractalGen->GetCurrentFrame() == mFractalGen->GetSolutionPeriod())
			{
				mFractalGen->SaveCurrentStep(L"Stability.png");
			}
		}

		LeaveCriticalSection(&mRenderThreadLock);
	}

	return 0;
}

void WindowApp::CreateMainWindow(HINSTANCE hInstance)
{
	LPCWSTR windowClassName = L"StabilityFractalClass";
	
	WNDCLASSEX wc;
	wc.cbSize        = sizeof(WNDCLASSEX);
	wc.style         = CS_HREDRAW | CS_VREDRAW | CS_PARENTDC;                                                                               
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = nullptr;
	wc.hIconSm       = nullptr;
	wc.hCursor       = (HCURSOR)LoadImage(nullptr, MAKEINTRESOURCE(OCR_NORMAL), IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName  = nullptr;
	wc.lpszClassName = windowClassName;
	wc.lpfnWndProc   = [](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT CALLBACK
	{
		if(message == WM_CREATE)
		{
			LPCREATESTRUCT createStruct = (LPCREATESTRUCT)(lparam);
			WindowApp* that = (WindowApp*)(createStruct->lpCreateParams);

			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)(that));
		}
		else
		{
			WindowApp* that = (WindowApp*)(GetWindowLongPtr(hwnd, GWLP_USERDATA));
			if(that)
			{
				return that->AppProc(hwnd, message, wparam, lparam);
			}
		}
		return DefWindowProc(hwnd, message, wparam, lparam);
	};

	if(!RegisterClassEx(&wc))
	{
		ThrowIfFailed(GetLastError());
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

	mMainWindowHandle = CreateWindowEx(wndStyleEx, windowClassName, L"Stability fractal", wndStyle, clientRect.left, clientRect.top, mMinWindowWidth, mMinWindowHeight, nullptr, nullptr, hInstance, this);

	UpdateWindow(mMainWindowHandle);
	ShowWindow(mMainWindowHandle, SW_SHOW);
}

void WindowApp::CreateChildWindows(HINSTANCE hInstance)
{
	mPreviewAreaHandle   = CreateWindowEx(0, WC_STATIC, L"Preview",   WS_CHILD, 0, 0, gPreviewAreaMinWidth, gPreviewAreaMinHeight, mMainWindowHandle, nullptr, hInstance, nullptr);
	mClickRuleAreaHandle = CreateWindowEx(0, WC_STATIC, L"ClickRule", WS_CHILD, 0, 0, gClickRuleAreaWidth,  gClickRuleAreaHeight,  mMainWindowHandle, nullptr, hInstance, nullptr);

	UpdateWindow(mPreviewAreaHandle);
	ShowWindow(mPreviewAreaHandle, SW_SHOW);

	UpdateWindow(mClickRuleAreaHandle);
	ShowWindow(mClickRuleAreaHandle, SW_SHOW);
}

void WindowApp::CreateBackgroundTaskThread()
{
	auto RenderThreadFunc = [](LPVOID lpParam) -> DWORD WINAPI
	{
		WindowApp* that = (WindowApp*)(lpParam);
		return that->RenderThread();
	};

	mCreateRenderThreadEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	mRenderThreadHandle = CreateThread(nullptr, 0, RenderThreadFunc, this, 0, nullptr);
	WaitForSingleObject(mCreateRenderThreadEvent, INFINITE);

	mRenderThreadID = GetThreadId(mRenderThreadHandle);
}

void WindowApp::CloseBackgroundTaskThread()
{
	PostThreadMessage(mRenderThreadID, RENDER_THREAD_EXIT, 0, 0);
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

	if(!mFractalGen->LoadClickRuleFromFile(L"ClickRule.png"))
	{
		mFractalGen->InitDefaultClickRule();
	}
}

int WindowApp::OnMenuItem(uint32_t menuItem)
{
	switch (menuItem)
	{
	case MENU_SAVE_CLICK_RULE:
	{
		std::wstring clickRuleFilename = L"ClickRule.png";

		FileDialog fileDialog;
		if(fileDialog.GetFilenameToSave(mMainWindowHandle, clickRuleFilename))
		{
			std::unique_ptr<std::wstring> clickRuleFilenamePtr = std::make_unique<std::wstring>(clickRuleFilename);
			PostThreadMessage(mRenderThreadID, RENDER_THREAD_SAVE_CLICK_RULE, 0, reinterpret_cast<LPARAM>(clickRuleFilenamePtr.release()));
		}
		break;
	}
	case MENU_OPEN_CLICK_RULE:
	{
		std::wstring clickRuleFilename = L"ClickRule.png";

		FileDialog fileDialog;
		if(fileDialog.GetFilenameToOpen(mMainWindowHandle, clickRuleFilename))
		{
			std::unique_ptr<std::wstring> clickRuleFilenamePtr = std::make_unique<std::wstring>(clickRuleFilename);
			PostThreadMessage(mRenderThreadID, RENDER_THREAD_LOAD_CLICK_RULE, 0, reinterpret_cast<LPARAM>(clickRuleFilenamePtr.release()));
		}
		break;
	}
	default:
	{
		break;
	}
	}

	return MNC_CLOSE;
}

int WindowApp::OnHotkey(uint32_t hotkey)
{
	switch (hotkey)
	{
	case 'R':
	{
		mPlayMode = PlayMode::MODE_CONTINUOUS_FRAMES;
		PostThreadMessage(mRenderThreadID, RENDER_THREAD_REINIT, 0, 0);
		mRenderer->NeedRedraw();
		break;
	}
	case 'P':
	{
		if(mPlayMode != PlayMode::MODE_STOP)
		{
			if(mPlayMode == PlayMode::MODE_PAUSED)
			{
				mPlayMode = PlayMode::MODE_CONTINUOUS_FRAMES;
			}
			else
			{
				mPlayMode = PlayMode::MODE_PAUSED;
			}
		}
		break;
	}
	case 'N':
	{
		if(mPlayMode == PlayMode::MODE_PAUSED)
		{
			mPlayMode = PlayMode::MODE_SINGLE_FRAME;
		}
		break;
	}
	case 'S':
	{
		if(mPlayMode == PlayMode::MODE_STOP)
		{
			PostThreadMessage(mRenderThreadID, RENDER_THREAD_REINIT, 0, 0);
			mPlayMode = PlayMode::MODE_CONTINUOUS_FRAMES;
		}
		else
		{
			mPlayMode = PlayMode::MODE_STOP;
		}
		break;
	}

	default:
	{
		break;
	}
	}

	return 0;
}

void WindowApp::Tick()
{
}

std::wstring WindowApp::IntermediateStateString(uint32_t frameNumber) const
{
	const int zerosPadding = 7; //Good enough

	std::wostringstream namestr;
	namestr.fill('0');
	namestr.width(zerosPadding);
	namestr << frameNumber;

	return namestr.str();
}