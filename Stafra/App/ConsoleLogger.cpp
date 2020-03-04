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

void ConsoleLogger::Block()
{
	//Don't need to block the console logger yet
}

void ConsoleLogger::Unblock()
{
	//Don't need to block the console logger yet
}

void ConsoleLogger::Flush() //We don't block console logger as for now
{
}
