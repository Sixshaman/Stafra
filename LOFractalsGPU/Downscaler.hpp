#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <vector>
#include <cstdint>

class Downscaler
{
public:
	Downscaler(ID3D11Device* device, uint32_t oldWidth, uint32_t oldHeight, uint32_t downscaleWidth, uint32_t downscaleHeight);
	~Downscaler();

	void DownscalePicture(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* srv, uint32_t oldWidth, uint32_t oldHeight);

	void CopyDownscaledTextureData(ID3D11DeviceContext* dc, std::vector<uint8_t>& downscaledData, uint32_t& downscaledWidth, uint32_t& downscaledHeight, uint32_t& rowPitch);

private:
	void CreateTextures(ID3D11Device* device, uint32_t oldWidth, uint32_t oldHeight, uint32_t downscaleWidth, uint32_t downscaleHeight);
	void LoadShaderData(ID3D11Device* device);

	void FinalStateTransform(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* srv, uint32_t oldWidth, uint32_t oldHeight);
	void ActualDownscaling(ID3D11DeviceContext* dc);

private:
	Microsoft::WRL::ComPtr<ID3D11Texture2D> mDownscaledStabilityTex;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> mDownscaledStabilityTexCopy;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  mFinalStateSRV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> mFinalStateUAV;

	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mDownscaledStateRTV;

	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mFinalStateTransformShader;
	Microsoft::WRL::ComPtr<ID3D11VertexShader>  mDownscaleVertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>   mDownscalePixelShader;

	Microsoft::WRL::ComPtr<ID3D11SamplerState> mBestSamplerEver;

	D3D11_VIEWPORT mDownscaleViewport;

	uint32_t mDownscaledWidth;
	uint32_t mDownscaledHeight;
};