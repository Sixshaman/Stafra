#pragma once

#include "Logger.hpp"

class ConsoleLogger: public Logger
{
public:
	ConsoleLogger();
	~ConsoleLogger();

	void WriteToLog(const std::wstring& message) override;
	void WriteToLog(const std::string&  message) override;

	void Block()   override;
	void Unblock() override;

	void Flush() override;
};