#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <cstdint>

/*
The class for transforming cell stability values to grayscale colors.
Input:               ID3D11ShaderResourceView containing stability values (possibly with encoded spawn periods)
Output:              ID3D11ShaderResourceView with floating-point values that equal 1.0f for "stable", 0.0f for "unstable" and possible in-between values for different spawn stability
Possible expansions: None ATM
*/

class FinalTransformer
{
	struct CBParamsStruct
	{
		uint32_t SpawnPeriod;
	};

public:
	FinalTransformer(ID3D11Device* device);
	~FinalTransformer();

	void PrepareForTransform(ID3D11Device* device, uint32_t width, uint32_t height);
	void ComputeTransform(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* srv, uint32_t spawnPeriod, bool useSmooth);

	ID3D11ShaderResourceView* GetTransformedSRV() const;

private:
	void ReinitTextures(ID3D11Device* device, uint32_t width, uint32_t height);
	void LoadShaderData(ID3D11Device* device);

	void FinalStateTransform(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* srv, uint32_t oldWidth, uint32_t oldHeight);
	void FinalStateTransformSmooth(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* srv, uint32_t spawnPeriod, uint32_t oldWidth, uint32_t oldHeight);

private:
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  mFinalStateSRV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> mFinalStateUAV;

	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mFinalStateTransformShader;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mFinalStateTransformSmoothShader;

	Microsoft::WRL::ComPtr<ID3D11Buffer> mCBufferParams;
	CBParamsStruct                       mCBufferParamsCopy;

	uint32_t mBoardWidth;
	uint32_t mBoardHeight;
};