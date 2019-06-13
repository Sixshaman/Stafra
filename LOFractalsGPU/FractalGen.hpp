#pragma once

//http://lightstrout.com/index.php/2019/05/31/stability-fractal/

#include <d3d11.h>
#include <DirectXMath.h>
#include <Windows.h>
#include <wrl/client.h>
#include <cstdint>
#include <string>
#include <vector>

struct CBParamsStruct
{
	DirectX::XMUINT2 BoardSize;
	DirectX::XMUINT2 RealTileSize;
};

class FractalGen
{
public:
	FractalGen(uint32_t pow2Size);
	~FractalGen();

	void ComputeFractal();
	void SaveFractalImage(const std::string& filename);

private:
	void CreateTextures();
	void CreateShaderData();

	void InitTextures();
	void StabilityNextStep();
	bool CompareWithInitialState();

	void     CopyStabilityTextureData(std::vector<uint8_t>& stabilityData);
	uint32_t GetEqualityBufferData();

	void CreateDefaultInitialBoard();
	bool LoadInitialState();

	void CheckDeviceLost();

	void DownscalePicture(uint32_t newWidth);

private:
	Microsoft::WRL::ComPtr<ID3D11Device>        mDevice;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> mDeviceContext;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> mDownscaledStabilityTex;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> mDownscaledStabilityTexCopy;

	//Ping-pong in the "Next stability" shader
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  mPrevStabilitySRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  mCurrStabilitySRV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> mPrevStabilityUAV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> mCurrStabilityUAV;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  mPrevBoardSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  mCurrBoardSRV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> mPrevBoardUAV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> mCurrBoardUAV;

	//For comparison
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  mInitialBoardSRV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> mInitialBoardUAV;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  mPrevEqualitySRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  mNextEqualitySRV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> mPrevEqualityUAV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> mNextEqualityUAV;

	Microsoft::WRL::ComPtr<ID3D11Buffer>               mEqualityBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer>               mEqualityBufferCopy;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>  mEqualityBufferUAV;

	//For the final downscaling
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  mFinalStateSRV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> mFinalStateUAV;

	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mDownscaledStateRTV;

	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mClear4CornersShader;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mInitialStateTransformShader;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mFinalStateTransformShader;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mCompareBoardsStartShader;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mCompareBoardsShrinkShader;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mCompareBoardsFinalShader;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mStabilityNextStepShader;
	Microsoft::WRL::ComPtr<ID3D11VertexShader>  mDownscaleVertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>   mDownscalePixelShader;

	Microsoft::WRL::ComPtr<ID3D11SamplerState> mBestSamplerEver;

	Microsoft::WRL::ComPtr<ID3D11Buffer> mCBufferParams;
	CBParamsStruct                       mCBufferParamsCopy;

	D3D11_VIEWPORT mViewport;

	uint32_t mSizeLo;
};