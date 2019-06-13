#pragma once

//http://lightstrout.com/index.php/2019/05/31/stability-fractal/

#include <d3d11.h>

#include <Windows.h>
#include <wrl/client.h>
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include "Downscaler.hpp"

class FractalGen
{
public:
	FractalGen(uint32_t pow2Size);
	~FractalGen();

	void ComputeFractal(bool SaveVideoFrames);
	void SaveFractalImage(const std::string& filename);

private:
	void CreateTextures();
	void CreateShaderData();

	void InitTextures();
	void StabilityNextStep();

	void CheckDeviceLost();

private:
	Microsoft::WRL::ComPtr<ID3D11Device>        mDevice;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> mDeviceContext;

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

	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mStabilityNextStepShader;

	std::unique_ptr<Downscaler> mDownscaler;

	uint32_t mSizeLo;
};