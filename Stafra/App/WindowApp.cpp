#include "WindowApp.hpp"
#include <comdef.h>
#include <CommCtrl.h>
#include <algorithm>
#include <sstream>
#include <Windowsx.h>
#include "FileDialog.hpp"
#include "WindowLogger.hpp"
#include "WindowConstants.hpp"
#include "..\Util.hpp"

#undef min
#undef max

//TODO different reset modes
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

	const int gButtonWidth  = 48;
	const int gButtonHeight = 48;

	const int gMinLogAreaWidth  = gClickRuleAreaWidth + gButtonWidth * 4 + gSpacing * 5;
	const int gMinLogAreaHeight = gPreviewAreaMinHeight - gClickRuleAreaHeight - gSpacing;

	const int gMinLeftSideWidth  = gPreviewAreaMinWidth;
	const int gMinLeftSideHeight = gPreviewAreaMinHeight;

	const int gMinRightSideWidth  = gMinLogAreaWidth;
	const int gMinRightSideHeight = gClickRuleAreaHeight + gSpacing + gMinLogAreaHeight;

	const int gMinTrackBarWidth = gButtonWidth * 4 + gSpacing * 3;
	const int gTrackBarHeight   = 48;

	const int gCheckBoxHeight = 20;

	const int gInputLayoutHeight = 20;

	const int gInputTextBoxWidth  = 100;
	const int gInputLabelMinWidth = gMinTrackBarWidth - gInputTextBoxWidth - gSpacing;
}

WindowApp::WindowApp(HINSTANCE hInstance, const CommandLineArguments& cmdArgs): mMainWindowHandle(nullptr), mPreviewAreaHandle(nullptr), mClickRuleAreaHandle(nullptr), mLogAreaHandle(nullptr),
                                                                                mRenderThreadHandle(nullptr), mCreateRenderThreadEvent(nullptr), mPlayMode(PlayMode::MODE_CONTINUOUS_FRAMES),
	                                                                            mLoggerMessageCount(0)
{
	Init(hInstance, cmdArgs);

	LayoutChildWindows();
	UpdateRendererForPreview();

	CreateBackgroundTaskThreads();
}

