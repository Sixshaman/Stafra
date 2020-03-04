#include "WindowLogger.hpp"
#include "WindowConstants.hpp"

WindowLogger::WindowLogger(HWND mainWindow): mMainWindowHandle(mainWindow), mLastMessageProcessed(true), mSentMessageCount(0)
{
	InitializeCriticalSection(&mMessageCriticalSection);

	mMessageBufferW.reserve(4096); //Removing this will actually break everything when the internal std::wstring buffer reallocates (but we don't)
}

WindowLogger::~WindowLogger()
{
	DeleteCriticalSection(&mMessageCriticalSection);
}

void WindowLogger::WriteToLog(const std::wstring& message)
{
	EnterCriticalSection(&mMessageCriticalSection); //Ensure that we send only one message at a time

	//If the text area message queue is blocked (i.e. when scrolling), text update message won't be recieved.
	//Another problem: the log area can handle messages slower than we can send them.
	//Solution: keep updating the internal message buffer until we're sure the logger area can process the message.
	if(mLastMessageProcessed)
	{
		mSentMessageCount++;
		mLastMessageProcessed = false;
		mMessageBufferW.clear();
	}
	
	mMessageBufferW += message + L"\r\n";

	PostMessage(mMainWindowHandle, MAIN_THREAD_APPEND_TO_LOG, mSentMessageCount, (LPARAM)(mMessageBufferW.c_str()));

	LeaveCriticalSection(&mMessageCriticalSection);
}

void WindowLogger::WriteToLog(const std::string& message)
{
	size_t requiredLen = MultiByteToWideChar(CP_UTF8, MB_COMPOSITE, message.c_str(), -1, nullptr, 0);
	
	std::wstring wMsg;
	wMsg.resize(requiredLen, L'\0');

	MultiByteToWideChar(CP_UTF8, MB_COMPOSITE, message.c_str(), -1, wMsg.data(), requiredLen);
	WriteToLog(wMsg);
}

void WindowLogger::Block()
{
	EnterCriticalSection(&mMessageCriticalSection);
}

void WindowLogger::Unblock()
{
	LeaveCriticalSection(&mMessageCriticalSection);
}

void WindowLogger::Flush()
{
	mLastMessageProcessed = true;
}