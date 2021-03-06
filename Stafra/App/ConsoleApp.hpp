#pragma once

#define OEMRESOURCE

#include <Windows.h>
#include <memory>
#include "StafraApp.hpp"

class ConsoleApp: public StafraApp
{
public:
	ConsoleApp(const CommandLineArguments& cmdArgs);
	~ConsoleApp();

	void ComputeFractal();

private:
	void Init(const CommandLineArguments& cmdArgs);

	void InitRenderer(const CommandLineArguments& args) override;
	void InitLogger(const CommandLineArguments& args)   override;
};