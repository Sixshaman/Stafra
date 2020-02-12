#include "ConsoleLogger.hpp"
#include <iostream>

ConsoleLogger::ConsoleLogger()
{

}

ConsoleLogger::~ConsoleLogger()
{
}

void ConsoleLogger::WriteToLog(const std::wstring& message)
{
	std::wcout << message << std::endl;
}

void ConsoleLogger::WriteToLog(const std::string& message)
{
	std::cout << message << std::endl;
}
