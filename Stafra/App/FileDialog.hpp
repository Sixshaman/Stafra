#ifndef FILEDIALOG_HPP
#define FILEDIALOG_HPP

#include <Windows.h>
#include <string>
#include <ShObjIdl.h>

/*
 * Class for save file dialog. 
 *
 */

class FileDialog
{
public:
	bool GetFilenameToSave(HWND hwnd, std::wstring& outFilename);
	bool GetFilenameToOpen(HWND hwnd, std::wstring& outFilename);

private:
	bool ShowFileDialog(HWND hwnd, const std::wstring& dialogTitle, const COMDLG_FILTERSPEC *fileTypes, uint32_t fileTypesSize, const CLSID dialogType, std::wstring& outFilename);
};

#endif FILEDIALOG_HPP