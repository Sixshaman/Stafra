#pragma once

#include "Logger.hpp"
#include <Windows.h>

class WindowLogger : public Logger
{
public:
	WindowLogger(HWND logAreaHandle);
	~WindowLogger();

	void WriteToLog(const std::wstring& message) override;
	void WriteToLog(const std::string& message)  override;

private:
	HWND mLogTextAreaHandle;

	uint32_t mCharacterLimit;
};