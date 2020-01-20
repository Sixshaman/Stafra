#include "FileDialog.hpp"
#include "..\Util.hpp"
#include <KnownFolders.h>
#include <wrl\client.h>

bool FileDialog::GetFilenameToSave(HWND wnd, std::wstring& outFilename)
{
	const COMDLG_FILTERSPEC fileTypes[] =
	{
		{L"PNG image(*.png)", L"*.png;"},
		{L"All Files (*.*)",  L"*.*"}
	};

	if(!ShowFileDialog(wnd, L"Save PNG image", fileTypes, ARRAYSIZE(fileTypes), CLSID_FileSaveDialog, outFilename))
	{
		return false;
	}

	return true;
}

bool FileDialog::GetFilenameToOpen(HWND hwnd, std::wstring& outFilename)
{
	const COMDLG_FILTERSPEC fileTypes[] =
	{
		{L"PNG image(*.png)", L"*.png;"},
		{L"All Files (*.*)", L"*.*"}
	};

	if(!ShowFileDialog(hwnd, L"Open PNG image", fileTypes, ARRAYSIZE(fileTypes), CLSID_FileOpenDialog, outFilename))
	{
		return false;
	}

	return true;
}

bool FileDialog::ShowFileDialog(HWND hwnd, const std::wstring& dialogTitle, const COMDLG_FILTERSPEC* fileTypes, uint32_t fileTypesSize, const CLSID dialogType, std::wstring& outFilename)
{
	Microsoft::WRL::ComPtr<IFileDialog> pFileDialog;
	ThrowIfFailed(CoCreateInstance(dialogType, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileDialog)));

	if(dialogType == CLSID_FileOpenDialog)
	{
		Microsoft::WRL::ComPtr<IShellItem> pShellFolder;
		ThrowIfFailed(SHCreateItemInKnownFolder(FOLDERID_Pictures, 0, nullptr, IID_PPV_ARGS(&pShellFolder)));
		ThrowIfFailed(pFileDialog->SetFolder(pShellFolder.Get()));
	}

	ThrowIfFailed(pFileDialog->SetTitle(dialogTitle.c_str()));
	ThrowIfFailed(pFileDialog->SetFileTypes(fileTypesSize, fileTypes));
	ThrowIfFailed(pFileDialog->SetFileTypeIndex(1));
	ThrowIfFailed(pFileDialog->SetFileName(outFilename.c_str()));

	DWORD options = 0;
	ThrowIfFailed(pFileDialog->GetOptions(&options));
	if(dialogType == CLSID_FileSaveDialog)
	{
		options = options | FOS_OVERWRITEPROMPT;
	}

	ThrowIfFailed(pFileDialog->SetOptions(options));

	HRESULT hr = pFileDialog->Show(hwnd);
	if(hr != HRESULT_FROM_WIN32(ERROR_CANCELLED))
	{	
		ThrowIfFailed(hr);
	}
	else
	{
		return false;
	}

	Microsoft::WRL::ComPtr<IShellItem> pShellResult;
	ThrowIfFailed(pFileDialog->GetResult(&pShellResult));

	PWSTR filePath = nullptr;
	ThrowIfFailed(pShellResult->GetDisplayName(SIGDN_FILESYSPATH, &filePath));

	outFilename = std::wstring(filePath);
	CoTaskMemFree(filePath);

	if(outFilename.size() <= 3 || outFilename.substr(outFilename.length() - 4, 4) != L".png")
	{
		outFilename += L".png";
	}

	return true;
}