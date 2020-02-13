#pragma once

#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <string>
#include "..\Util.hpp"

/*
The class for loading initial state from file.
Input:               Filename
Output#1:            ID3D11Texture2D with initial board and D3D11_BIND_SHADER_RESOURCE bind flag. The cell values for the board are based on the pixel luminance (threshold: 0.15)
Output#2:            ID3D11Texture2D with click rule and D3D11_BIND_SHADER_RESOURCE bind flag. The cell values for the click rule are based on the pixel luminance (threshold: 0.15)
Possible expansions: Different thresholds for luminance and different bases for cell values
*/

class BoardLoader
{
public:
	BoardLoader(ID3D11Device* device);
	~BoardLoader();

	Utils::BoardLoadError LoadBoardFromFile(ID3D11Device* device, ID3D11DeviceContext* dc, const std::wstring& filename, ID3D11Texture2D** outBoardTex);
	Utils::BoardLoadError LoadClickRuleFromFile(ID3D11Device* device, ID3D11DeviceContext* dc, const std::wstring& filename, ID3D11Texture2D** outClickRule);

private:
	void LoadShaderData(ID3D11Device* device);

	void InitialStateTransform(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* textureSRV, ID3D11UnorderedAccessView* boardUAV, uint32_t width, uint32_t height);

private:
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mInitialStateTransformShader;
};