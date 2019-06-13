#pragma once

#include <wrl/client.h>
#include <d3d11.h>
#include <cstdint>
#include <DirectXMath.h>

class EqualityChecker
{
	struct CBParamsStruct
	{
		DirectX::XMUINT2 BoardSize;
	};

public:
	EqualityChecker(ID3D11Device* device, uint32_t maxWidth, uint32_t maxHeight);
	~EqualityChecker();

	bool CheckEquality(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* left, ID3D11ShaderResourceView* right);

private:
	void CreateTextures(ID3D11Device* device);
	void LoadShaderData(ID3D11Device* device);

	void CreateEqualityTexture(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* left, ID3D11ShaderResourceView* right);
	void ShrinkEqualityTexture(ID3D11DeviceContext* dc, uint32_t curWidth, uint32_t curHeight, uint32_t& outNewWidth, uint32_t& outNewHeight);
	void FinalizeEqualityTexture(ID3D11DeviceContext* dc, uint32_t curWidth, uint32_t curHeight);

	uint32_t GetEqualityBufferData(ID3D11DeviceContext* dc);

private:
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  mPrevEqualitySRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  mNextEqualitySRV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> mPrevEqualityUAV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> mNextEqualityUAV;

	Microsoft::WRL::ComPtr<ID3D11Buffer>               mEqualityBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer>               mEqualityBufferCopy;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>  mEqualityBufferUAV;

	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mCompareBoardsStartShader;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mCompareBoardsShrinkShader;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mCompareBoardsFinalShader;

	Microsoft::WRL::ComPtr<ID3D11Buffer> mCBufferParams;
	CBParamsStruct                       mCBufferParamsCopy;

	uint32_t mMaxWidth;
	uint32_t mMaxHeight;
};