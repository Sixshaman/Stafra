#pragma once

//http://lightstrout.com/index.php/2019/05/31/stability-fractal/

#include <d3d11.h>
#include <DirectXMath.h>
#include <Windows.h>
#include <wrl/client.h>
#include <cstdint>
#include <string>
#include <vector>

class FractalGen
{
	struct CBParamsStruct
	{
		DirectX::XMUINT2 BoardSize;
	};

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

	void CopyStabilityTextureData(std::vector<uint8_t>& stabilityData);

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

	//For the final downscaling
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  mFinalStateSRV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> mFinalStateUAV;

	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mDownscaledStateRTV;

	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mClear4CornersShader;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mInitialStateTransformShader;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mFinalStateTransformShader;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mStabilityNextStepShader;
	Microsoft::WRL::ComPtr<ID3D11VertexShader>  mDownscaleVertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>   mDownscalePixelShader;

	Microsoft::WRL::ComPtr<ID3D11SamplerState> mBestSamplerEver;

	Microsoft::WRL::ComPtr<ID3D11Buffer> mCBufferParams;
	CBParamsStruct                       mCBufferParamsCopy;

	D3D11_VIEWPORT mViewport;

	uint32_t mSizeLo;
};