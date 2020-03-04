#pragma once

#include "Logger.hpp"
#include <Windows.h>

class WindowLogger : public Logger
{
public:
	WindowLogger(HWND mainWindow);
	~WindowLogger();

	void WriteToLog(const std::wstring& message) override;
	void WriteToLog(const std::string& message)  override;

	void Block()   override;
	void Unblock() override;

	void Flush() override;

private:
	HWND         mMainWindowHandle;
	std::wstring mMessageBufferW;

	bool             mLastMessageProcessed;
	CRITICAL_SECTION mMessageCriticalSection;

	uint32_t mSentMessageCount;
};