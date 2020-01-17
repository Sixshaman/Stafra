#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <cstdint>

/*
The class for downscaling a picture.
Input:               ID3D11ShaderResourceView containing floating-point texel values and downscaled width and height
Output:              ID3D11ShaderResourceView containing (downscaled width) x (downscaled height) image
Possible expansions: None ATM
*/

class Downscaler
{
	struct CBParamsStruct
	{
		uint32_t SpawnPeriod;
	};

public:
	Downscaler(ID3D11Device* device);
	~Downscaler();

	void PrepareForDownscaling(ID3D11Device* device, uint32_t downscaleWidth, uint32_t downscaleHeight);
	void DownscalePicture(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* srv);

	ID3D11ShaderResourceView* GetDownscaledSRV() const;

private:
	void ReinitTextures(ID3D11Device* device, uint32_t downscaleWidth, uint32_t downscaleHeight);
	void LoadShaderData(ID3D11Device* device);

	void ActualDownscaling(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* srv);

private:
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mDownscaledStateSRV;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>   mDownscaledStateRTV;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> mDownscaleVertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>  mDownscalePixelShader;

	Microsoft::WRL::ComPtr<ID3D11SamplerState> mBestSamplerEver;

	D3D11_VIEWPORT mDownscaleViewport;

	uint32_t mDownscaledWidth;
	uint32_t mDownscaledHeight;
};