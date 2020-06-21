#pragma once

#include <string>
#include <vector>

enum class CmdParseResult
{
	PARSE_OK,
	PARSE_HELP,
	PARSE_WRONG_PSIZE,
	PARSE_WRONG_FINAL_FRAME,
	PARSE_WRONG_SPAWN,
	PARSE_WRONG_RESET_MODE,
	PARSE_SILENT,
	PARSE_UNKNOWN_OPTION
};

//Board reset mode from cmd
enum class CmdResetMode
{
	RESET_4_CORNERS,
	RESET_4_SIDES,
	RESET_CENTER
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

	uint32_t PowSize()     const;
	uint32_t FinalFrame()  const;
	uint32_t SpawnPeriod() const;

	bool HelpOnly()        const;
	bool SaveVideoFrames() const;
	bool SmoothTransform() const;
	bool SilentMode()      const;

	CmdResetMode ResetMode() const;

private:
	CommandLineArguments();

	uint32_t ParseInt(std::string intStr, uint32_t min, uint32_t max);

private:
	std::vector<std::string> mCmdLineArgs;

	uint32_t mPowSize;
	uint32_t mFinalFrame;
	uint32_t mSpawnPeriod;

	bool mHelpOnly;
	bool mSaveVideoFrames;
	bool mSmoothTransform;
	bool mSilentMode;

	CmdResetMode mResetMode;
};