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
	PARSE_SILENT,
	PARSE_UNKNOWN_OPTION
};

class CommandLineArguments
{
public:
	CommandLineArguments(int argc, char* argv[]);
	CommandLineArguments(const std::string& cmdLine);
	~CommandLineArguments();

	CmdParseResult ParseArgs();

	std::string GetHelpMessage()                         const;
	std::string GetErrorMessage(CmdParseResult parseRes) const;

	uint32_t PowSize()       const;
	uint32_t Enlonging()     const;
	uint32_t SpawnPeriod()   const;
	bool   SaveVideoFrames() const;
	bool   SmoothTransform() const;
	bool   SilentMode()      const;

private:
	CommandLineArguments();

	uint32_t ParseInt(std::string intStr, uint32_t min, uint32_t max);

private:
	std::vector<std::string> mCmdLineArgs;

	uint32_t mPowSize;
	uint32_t mEnlonging;
	uint32_t mSpawnPeriod;
	bool     mSaveVideoFrames;
	bool     mSmoothTransform;
	bool     mSilentMode;
};