WindowApp::~WindowApp()
{
	DeleteObject(mLogAreaFont);
	DeleteObject(mButtonsFont);

	DestroyWindow(mLogAreaHandle);
	DestroyWindow(mPreviewAreaHandle);
	DestroyWindow(mClickRuleAreaHandle);
	DestroyWindow(mMainWindowHandle);

	DestroyWindow(mButtonNextFrame);
	DestroyWindow(mButtonPausePlay);
	DestroyWindow(mButtonReset);
	DestroyWindow(mButtonStop);

	DestroyWindow(mSizeTrackbar);
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

void WindowApp::Init(HINSTANCE hInstance, const CommandLineArguments& cmdArgs)
{
	CreateMainWindow(hInstance);
	CreateChildWindows(hInstance);

	StafraApp::Init(cmdArgs);

	std::wstring wndTitle = L"Stability fractal " + std::to_wstring(mFractalGen->GetWidth()) + L"x" + std::to_wstring(mFractalGen->GetHeight());
	SetWindowText(mMainWindowHandle, wndTitle.c_str());

	int psize = (int)log2f((mFractalGen->GetWidth() + 1));
	SendMessage(mSizeTrackbar, TBM_SETPOS, TRUE, psize);

	if(cmdArgs.SaveVideoFrames())
	{
		SendMessage(mVideoFramesCheckBox, BM_SETCHECK, BST_CHECKED, 0);
	}

	std::wstring finalFrameStr = std::to_wstring(mFinalFrameNumber);
	SendMessage(mLastFrameTextBox, WM_SETTEXT, 0, (LPARAM)finalFrameStr.c_str());

	std::wstring spawnStr = std::to_wstring(mSpawnPeriod);
	SendMessage(mSpawnTextBox, WM_SETTEXT, 0, (LPARAM)spawnStr.c_str());

	if(cmdArgs.SmoothTransform())
	{
		SendMessage(mSmoothCheckBox, BM_SETCHECK, BST_CHECKED, 0);
	}
}

void WindowApp::InitRenderer(const CommandLineArguments& args)
{
	mRenderer = std::make_unique<DisplayRenderer>(args.GpuIndex(), mPreviewAreaHandle, mClickRuleAreaHandle);
}

void WindowApp::InitLogger(const CommandLineArguments& args)
{
	mLogger = std::make_unique<WindowLogger>(mMainWindowHandle);
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
		CloseBackgroundTaskThreads();
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
		OnHotkey((uint32_t)wparam);
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
	case WM_COMMAND: //Button, textbox, etc. events
	{
		if(lparam == 0)
		{
			return OnMenuItem(LOWORD(wparam));
		}
		else
		{
			HWND itmeHandle = (HWND)lparam;
			if(itmeHandle == mButtonReset)
			{
				OnCommandReset();
			}
			else if(itmeHandle == mButtonPausePlay)
			{
				OnCommandPause();
			}
			else if(itmeHandle == mButtonStop)
			{
				OnCommandStop();
			}
			else if(itmeHandle == mButtonNextFrame)
			{
				OnCommandNextFrame();
			}
			else if(itmeHandle == mVideoFramesCheckBox)
			{
				bool checked = (SendMessage(mVideoFramesCheckBox, BM_GETCHECK, 0, 0) == BST_CHECKED);
				if(checked)
				{
					SendMessage(mVideoFramesCheckBox, BM_SETCHECK, BST_UNCHECKED, 0);
					mSaveVideoFrames = false;
				}
				else
				{
					SendMessage(mVideoFramesCheckBox, BM_SETCHECK, BST_CHECKED, 0);
					mSaveVideoFrames = true;
				}
			}
			else if(itmeHandle == mSpawnTextBox)
			{
				if(HIWORD(wparam) == EN_CHANGE) //Spawn period textbox contents changed
				{
					uint32_t spawnPeriod = ParseSpawnPeriod();
					if(spawnPeriod != 0 && mPlayMode == PlayMode::MODE_STOP)
					{
						EnableWindow(mSmoothCheckBox, TRUE);
					}
					else
					{
						EnableWindow(mSmoothCheckBox, FALSE);
					}
				}
			}
			else if(itmeHandle == mSmoothCheckBox)
			{
				bool checked = (SendMessage(mSmoothCheckBox, BM_GETCHECK, 0, 0) == BST_CHECKED);
				if(checked)
				{
					SendMessage(mSmoothCheckBox, BM_SETCHECK, BST_UNCHECKED, 0);
					mUseSmoothTransform = false;
				}
				else
				{
					SendMessage(mSmoothCheckBox, BM_SETCHECK, BST_CHECKED, 0);
					mUseSmoothTransform = true;
				}
			}
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
			UINT clickRuleMenuFlags     = MF_BYCOMMAND | MF_STRING;
			UINT clickRuleMenuFileFlags = MF_BYCOMMAND | MF_STRING;

			if(mPlayMode != PlayMode::MODE_STOP)
			{
				clickRuleMenuFileFlags = clickRuleMenuFlags | MF_DISABLED | MF_GRAYED;
			}

			HMENU popupMenu = CreatePopupMenu();
			InsertMenu(popupMenu, 0, clickRuleMenuFileFlags, MENU_OPEN_CLICK_RULE, L"Open click rule...");
			InsertMenu(popupMenu, 0, clickRuleMenuFileFlags, MENU_SAVE_CLICK_RULE, L"Save click rule...");
		
			InsertMenu(popupMenu, 0, clickRuleMenuFileFlags, MENU_RESET_CLICK_RULE, L"Reset click rule");

			if(mRenderer->GetClickRuleGridVisible())
			{
				InsertMenu(popupMenu, 0, clickRuleMenuFlags, MENU_HIDE_CLICK_RULE_GRID, L"Hide grid");
			}
			else
			{
				InsertMenu(popupMenu, 0, clickRuleMenuFlags, MENU_SHOW_CLICK_RULE_GRID, L"Show grid");
			}

			TrackPopupMenu(popupMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON | TPM_NOANIMATION, pt.x, pt.y, 0, mMainWindowHandle, nullptr);
		}
		else if(PtInRect(&previewRect, pt))
		{
			UINT boardLoadMenuFlags = MF_BYCOMMAND | MF_STRING;
			UINT boardSaveMenuFlags = MF_BYCOMMAND | MF_STRING;
			if(mPlayMode != PlayMode::MODE_STOP)
			{
				boardLoadMenuFlags = boardLoadMenuFlags | MF_DISABLED | MF_GRAYED;
				if(mPlayMode != PlayMode::MODE_PAUSED)
				{
					boardSaveMenuFlags = boardSaveMenuFlags | MF_DISABLED | MF_GRAYED;
				}
			}

			HMENU popupMenu = CreatePopupMenu();
			InsertMenu(popupMenu, 0, boardLoadMenuFlags, MENU_OPEN_BOARD, L"Open board...");
			InsertMenu(popupMenu, 0, boardSaveMenuFlags, MENU_SAVE_BOARD, L"Save stability...");

			      UINT              initialStateMenuIDs[3]    = {MENU_INITIAL_STATE_CORNERS,         MENU_INITIAL_STATE_SIDES,         MENU_INITIAL_STATE_CENTER};
			const WCHAR*            initialStateMenuLabels[3] = {L"Corners" ,                        L"Sides" ,                        L"Center"};
                  ResetBoardModeApp initialStateResetModes[3] = {ResetBoardModeApp::RESET_4_CORNERS, ResetBoardModeApp::RESET_4_SIDES, ResetBoardModeApp::RESET_CENTER};

			HMENU initialStatesMenu = CreatePopupMenu();
			for(int i = 0; i < 3; i++)
			{
				UINT currFlags = boardSaveMenuFlags;
				if(mResetMode == initialStateResetModes[i])
				{
					currFlags = currFlags | MF_CHECKED;
				}

				InsertMenu(initialStatesMenu, 0, currFlags, initialStateMenuIDs[i], initialStateMenuLabels[i]);
			}

			AppendMenu(popupMenu, MF_POPUP, (UINT_PTR)initialStatesMenu, L"Initial state");
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
	case WM_CTLCOLORSTATIC: //To paint the log white
	{
		if((HWND)lparam == mLogAreaHandle)
		{
			HBRUSH whiteBrush = GetSysColorBrush(COLOR_WINDOW);
			return (LRESULT)(whiteBrush);
		}
		else if((HWND)lparam == mLastFrameTextBox) //Paint the last frame text box differently
		{
			HBRUSH whiteBrush = GetSysColorBrush(COLOR_WINDOWFRAME);
			return (LRESULT)(whiteBrush);
		}
		else if((HWND)lparam == mSpawnTextBox) //Paint the last frame text box differently
		{
			HBRUSH whiteBrush = GetSysColorBrush(COLOR_WINDOWFRAME);
			return (LRESULT)(whiteBrush);
		}
		else
		{
			return DefWindowProc(hwnd, message, wparam, lparam);
		}
	}
	case WM_HSCROLL: //Trackbar slide
	{
		if((HWND)lparam == mSizeTrackbar)
		{
			int sliderPos = SendMessage(mSizeTrackbar, TBM_GETPOS, 0, 0);

			int minSize = (1 << 2)  - 1;
			int maxSize = (1 << 14) - 1;

			int chosenSize = (1 << sliderPos) - 1;
			
			if(chosenSize < minSize)
			{
				chosenSize = minSize;
			}

			if(chosenSize > maxSize)
			{
				chosenSize = maxSize;
			}

			std::wstring wndTitle = L"Stability fractal " + std::to_wstring(chosenSize) + L"x" + std::to_wstring(chosenSize);
			SetWindowText(mMainWindowHandle, wndTitle.c_str());
		}

		return 0;
	}
	case WM_NOTIFY: //Trackbar released
	{
		NMHDR* notificationInfo = (NMHDR*)(lparam);
		if(notificationInfo->hwndFrom == mSizeTrackbar && notificationInfo->code == NM_RELEASEDCAPTURE) //Trackbar mouse released
		{
			int sliderPos = SendMessage(mSizeTrackbar, TBM_GETPOS, 0, 0);

			int minSize = (1 << 2) - 1;
			int maxSize = (1 << 14) - 1;

			int chosenSize = (1 << sliderPos) - 1;

			if (chosenSize < minSize)
			{
				chosenSize = minSize;
			}

			if (chosenSize > maxSize)
			{
				chosenSize = maxSize;
			}

			PostThreadMessage(mRenderThreadID, RENDER_THREAD_RESIZE_BOARD, chosenSize, chosenSize);
		}

		break;
	}
	case MAIN_THREAD_APPEND_TO_LOG:
	{
		//Sometimes the same message for unknown reasons is processed multiple times.
		//Dirty workaround: additional check process the message ONLY if it wasn't processed yet.
		//How do we know if it was processed? Keep the amount of processed messages in mLoggerMessageCount 
		//and the id of the currently processed message in wparam
		if(mLoggerMessageCount < wparam)
		{
			mLogger->Block(); //Can't let the logger update while we're processing the logger message

			mLogger->Flush(); //Tell the logger that we've processed the message

			//We can't unfortunately do it in logger because it will create a deadlock during window destruction and thread closing,
			//because we use WaitForSingleObject to wait for background threads to finish. If during that time we call SendMessage()
			//from a background thread, deadlock will occur since background thread waits for the main thread to process the message
			//while the main thread waits for the background thread to close. 
			//Solution: handle log window updates in the main thread only.
			int currTextLength = GetWindowTextLength(mLogAreaHandle);

			//Update the logger area
			SendMessage(mLogAreaHandle, EM_SETSEL,     currTextLength, currTextLength);
			SendMessage(mLogAreaHandle, EM_REPLACESEL, FALSE,          lparam);
			SendMessage(mLogAreaHandle, EM_SCROLL,     SB_LINEDOWN,    0);

			mLogger->Unblock();

			mLoggerMessageCount++;
		}

		return 0;
	}
	default:
	{
		break;
	}
	}

	return DefWindowProc(hwnd, message, wparam, lparam);
}

void WindowApp::RenderThreadFunc()
{
	MSG threadMsg = {0};
	PeekMessage(&threadMsg, nullptr, WM_USER, WM_USER, PM_NOREMOVE); //This creates the thread message queue
	SetEvent(mCreateRenderThreadEvent);

	bool bThreadRunning = true;
	while(bThreadRunning)
	{
		GetMessage(&threadMsg, (HWND)(-1), WM_APP, 0xBFFF);

		switch (threadMsg.message)
		{
		case RENDER_THREAD_EXIT:
		{
			bThreadRunning = false;
			break;
		}
		case RENDER_THREAD_REINIT:
		{
			InitBoard(mFractalGen->GetWidth(), mFractalGen->GetHeight());

			mFractalGen->ResetComputingParameters();

			std::wstring wndTitle = L"Stability fractal " + std::to_wstring(mFractalGen->GetWidth()) + L"x" + std::to_wstring(mFractalGen->GetHeight());
			SetWindowText(mMainWindowHandle, wndTitle.c_str());

			break;
		}
		case RENDER_THREAD_RESIZE:
		{
			int width  = LOWORD(threadMsg.lParam);
			int height = HIWORD(threadMsg.lParam);

			mRenderer->ResizePreviewArea(width, height);
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
			LoadClickRuleFromFile(*clickRuleFilenamePtr);
			break;
		}
		case RENDER_THREAD_LOAD_BOARD:
		{
			//Catch the pointer
			std::unique_ptr<std::wstring> boardFilenamePtr(reinterpret_cast<std::wstring*>(threadMsg.lParam));
			LoadBoardFromFile(*boardFilenamePtr);

			std::wstring wndTitle = L"Stability fractal " + std::to_wstring(mFractalGen->GetWidth()) + L"x" + std::to_wstring(mFractalGen->GetHeight());
			SetWindowText(mMainWindowHandle, wndTitle.c_str());

			int psize = (int)log2f((mFractalGen->GetWidth() + 1));
			PostMessage(mSizeTrackbar, TBM_SETPOS, TRUE, psize);

			uint32_t finalFrame = mFractalGen->GetDefaultSolutionPeriod(mFractalGen->GetWidth());
			std::wstring finalFrameStr = std::to_wstring(finalFrame);
			SendMessage(mLastFrameTextBox, WM_SETTEXT, 0, (LPARAM)finalFrameStr.c_str());

			mResetMode = ResetBoardModeApp::RESET_CUSTOM_IMAGE;
			break;
		}
		case RENDER_THREAD_SAVE_STABILITY:
		{
			//Catch the pointer
			std::unique_ptr<std::wstring> stabilityFilenamePtr(reinterpret_cast<std::wstring*>(threadMsg.lParam));
			SaveStability(*stabilityFilenamePtr);
			break;
		}
		case RENDER_THREAD_SAVE_VIDEO_FRAME:
		{
			//Catch the pointer
			std::unique_ptr<std::wstring> frameFilenamePtr(reinterpret_cast<std::wstring*>(threadMsg.lParam));
			SaveCurrentVideoFrame(*frameFilenamePtr);
			break;
		}
		case RENDER_THREAD_REDRAW:
		{
			mRenderer->DrawPreview();
			break;
		}
		case RENDER_THREAD_REDRAW_CLICK_RULE:
		{
			mRenderer->DrawClickRule();
			break;
		}
		case RENDER_THREAD_COMPUTE_TICK:
		{
			ComputeFractalTick();
			break;
		}
		case RENDER_THREAD_RESIZE_BOARD:
		{
			uint32_t width  = (uint32_t)(threadMsg.wParam);
			uint32_t height = (uint32_t)(threadMsg.lParam);

			if(mResetMode == ResetBoardModeApp::RESET_CUSTOM_IMAGE)
			{
				mFractalGen->ChangeSize(width, height);
			}
			else
			{
				InitBoard(width, height);
			}

			std::wstring wndTitle = L"Stability fractal " + std::to_wstring(mFractalGen->GetWidth()) + L"x" + std::to_wstring(mFractalGen->GetHeight());
			SetWindowText(mMainWindowHandle, wndTitle.c_str());

			uint32_t finalFrame = mFractalGen->GetDefaultSolutionPeriod(mFractalGen->GetWidth());
			std::wstring finalFrameStr = std::to_wstring(finalFrame);
			SendMessage(mLastFrameTextBox, WM_SETTEXT, 0, (LPARAM)finalFrameStr.c_str());

			break;
		}
		case RENDER_THREAD_SYNC:
		{
			PostThreadMessage(mTickThreadID, TICK_THREAD_SYNC, 0, 0);
			break;
		}
		default:
			break;
		}
	}
}

void WindowApp::TickThreadFunc()
{
	MSG threadMsg = { 0 };
	PeekMessage(&threadMsg, nullptr, WM_USER, WM_USER, PM_NOREMOVE); //This creates the thread message queue
	SetEvent(mCreateTickThreadEvent);

	while(true)
	{
		GetMessage(&threadMsg, (HWND)(-1), WM_APP, 0xBFFF);
		if(threadMsg.message == TICK_THREAD_EXIT)
		{
			break;
		}
		else if(threadMsg.message == TICK_THREAD_SYNC) //Next tick available
		{
			if(mRenderer->ConsumeNeedRedraw())
			{
				PostThreadMessage(mRenderThreadID, RENDER_THREAD_REDRAW, 0, 0);
			}

			if(mRenderer->ConsumeNeedRedrawClickRule())
			{
				PostThreadMessage(mRenderThreadID, RENDER_THREAD_REDRAW_CLICK_RULE, 0, 0);
			}

			if(mPlayMode == PlayMode::MODE_SINGLE_FRAME || mPlayMode == PlayMode::MODE_CONTINUOUS_FRAMES)
			{
				PostThreadMessage(mRenderThreadID, RENDER_THREAD_COMPUTE_TICK, 0, 0);

				if(mPlayMode == PlayMode::MODE_SINGLE_FRAME)
				{
					mPlayMode = PlayMode::MODE_PAUSED;
				}

				if(mSaveVideoFrames && mFractalGen->GetLastFrameNumber() <= mFinalFrameNumber)
				{
					std::wstring frameNumberStr = IntermediateStateString(mFractalGen->GetLastFrameNumber());

					std::unique_ptr<std::wstring> frameFilenamePtr = std::make_unique<std::wstring>(L"DiffStabil\\Stabl" + frameNumberStr + L".png");
					PostThreadMessage(mRenderThreadID, RENDER_THREAD_SAVE_VIDEO_FRAME, 0, reinterpret_cast<LPARAM>(frameFilenamePtr.release()));
				}

				if(mFractalGen->GetLastFrameNumber() == mFinalFrameNumber)
				{
					std::unique_ptr<std::wstring> stabilityFilenamePtr = std::make_unique<std::wstring>(L"Stability.png");
					PostThreadMessage(mRenderThreadID, RENDER_THREAD_SAVE_STABILITY, 0, reinterpret_cast<LPARAM>(stabilityFilenamePtr.release()));
				}
			}

			PostThreadMessage(mRenderThreadID, RENDER_THREAD_SYNC, 0, 0); //Don't produce next commands until renderer finishes with these
		}
	}	
}

uint32_t WindowApp::ParseFinalFrame()
{
	uint32_t finalFrameTextLength = SendMessage(mLastFrameTextBox, WM_GETTEXTLENGTH, 0, 0);
	if(finalFrameTextLength == 0)
	{
		uint32_t finalFrame = mFractalGen->GetDefaultSolutionPeriod(mFractalGen->GetWidth());

		std::wstring finalFrameStr = std::to_wstring(finalFrame);
		SendMessage(mLastFrameTextBox, WM_SETTEXT, 0, (LPARAM)finalFrameStr.c_str());

		return finalFrame;
	}
	else
	{
		std::wstring finalFrameStr;
		finalFrameStr.resize(finalFrameTextLength);

		SendMessage(mLastFrameTextBox, WM_GETTEXT, finalFrameTextLength + 1, (LPARAM)finalFrameStr.data());

		uint32_t finalFrame = 0;
		std::wistringstream iss(finalFrameStr);

		if(!(iss >> finalFrame)) //The string cannot be parsed
		{
			finalFrame = mFractalGen->GetDefaultSolutionPeriod(mFractalGen->GetWidth());

			finalFrameStr = std::to_wstring(finalFrame);
			SendMessage(mLastFrameTextBox, WM_SETTEXT, 0, (LPARAM)finalFrameStr.c_str());
		}

		return finalFrame;
	}
}

uint32_t WindowApp::ParseSpawnPeriod()
{
	uint32_t spawnTextLength = SendMessage(mSpawnTextBox, WM_GETTEXTLENGTH, 0, 0);
	if(spawnTextLength == 0)
	{
		uint32_t spawnPeriod = 0;

		std::wstring spawnStr = std::to_wstring(spawnPeriod);
		SendMessage(mSpawnTextBox, WM_SETTEXT, 0, (LPARAM)spawnStr.c_str());

		return spawnPeriod;
	}
	else
	{
		std::wstring spawnStr;
		spawnStr.resize(spawnTextLength);

		SendMessage(mSpawnTextBox, WM_GETTEXT, spawnTextLength + 1, (LPARAM)spawnStr.data());

		uint32_t spawnPeriod = 0;
		std::wistringstream iss(spawnStr);

		if(!(iss >> spawnPeriod)) //The string cannot be parsed
		{
			spawnPeriod = 0;

			spawnStr = std::to_wstring(spawnPeriod);
			SendMessage(mSpawnTextBox, WM_SETTEXT, 0, (LPARAM)spawnStr.c_str());
		}

		return spawnPeriod;
	}
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
	wc.lpfnWndProc   = [](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT
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

	mMainWindowHandle = CreateWindowEx(wndStyleEx, windowClassName, L"Stability Fractal", wndStyle, clientRect.left, clientRect.top, mMinWindowWidth, mMinWindowHeight, nullptr, nullptr, hInstance, this);

	UpdateWindow(mMainWindowHandle);
	ShowWindow(mMainWindowHandle, SW_SHOW);
}

void WindowApp::CreateChildWindows(HINSTANCE hInstance)
{
	DWORD previewStyle          = 0;
	DWORD clickRuleStyle        = 0;
	DWORD logStyle              = ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_LEFT | ES_MULTILINE | ES_READONLY /* | ES_UPPERCASE /*The best one!*/ | ES_WANTRETURN;
	DWORD buttonStyle           = BS_TEXT | BS_FLAT | BS_PUSHBUTTON;
	DWORD trackbarStyle         = TBS_AUTOTICKS | TBS_HORZ | TBS_BOTTOM;
	DWORD checkboxStyle         = BS_TEXT | BS_FLAT | BS_CHECKBOX;
	DWORD labelStyle            = SS_LEFTNOWORDWRAP;
	DWORD textBoxLastFrameStyle = ES_LEFT | ES_NUMBER;

	mPreviewAreaHandle   = CreateWindowEx(0, WC_STATIC, L"Preview",   WS_CHILD |              previewStyle,   0, 0, gPreviewAreaMinWidth, gPreviewAreaMinHeight, mMainWindowHandle, nullptr, hInstance, nullptr);
	mClickRuleAreaHandle = CreateWindowEx(0, WC_STATIC, L"ClickRule", WS_CHILD |              clickRuleStyle, 0, 0, gClickRuleAreaWidth,  gClickRuleAreaHeight,  mMainWindowHandle, nullptr, hInstance, nullptr);
	mLogAreaHandle       = CreateWindowEx(0, WC_EDIT,   L"",          WS_CHILD | WS_VSCROLL | logStyle,       0, 0, gMinLogAreaWidth,     gMinLogAreaHeight,     mMainWindowHandle, nullptr, hInstance, nullptr);

	mButtonReset     = CreateWindowEx(0, WC_BUTTON, L"🔄", WS_CHILD | buttonStyle, 0, 0, gButtonWidth, gButtonHeight, mMainWindowHandle, (HMENU)(MENU_RESET),      hInstance, nullptr);
	mButtonPausePlay = CreateWindowEx(0, WC_BUTTON, L"⏸", WS_CHILD | buttonStyle, 0, 0, gButtonWidth, gButtonHeight, mMainWindowHandle, (HMENU)(MENU_PAUSE),      hInstance, nullptr);
	mButtonStop      = CreateWindowEx(0, WC_BUTTON, L"⏹", WS_CHILD | buttonStyle, 0, 0, gButtonWidth, gButtonHeight, mMainWindowHandle, (HMENU)(MENU_STOP),       hInstance, nullptr);
	mButtonNextFrame = CreateWindowEx(0, WC_BUTTON, L"⏭️", WS_CHILD | buttonStyle, 0, 0, gButtonWidth, gButtonHeight, mMainWindowHandle, (HMENU)(MENU_NEXT_FRAME), hInstance, nullptr);

	mSizeTrackbar = CreateWindowEx(0, TRACKBAR_CLASS, L"Size", WS_CHILD | trackbarStyle, 0, 0, gMinLogAreaWidth, gTrackBarHeight, mMainWindowHandle, nullptr, hInstance, nullptr);

	mVideoFramesCheckBox = CreateWindowEx(0, WC_BUTTON, L"Save video frames", WS_CHILD | checkboxStyle, 0, 0, gMinLogAreaWidth, gCheckBoxHeight, mMainWindowHandle, nullptr, hInstance, nullptr);

	mLastFrameLabel   = CreateWindowEx(0, WC_STATIC, L"Last frame: ", WS_CHILD | labelStyle,            0, 0, gInputLabelMinWidth, gInputLayoutHeight, mMainWindowHandle, nullptr, hInstance, nullptr);
	mLastFrameTextBox = CreateWindowEx(0, WC_EDIT,   L"",             WS_CHILD | textBoxLastFrameStyle, 0, 0, gInputTextBoxWidth,  gInputLayoutHeight, mMainWindowHandle, nullptr, hInstance, nullptr);
	
	mSpawnLabel   = CreateWindowEx(0, WC_STATIC, L"Spawn: ", WS_CHILD | labelStyle,            0, 0, gInputLabelMinWidth, gInputLayoutHeight, mMainWindowHandle, nullptr, hInstance, nullptr);
	mSpawnTextBox = CreateWindowEx(0, WC_EDIT,   L"0",       WS_CHILD | textBoxLastFrameStyle, 0, 0, gInputTextBoxWidth,  gInputLayoutHeight, mMainWindowHandle, nullptr, hInstance, nullptr);

	mSmoothCheckBox = CreateWindowEx(0, WC_BUTTON, L"Use smooth visuals", WS_CHILD | checkboxStyle, 0, 0, gMinLogAreaWidth, gCheckBoxHeight, mMainWindowHandle, nullptr, hInstance, nullptr);

	UpdateWindow(mPreviewAreaHandle);
	ShowWindow(mPreviewAreaHandle, SW_SHOW);

	UpdateWindow(mClickRuleAreaHandle);
	ShowWindow(mClickRuleAreaHandle, SW_SHOW);

	UpdateWindow(mLogAreaHandle);
	ShowWindow(mLogAreaHandle, SW_SHOW);

	UpdateWindow(mButtonReset);
	UpdateWindow(mButtonPausePlay);
	UpdateWindow(mButtonStop);
	UpdateWindow(mButtonNextFrame);

	ShowWindow(mButtonReset,     SW_SHOW);
	ShowWindow(mButtonPausePlay, SW_SHOW);
	ShowWindow(mButtonStop,      SW_SHOW);
	ShowWindow(mButtonNextFrame, SW_SHOW);

	UpdateWindow(mSizeTrackbar);
	ShowWindow(mSizeTrackbar, SW_SHOW);

	EnableWindow(mSizeTrackbar, FALSE);
	EnableWindow(mVideoFramesCheckBox, FALSE);
	EnableWindow(mLastFrameLabel, FALSE);
	EnableWindow(mLastFrameTextBox, FALSE);
	EnableWindow(mSpawnLabel, FALSE);
	EnableWindow(mSpawnTextBox, FALSE);
	EnableWindow(mSmoothCheckBox, FALSE);

	UpdateWindow(mVideoFramesCheckBox);
	ShowWindow(mVideoFramesCheckBox, SW_SHOW);

	UpdateWindow(mLastFrameLabel);
	UpdateWindow(mLastFrameTextBox);

	ShowWindow(mLastFrameLabel,   SW_SHOW);
	ShowWindow(mLastFrameTextBox, SW_SHOW);

	UpdateWindow(mSpawnLabel);
	UpdateWindow(mSpawnTextBox);

	ShowWindow(mSpawnLabel, SW_SHOW);
	ShowWindow(mSpawnTextBox, SW_SHOW);

	UpdateWindow(mSmoothCheckBox);
	ShowWindow(mSmoothCheckBox, SW_SHOW);

	//We need to redirect WM_KEYUP messages from the logger window and buttons to the main window
	auto keyUpSubclassProc = [](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
	{
		if(message == WM_KEYUP)
		{
			HWND parentWnd = GetParent(hwnd);
			SendMessage(parentWnd, message, wparam, lparam);

			if(wparam == 'S')
			{
				SetFocus(parentWnd);
			}
		}

		return DefSubclassProc(hwnd, message, wparam, lparam);
	};

	//Subclass the window procedure to redirect WM_KEYUP messages
	SetWindowSubclass(mLogAreaHandle, keyUpSubclassProc, 0, 0);

	SetWindowSubclass(mButtonNextFrame, keyUpSubclassProc, 0, 0);
	SetWindowSubclass(mButtonPausePlay, keyUpSubclassProc, 0, 0);
	SetWindowSubclass(mButtonStop,      keyUpSubclassProc, 0, 0);
	SetWindowSubclass(mButtonNextFrame, keyUpSubclassProc, 0, 0);

	SetWindowSubclass(mSizeTrackbar, keyUpSubclassProc, 0, 0);

	SetWindowSubclass(mVideoFramesCheckBox, keyUpSubclassProc, 0, 0);
	SetWindowSubclass(mLastFrameLabel,      keyUpSubclassProc, 0, 0);
	SetWindowSubclass(mLastFrameTextBox,    keyUpSubclassProc, 0, 0);

	SetWindowSubclass(mSpawnLabel,   keyUpSubclassProc, 0, 0);
	SetWindowSubclass(mSpawnTextBox, keyUpSubclassProc, 0, 0);

	SetWindowSubclass(mSmoothCheckBox, keyUpSubclassProc, 0, 0);

	//Set logger area font
	std::wstring logFontName = L"Lucida Console";

	LOGFONT logFontDesc;
	logFontDesc.lfHeight         = 20;
	logFontDesc.lfWidth          = 0;
	logFontDesc.lfEscapement     = 0;
	logFontDesc.lfOrientation    = 0;
	logFontDesc.lfWeight         = FW_NORMAL;
	logFontDesc.lfItalic         = FALSE;
	logFontDesc.lfUnderline      = FALSE;
	logFontDesc.lfStrikeOut      = FALSE;
	logFontDesc.lfCharSet        = OEM_CHARSET;
	logFontDesc.lfOutPrecision   = OUT_DEFAULT_PRECIS;
	logFontDesc.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
	logFontDesc.lfQuality        = PROOF_QUALITY;
	logFontDesc.lfPitchAndFamily = FF_MODERN | FIXED_PITCH;
	memcpy_s(logFontDesc.lfFaceName, 32, logFontName.c_str(), logFontName.length() + 1);

	mLogAreaFont = CreateFontIndirect(&logFontDesc);
	if(mLogAreaFont)
	{
		SendMessage(mLogAreaHandle, WM_SETFONT, (WPARAM)mLogAreaFont, TRUE);
	}

	SendMessage(mLogAreaHandle, EM_SETLIMITTEXT, 20000000, 0);

	//Change the size of the buttons font
	HFONT fontDefault = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

	LOGFONT btnFont;
	GetObject(fontDefault, sizeof(LOGFONT), (void*)&btnFont);

	btnFont.lfHeight = 28;
	mButtonsFont = CreateFontIndirect(&btnFont);

	if(mButtonsFont)
	{
		SendMessage(mButtonReset,     WM_SETFONT, (WPARAM)mButtonsFont, TRUE);
		SendMessage(mButtonPausePlay, WM_SETFONT, (WPARAM)mButtonsFont, TRUE);
		SendMessage(mButtonStop,      WM_SETFONT, (WPARAM)mButtonsFont, TRUE);
		SendMessage(mButtonNextFrame, WM_SETFONT, (WPARAM)mButtonsFont, TRUE);
	}

	//Change the trackbar range. The actual size of the image is given by (2^p - 1)
	UINT16 minPSize = 2;
	UINT16 maxPSize = 14;

	SendMessage(mSizeTrackbar, TBM_SETRANGE, TRUE, MAKELPARAM(minPSize, maxPSize));

	SetFocus(mMainWindowHandle);
}

void WindowApp::CreateBackgroundTaskThreads()
{
	auto RenderThreadProc = [](LPVOID lpParam) -> DWORD
	{
		WindowApp* that = (WindowApp*)(lpParam);
		that->RenderThreadFunc();
		return 0;
	};

	auto TickThreadProc = [](LPVOID lpParam) -> DWORD
	{
		WindowApp* that = (WindowApp*)(lpParam);
		that->TickThreadFunc();
		return 0;
	};

	mCreateRenderThreadEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if(mCreateRenderThreadEvent)
	{
		mRenderThreadHandle = CreateThread(nullptr, 0, RenderThreadProc, this, 0, nullptr);
		if(mRenderThreadHandle)
		{
			mRenderThreadID = GetThreadId(mRenderThreadHandle);
			WaitForSingleObject(mCreateRenderThreadEvent, INFINITE);
		}
		else
		{
			ThrowIfFailed(GetLastError());
		}
	}
	else
	{
		ThrowIfFailed(GetLastError());
	}

	mCreateTickThreadEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if(mCreateTickThreadEvent)
	{
		mTickThreadHandle = CreateThread(nullptr, 0, TickThreadProc, this, 0, nullptr);
		if(mTickThreadHandle)
		{
			WaitForSingleObject(mCreateTickThreadEvent, INFINITE);
			mTickThreadID = GetThreadId(mTickThreadHandle);
			PostThreadMessage(mTickThreadID, TICK_THREAD_SYNC, 0, 0); //Syncing the render and tick threads
		}
		else
		{
			ThrowIfFailed(GetLastError());
		}
	}
	else
	{
		ThrowIfFailed(GetLastError());
	}
}

void WindowApp::CloseBackgroundTaskThreads()
{
	PostThreadMessage(mTickThreadID, TICK_THREAD_EXIT, 0, 0);
	WaitForSingleObject(mTickThreadHandle, 5000);
	CloseHandle(mTickThreadHandle);

	PostThreadMessage(mRenderThreadID, RENDER_THREAD_EXIT, 0, 0);
	WaitForSingleObject(mRenderThreadHandle, 5000);
	CloseHandle(mRenderThreadHandle);
}

void WindowApp::LayoutChildWindows()
{
	RECT wndRect;
	GetClientRect(mMainWindowHandle, &wndRect);

	int wndWidth  = wndRect.right  - wndRect.left;
	int wndHeight = wndRect.bottom - wndRect.top;

	int rightSideWidth  = gClickRuleAreaWidth;
	int rightSideHeight = gMinLogAreaHeight + gSpacing + gClickRuleAreaHeight;

	int previewHeight = wndHeight - gMarginTop - gMarginBottom;
	int previewWidth  = wndWidth  - rightSideWidth - gSpacing - gMarginLeft - gMarginRight;

	int sizePreview = std::min(previewWidth, previewHeight);
	previewWidth    = sizePreview;
	previewHeight   = sizePreview;

	//Log: placed right of the preview, stretching to the far right of the window
	int logWidth  = wndWidth  - gSpacing - previewWidth - gMarginLeft - gMarginRight;
	int logHeight = wndHeight - gSpacing - gClickRuleAreaHeight - gMarginTop - gMarginBottom;
	
	//Buttons: placed below the log and right of the click rule, centered
	int buttonLeftRightSpace = (logWidth - gClickRuleAreaWidth - gButtonWidth * 4.0f - gSpacing * 3.0f) / 2;

	int buttonsPosY      = gMarginTop + sizePreview - gClickRuleAreaHeight;
	int buttonsStartPosX = gMarginLeft + previewWidth + gSpacing + gClickRuleAreaWidth + buttonLeftRightSpace;

	int buttonResetPosX     = buttonsStartPosX;
	int buttonPausePlayPosX = buttonResetPosX     + gButtonWidth + gSpacing;
	int buttonStopPosX      = buttonPausePlayPosX + gButtonWidth + gSpacing;
	int buttonNextFramePosX = buttonStopPosX      + gButtonWidth + gSpacing;

	//Trackbar: horizontally aligned with buttons, vertically placed below them, fills the entire right side of the screen horizontally
	int trackbarPosX = gMarginLeft + previewWidth + gSpacing + gClickRuleAreaWidth + gSpacing;
	int trackbarPosY = buttonsPosY + gButtonHeight + gSpacing;

	int trackbarWidth  = logWidth - gClickRuleAreaWidth - gSpacing * 2;
	int trackbarHeight = gTrackBarHeight;

	//Save video frames checkbox: horizontally aligned with buttons, vertically placed below the trackbar, fills the entire right side of the screen horizontally
	int checkBoxVideoPosX = trackbarPosX;
	int checkBoxVideoPosY = trackbarPosY + trackbarHeight + gSpacing;

	int checkBoxVideoWidth  = logWidth - gClickRuleAreaWidth - gSpacing * 2;
	int checkBoxVideoHeight = gCheckBoxHeight;

	//Last frame layout: horizontally aligned with buttons, vertically placed below the save video frames checkbox. The text box is fixed width and placed on the right, the label fills the remaining space
	int lastFrameLayoutPosY   = checkBoxVideoPosY + checkBoxVideoHeight + gSpacing;
	int lastFrameLayoutHeight = gInputLayoutHeight;

	int labelLastFramePosX   = trackbarPosX;
	int textBoxLastFramePosX = labelLastFramePosX + trackbarWidth - gInputTextBoxWidth;

	int textBoxLastFrameWidth = gInputTextBoxWidth;
	int labelLastFrameWidth   = trackbarWidth - textBoxLastFrameWidth - gSpacing;

	//Spawn period layout: the same alignment as last frame layout, but placed below it
	int spawnLayoutPosY   = lastFrameLayoutPosY + lastFrameLayoutHeight + gSpacing;
	int spawnLayoutHeight = gInputLayoutHeight;

	int labelSpawnPosX   = trackbarPosX;
	int textBoxSpawnPosX = labelSpawnPosX + trackbarWidth - gInputTextBoxWidth;

	int textBoxSpawnWidth = gInputTextBoxWidth;
	int labelSpawnWidth   = trackbarWidth - textBoxSpawnWidth - gSpacing;

	//Smooth checkbox: the same alignment as save video frames checkbox, but placed below spawn period layout
	int checkBoxSmoothPosX = checkBoxVideoPosX;
	int checkBoxSmoothPosY = spawnLayoutPosY + spawnLayoutHeight + gSpacing;

	int checkBoxSmoothWidth  = checkBoxVideoWidth;
	int checkBoxSmoothHeight = gCheckBoxHeight;

	//Finally layout the elements
	SetWindowPos(mPreviewAreaHandle,   HWND_TOP, gMarginLeft,                           gMarginTop,                                      sizePreview,         sizePreview,          0);
	SetWindowPos(mClickRuleAreaHandle, HWND_TOP, gMarginLeft + previewWidth + gSpacing, gMarginTop + sizePreview - gClickRuleAreaHeight, gClickRuleAreaWidth, gClickRuleAreaHeight, 0);
	SetWindowPos(mLogAreaHandle,       HWND_TOP, gMarginLeft + previewWidth + gSpacing, gMarginTop,                                      logWidth,            logHeight,            0);

	SetWindowPos(mButtonReset,     HWND_TOP, buttonResetPosX,     buttonsPosY, gButtonWidth, gButtonHeight, 0);
	SetWindowPos(mButtonPausePlay, HWND_TOP, buttonPausePlayPosX, buttonsPosY, gButtonWidth, gButtonHeight, 0);
	SetWindowPos(mButtonStop,      HWND_TOP, buttonStopPosX,      buttonsPosY, gButtonWidth, gButtonHeight, 0);
	SetWindowPos(mButtonNextFrame, HWND_TOP, buttonNextFramePosX, buttonsPosY, gButtonWidth, gButtonHeight, 0);

	SetWindowPos(mSizeTrackbar, HWND_TOP, trackbarPosX, trackbarPosY, trackbarWidth, gTrackBarHeight, 0);

	SetWindowPos(mVideoFramesCheckBox, HWND_TOP, checkBoxVideoPosX, checkBoxVideoPosY, checkBoxVideoWidth, checkBoxVideoHeight, 0);

	SetWindowPos(mLastFrameLabel,   HWND_TOP, labelLastFramePosX,   lastFrameLayoutPosY, labelLastFrameWidth,   lastFrameLayoutHeight, 0);
	SetWindowPos(mLastFrameTextBox, HWND_TOP, textBoxLastFramePosX, lastFrameLayoutPosY, textBoxLastFrameWidth, lastFrameLayoutHeight, 0);

	SetWindowPos(mSpawnLabel,   HWND_TOP, labelSpawnPosX,   spawnLayoutPosY, labelSpawnWidth,   spawnLayoutHeight, 0);
	SetWindowPos(mSpawnTextBox, HWND_TOP, textBoxSpawnPosX, spawnLayoutPosY, textBoxSpawnWidth, spawnLayoutHeight, 0);

	SetWindowPos(mSmoothCheckBox, HWND_TOP, checkBoxSmoothPosX, checkBoxSmoothPosY, checkBoxSmoothWidth, checkBoxSmoothHeight, 0);
}

void WindowApp::CalculateMinWindowSize()
{
	mMinWindowWidth  = gMarginLeft + gMinLeftSideWidth + gSpacing + gMinRightSideWidth + gMarginRight;
	mMinWindowHeight = gMarginTop  + std::max(gMinLeftSideHeight, gMinRightSideHeight) + gMarginBottom;
}

void WindowApp::UpdateRendererForPreview()
{
	RECT previewAreaRect;
	GetClientRect(mPreviewAreaHandle, &previewAreaRect);

	int previewWidth  = previewAreaRect.right  - previewAreaRect.left;
	int previewHeight = previewAreaRect.bottom - previewAreaRect.top;

	LPARAM lparam = MAKELPARAM(previewWidth, previewHeight);
	PostThreadMessage(mRenderThreadID, RENDER_THREAD_RESIZE, 0, lparam);
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
	case MENU_SAVE_BOARD:
	{
		std::wstring boardFilename = L"Stability.png";

		FileDialog fileDialog;
		if(fileDialog.GetFilenameToSave(mMainWindowHandle, boardFilename))
		{
			std::unique_ptr<std::wstring> boardFilenamePtr = std::make_unique<std::wstring>(boardFilename);
			PostThreadMessage(mRenderThreadID, RENDER_THREAD_SAVE_STABILITY, 0, reinterpret_cast<LPARAM>(boardFilenamePtr.release()));
		}
		break;
	}
	case MENU_OPEN_BOARD:
	{
		std::wstring boardFilename = L"Stability.png";

		FileDialog fileDialog;
		if (fileDialog.GetFilenameToOpen(mMainWindowHandle, boardFilename))
		{
			std::unique_ptr<std::wstring> boardFilenamePtr = std::make_unique<std::wstring>(boardFilename);
			PostThreadMessage(mRenderThreadID, RENDER_THREAD_LOAD_BOARD, 0, reinterpret_cast<LPARAM>(boardFilenamePtr.release()));
		}
		break;
	}
	case MENU_HIDE_CLICK_RULE_GRID:
	{
		mRenderer->SetClickRuleGridVisible(false);
		break;
	}
	case MENU_SHOW_CLICK_RULE_GRID:
	{
		mRenderer->SetClickRuleGridVisible(true);
		break;
	}
	case MENU_RESET_CLICK_RULE:
	{
		mFractalGen->InitDefaultClickRule();
		break;
	}
	case MENU_INITIAL_STATE_CORNERS:
	{
		mResetMode = ResetBoardModeApp::RESET_4_CORNERS;
		break;
	}
	case MENU_INITIAL_STATE_SIDES:
	{
		mResetMode = ResetBoardModeApp::RESET_4_SIDES;
		break;
	}
	case MENU_INITIAL_STATE_CENTER:
	{
		mResetMode = ResetBoardModeApp::RESET_CENTER;
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
		OnCommandReset();
		break;
	}
	case 'P':
	{
		OnCommandPause();
		break;
	}
	case 'N':
	{
		OnCommandNextFrame();
		break;
	}
	case 'S':
	{
		OnCommandStop();
		break;
	}
	case VK_OEM_PLUS:
	{
		IncreaseBoardSize();
		break;
	}
	case VK_OEM_MINUS:
	{
		DecreaseBoardSize();
		break;
	}
	case '1':
	{
		mResetMode = ResetBoardModeApp::RESET_4_CORNERS;
		break;
	}
	case '2':
	{
		mResetMode = ResetBoardModeApp::RESET_4_SIDES;
		break;
	}
	case '3':
	{
		mResetMode = ResetBoardModeApp::RESET_CENTER;
		break;
	}
	default:
	{
		break;
	}
	}

	return 0;
}

void WindowApp::OnCommandReset()
{
	//Start the simulation again

	mFinalFrameNumber = ParseFinalFrame();
	mSpawnPeriod      = ParseSpawnPeriod();

	mLogger->WriteToLog(L"Spawn period: " + std::to_wstring(mSpawnPeriod));

	mFractalGen->SetSpawnPeriod(mSpawnPeriod);
	mFractalGen->SetUseSmooth(mUseSmoothTransform);

	mPlayMode = PlayMode::MODE_CONTINUOUS_FRAMES;
	PostThreadMessage(mRenderThreadID, RENDER_THREAD_REINIT, 0, 0);
	mRenderer->NeedRedraw();

	EnableWindow(mSizeTrackbar,        FALSE);
	EnableWindow(mVideoFramesCheckBox, FALSE);
	EnableWindow(mLastFrameLabel,      FALSE);
	EnableWindow(mLastFrameTextBox,    FALSE);
	EnableWindow(mSpawnLabel,          FALSE);
	EnableWindow(mSpawnTextBox,        FALSE);
	EnableWindow(mSmoothCheckBox,      FALSE);
}

void WindowApp::OnCommandPause()
{
	if(mPlayMode != PlayMode::MODE_STOP)
	{
		if(mPlayMode == PlayMode::MODE_PAUSED)
		{
			//The simulation is paused, continue it

			mPlayMode = PlayMode::MODE_CONTINUOUS_FRAMES;

			SetWindowText(mButtonPausePlay, L"⏸");
		}
		else
		{
			//The simulation is running, pause it

			mPlayMode = PlayMode::MODE_PAUSED;

			SetWindowText(mButtonPausePlay, L"▶");
		}
	}
	else
	{
		//The simulation is stopped, start it again

		mFinalFrameNumber = ParseFinalFrame();
		mSpawnPeriod      = ParseSpawnPeriod();

		mLogger->WriteToLog(L"Spawn period: " + std::to_wstring(mSpawnPeriod));

		mFractalGen->SetSpawnPeriod(mSpawnPeriod);
		mFractalGen->SetUseSmooth(mUseSmoothTransform);

		PostThreadMessage(mRenderThreadID, RENDER_THREAD_REINIT, 0, 0);
		mPlayMode = PlayMode::MODE_CONTINUOUS_FRAMES;

		SetWindowText(mButtonPausePlay, L"⏸");

		EnableWindow(mSizeTrackbar,        FALSE);
		EnableWindow(mVideoFramesCheckBox, FALSE);
		EnableWindow(mLastFrameLabel,      FALSE);
		EnableWindow(mLastFrameTextBox,    FALSE);
		EnableWindow(mSpawnLabel,          FALSE);
		EnableWindow(mSpawnTextBox,        FALSE);
		EnableWindow(mSmoothCheckBox,      FALSE);
	}
}

void WindowApp::OnCommandStop()
{
	if(mPlayMode == PlayMode::MODE_STOP)
	{
		//The simulation is stopped, start it again

		mFinalFrameNumber = ParseFinalFrame();
		mSpawnPeriod      = ParseSpawnPeriod();

		mLogger->WriteToLog(L"Spawn period: " + std::to_wstring(mSpawnPeriod));

		mFractalGen->SetSpawnPeriod(mSpawnPeriod);
		mFractalGen->SetUseSmooth(mUseSmoothTransform);

		PostThreadMessage(mRenderThreadID, RENDER_THREAD_REINIT, 0, 0);
		mPlayMode = PlayMode::MODE_CONTINUOUS_FRAMES;

		EnableWindow(mSizeTrackbar,        FALSE);
		EnableWindow(mVideoFramesCheckBox, FALSE);
		EnableWindow(mLastFrameLabel,      FALSE);
		EnableWindow(mLastFrameTextBox,    FALSE);
		EnableWindow(mSpawnLabel,          FALSE);
		EnableWindow(mSpawnTextBox,        FALSE);
		EnableWindow(mSmoothCheckBox,      FALSE);

		SetWindowText(mButtonPausePlay, L"⏸");
	}
	else
	{
		//The simulation is running or paused, stop it

		mPlayMode = PlayMode::MODE_STOP;

		EnableWindow(mSizeTrackbar,        TRUE);
		EnableWindow(mVideoFramesCheckBox, TRUE);
		EnableWindow(mLastFrameLabel,      TRUE);
		EnableWindow(mLastFrameTextBox,    TRUE);
		EnableWindow(mSpawnLabel,          TRUE);
		EnableWindow(mSpawnTextBox,        TRUE);

		uint32_t spawn = ParseSpawnPeriod();
		if(spawn != 0)
		{
			EnableWindow(mSmoothCheckBox, TRUE);
		}
		else
		{
			EnableWindow(mSmoothCheckBox, FALSE);
		}

		SetWindowText(mButtonPausePlay, L"▶");
	}
}

void WindowApp::OnCommandNextFrame()
{
	if(mPlayMode == PlayMode::MODE_PAUSED)
	{
		//The simulation is paused, calculate just one frame

		mPlayMode = PlayMode::MODE_SINGLE_FRAME;
	}
}

void WindowApp::IncreaseBoardSize()
{
	if(mPlayMode == PlayMode::MODE_STOP)
	{
		const uint32_t maxSize = (1 << 14) - 1;

		uint32_t currWidth  = mFractalGen->GetWidth();
		uint32_t currHeight = mFractalGen->GetHeight();

		//W = H = 2^p - 1. We need to increase p
		uint32_t newWidth  = (currWidth  + 1) * 2 - 1;
		uint32_t newHeight = (currHeight + 1) * 2 - 1;

		//Clamp the values
		if(newWidth > maxSize)
		{
			newWidth = maxSize;
		}

		if(newHeight > maxSize)
		{
			newHeight = maxSize;
		}

		int psize = (int)log2f((newWidth + 1));
		PostMessage(mSizeTrackbar, TBM_SETPOS, TRUE, psize);

		PostThreadMessage(mRenderThreadID, RENDER_THREAD_RESIZE_BOARD, newWidth, newHeight);
	}
}

void WindowApp::DecreaseBoardSize()
{
	if(mPlayMode == PlayMode::MODE_STOP)
	{
		const uint32_t minSize = (1 << 2) - 1;

		uint32_t currWidth  = mFractalGen->GetWidth();
		uint32_t currHeight = mFractalGen->GetHeight();

		//W = H = 2^p - 1. We need to decrease p
		uint32_t newWidth  = (currWidth  + 1) / 2 - 1;
		uint32_t newHeight = (currHeight + 1) / 2 - 1;

		//Clamp the values
		if(newWidth < minSize)
		{
			newWidth = minSize;
		}

		if(newHeight < minSize)
		{
			newHeight = minSize;
		}

		int psize = (int)log2f((newWidth + 1));
		PostMessage(mSizeTrackbar, TBM_SETPOS, TRUE, psize);

		PostThreadMessage(mRenderThreadID, RENDER_THREAD_RESIZE_BOARD, newWidth, newHeight);
	}
}