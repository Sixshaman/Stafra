#include "WindowLogger.h"

WindowLogger::WindowLogger(DWORD mainThreadId): mMainThreadId(mainThreadId)
{
}

WindowLogger::~WindowLogger()
{
}

void WindowLogger::WriteToLog(const std::wstring& message)
{
	uint32_t currMessageLength = mMessageBufferW.length();
	mMessageBufferW           += message + L"\r\n";
	
	PostThreadMessageW(mMainThreadId, WM_APP + 2, 0, (LPARAM)(mMessageBufferW.c_str() + currMessageLength));
}

void WindowLogger::WriteToLog(const std::string& message)
{
	uint32_t currMessageLength = mMessageBufferA.length();
	mMessageBufferA           += message + "\r\n";
	
	PostThreadMessageW(mMainThreadId, WM_APP + 1, 0, (LPARAM)(mMessageBufferA.c_str() + currMessageLength));
}
