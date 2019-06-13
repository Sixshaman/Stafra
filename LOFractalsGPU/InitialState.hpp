#pragma once

#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <string>

enum class LoadError
{
	LOAD_SUCCESS,
	ERROR_CANT_READ_FILE,
	ERROR_WRONG_SIZE
};

class InitialState
{
	struct CBParamsStruct
	{
		DirectX::XMUINT2 BoardSize;
	};

public:
	InitialState(ID3D11Device* device);
	~InitialState();

	LoadError LoadFromFile(ID3D11Device* device, ID3D11DeviceContext* dc, const std::wstring& filename, ID3D11UnorderedAccessView* initialBoardUAV, uint32_t& texWidth, uint32_t& texHeight);
	void      CreateDefault(ID3D11DeviceContext* dc, ID3D11UnorderedAccessView* initialBoardUAV, uint32_t texWidth, uint32_t texHeight);

private:
	void LoadShaderData(ID3D11Device* device);

	void DefaultInitialState(ID3D11DeviceContext* dc, ID3D11UnorderedAccessView* initialBoardUAV, uint32_t width, uint32_t height);
	void InitialStateTransform(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* initialStateSRV, ID3D11UnorderedAccessView* initialBoardUAV, uint32_t width, uint32_t height);

private:
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mClear4CornersShader;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mInitialStateTransformShader;

	Microsoft::WRL::ComPtr<ID3D11Buffer> mCBufferParams;
	CBParamsStruct                       mCBufferParamsCopy;
};