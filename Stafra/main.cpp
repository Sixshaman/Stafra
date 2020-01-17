#include "App/WindowApp.hpp"
#include "App/CommandLineArguments.hpp"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	CommandLineArguments cmdArgs(lpCmdLine);
	cmdArgs.ParseArgs();

	WindowApp app(hInstance, cmdArgs);
	return app.Run();
}