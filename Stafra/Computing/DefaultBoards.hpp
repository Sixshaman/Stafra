#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <string>

/*
The class for creating initial states.
Input:               Desired width, desired height
Output:              ID3D11Texture2D with initial board and D3D11_BIND_SHADER_RESOURCE bind flag
Possible expansions: More different initial state configurations (1 mClear*Shader shader, 1 enum entry and 1 private InitialState*() method for each one)
*/

enum class BoardClearMode
{
	FOUR_CORNERS,
	FOUR_SIDES,
	CENTER
};

class DefaultBoards
{
	struct CBParamsStruct
	{
		DirectX::XMUINT2 BoardSize;
	};

public:
	DefaultBoards(ID3D11Device* device);
	~DefaultBoards();

	void CreateBoard(ID3D11Device* device, ID3D11DeviceContext* dc, uint32_t texWidth, uint32_t texHeight, BoardClearMode clearMode, ID3D11Texture2D** outBoardTex);

private:
	void LoadShaderData(ID3D11Device* device);

	void InitialState4Corners(ID3D11DeviceContext* dc, ID3D11UnorderedAccessView* boardUAV, uint32_t width, uint32_t height);
	void InitialState4Sides(ID3D11DeviceContext* dc,   ID3D11UnorderedAccessView* boardUAV, uint32_t width, uint32_t height);
	void InitialStateCenter(ID3D11DeviceContext* dc,   ID3D11UnorderedAccessView* boardUAV, uint32_t width, uint32_t height);

private:
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mClear4CornersShader;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mClear4SidesShader;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mClearCenterShader;

	Microsoft::WRL::ComPtr<ID3D11Buffer> mCBufferParams;
	CBParamsStruct                       mCBufferParamsCopy;
};