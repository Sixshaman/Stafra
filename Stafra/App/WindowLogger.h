#pragma once

#include "Logger.hpp"
#include <Windows.h>

class WindowLogger : public Logger
{
public:
	WindowLogger(DWORD mainThreadId);
	~WindowLogger();

	void WriteToLog(const std::wstring& message) override;
	void WriteToLog(const std::string& message)  override;

private:
	DWORD mMainThreadId;

	std::wstring mMessageBufferW;
	std::string  mMessageBufferA;
};