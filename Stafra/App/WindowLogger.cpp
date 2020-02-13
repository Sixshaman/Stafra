#include "WindowLogger.h"

WindowLogger::WindowLogger(HWND logAreaHandle): mLogTextAreaHandle(logAreaHandle)
{
	mCharacterLimit = SendMessage(mLogTextAreaHandle, EM_GETLIMITTEXT, 0, 0);
}

WindowLogger::~WindowLogger()
{
}

void WindowLogger::WriteToLog(const std::wstring& message)
{
	int endPos = GetWindowTextLength(mLogTextAreaHandle);

	SendMessageW(mLogTextAreaHandle, EM_SETSEL,     endPos, endPos);
	SendMessageW(mLogTextAreaHandle, EM_REPLACESEL, FALSE,  (LPARAM)(message + L"\r\n").c_str());
}

void WindowLogger::WriteToLog(const std::string& message)
{
	int endPos = GetWindowTextLength(mLogTextAreaHandle);

	SendMessageA(mLogTextAreaHandle, EM_SETSEL, endPos, endPos);
	SendMessageA(mLogTextAreaHandle, EM_REPLACESEL, FALSE, (LPARAM)(message + "\r\n").c_str());
}
