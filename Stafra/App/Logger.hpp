#pragma once

#include <string>

class Logger
{
public:
	Logger()          = default;
	virtual ~Logger() = default;

	virtual void WriteToLog(const std::wstring& message) = 0;
	virtual void WriteToLog(const std::string&  message) = 0;
	
	virtual void Block()   = 0;
	virtual void Unblock() = 0;

	virtual void Flush() = 0;
};
