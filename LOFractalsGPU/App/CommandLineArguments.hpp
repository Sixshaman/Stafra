#pragma once

#include <string>
#include <vector>

enum class CmdParseResult
{
	PARSE_OK,
	PARSE_HELP,
	PARSE_WRONG_PSIZE,
	PARSE_WRONG_ENLONGING,
	PARSE_WRONG_SPAWN,
	PARSE_UNKNOWN_OPTION
};

class CommandLineArguments
{
public:
	CommandLineArguments(int argc, char* argv[]);
	~CommandLineArguments();

	CmdParseResult ParseArgs();

	std::string GetHelpMessage();
	std::string GetErrorMessage(CmdParseResult parseRes);

	size_t PowSize();
	size_t Enlonging();
	size_t SpawnPeriod();
	bool   SaveVideoFrames();
	bool   SmoothTransform();

private:
	size_t ParseInt(std::string intStr, size_t min, size_t max);

private:
	std::vector<std::string> mCmdLineArgs;

	size_t mPowSize;
	size_t mEnlonging;
	size_t mSpawnPeriod;
	bool   mSaveVideoFrames;
	bool   mSmoothTransform;
};