#include "App/WindowApp.hpp"
#include "App/ConsoleApp.hpp"
#include "App/CommandLineArguments.hpp"

#include <iostream>

int main(int argc, char* argv[])
{
	CommandLineArguments cmdArgs(argc, argv);
	cmdArgs.ParseArgs();

	if(cmdArgs.SilentMode())
	{
		ConsoleApp app(cmdArgs);
		app.ComputeFractal();
		return 0;
	}
	else
	{
		fclose(stdin);
		fclose(stdout);
		fclose(stderr);

		FreeConsole();

		WindowApp app((HINSTANCE)GetModuleHandle(nullptr), cmdArgs);
		return app.Run();
	}
}