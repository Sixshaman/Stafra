#include "Downscaler.hpp"
#include "../Util.hpp"
#include <d3dcompiler.h>

Downscaler::Downscaler(ID3D11Device* device): mDownscaledWidth(1024), mDownscaledHeight(1024)
{
	LoadShaderData(device);
}

Downscaler::~Downscaler()
{
}

void Downscaler::PrepareForDownscaling(ID3D11Device* device, uint32_t downscaleWidth, uint32_t downscaleHeight)
{
	mDownscaledWidth  = downscaleWidth;
	mDownscaledHeight = downscaleHeight;
	ReinitTextures(device, downscaleWidth, downscaleHeight);

	mDownscaleViewport.TopLeftX = 0.0f;
	mDownscaleViewport.TopLeftY = 0.0f;
	mDownscaleViewport.Width    = (float)downscaleWidth;
	mDownscaleViewport.Height   = (float)downscaleHeight;
	mDownscaleViewport.MinDepth = 0.0f;
	mDownscaleViewport.MaxDepth = 1.0f;
}

void Downscaler::DownscalePicture(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* srv)
{
	ActualDownscaling(dc, srv);
}

ID3D11ShaderResourceView* Downscaler::GetDownscaledSRV() const
{
	return mDownscaledStateSRV.Get();
}

void Downscaler::ReinitTextures(ID3D11Device* device, uint32_t downscaleWidth, uint32_t downscaleHeight)
{
	mDownscaledStateSRV.Reset();
	mDownscaledStateRTV.Reset();

	D3D11_TEXTURE2D_DESC downscaledPictureTexDesc;
	downscaledPictureTexDesc.Width              = downscaleWidth;
	downscaledPictureTexDesc.Height             = downscaleHeight;
	downscaledPictureTexDesc.Format             = DXGI_FORMAT_R32_FLOAT;
	downscaledPictureTexDesc.Usage              = D3D11_USAGE_DEFAULT;
	downscaledPictureTexDesc.BindFlags          = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	downscaledPictureTexDesc.CPUAccessFlags     = 0;
	downscaledPictureTexDesc.ArraySize          = 1;
	downscaledPictureTexDesc.MipLevels          = 1;
	downscaledPictureTexDesc.SampleDesc.Count   = 1;
	downscaledPictureTexDesc.SampleDesc.Quality = 0;
	downscaledPictureTexDesc.MiscFlags          = 0;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> downscaledTex;
	ThrowIfFailed(device->CreateTexture2D(&downscaledPictureTexDesc, nullptr, downscaledTex.GetAddressOf()));
	
	D3D11_SHADER_RESOURCE_VIEW_DESC downscaledSRVDesc;
	downscaledSRVDesc.Format                    = DXGI_FORMAT_R32_FLOAT;
	downscaledSRVDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
	downscaledSRVDesc.Texture2D.MipLevels       = 1;
	downscaledSRVDesc.Texture2D.MostDetailedMip = 0;

	ThrowIfFailed(device->CreateShaderResourceView(downscaledTex.Get(), &downscaledSRVDesc, mDownscaledStateSRV.GetAddressOf()));

	D3D11_RENDER_TARGET_VIEW_DESC downscaledRTVDesc;
	downscaledRTVDesc.Format             = DXGI_FORMAT_R32_FLOAT;
	downscaledRTVDesc.ViewDimension      = D3D11_RTV_DIMENSION_TEXTURE2D;
	downscaledRTVDesc.Texture2D.MipSlice = 0;

	ThrowIfFailed(device->CreateRenderTargetView(downscaledTex.Get(), &downscaledRTVDesc, mDownscaledStateRTV.GetAddressOf()));
}

void Downscaler::LoadShaderData(ID3D11Device* device)
{
	const std::wstring shaderDir = Utils::GetShaderPath() + L"Downscale\\";
	Utils::LoadShaderFromFile(device, shaderDir + L"DownscaleBigPictureVS.cso", mDownscaleVertexShader.GetAddressOf());
	Utils::LoadShaderFromFile(device, shaderDir + L"DownscaleBigPicturePS.cso", mDownscalePixelShader.GetAddressOf());

	D3D11_SAMPLER_DESC samDesc;
	ZeroMemory(&samDesc, sizeof(D3D11_SAMPLER_DESC));
	samDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samDesc.Filter   = D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;

	ThrowIfFailed(device->CreateSamplerState(&samDesc, mBestSamplerEver.GetAddressOf()));
}

void Downscaler::ActualDownscaling(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* srv)
{
	ID3D11Buffer* vertexBuffers[] = {nullptr};
	UINT          strides[]       = {0};
	UINT          offsets[]       = {0};

	dc->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
	dc->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);

	dc->IASetInputLayout(nullptr);
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	dc->VSSetShader(mDownscaleVertexShader.Get(), nullptr, 0);

	ID3D11ShaderResourceView* downscaleSRVs[] = {srv};
	dc->PSSetShaderResources(0, 1, downscaleSRVs);

	ID3D11SamplerState* samplers[] = { mBestSamplerEver.Get() };
	dc->PSSetSamplers(0, 1, samplers);

	dc->PSSetShader(mDownscalePixelShader.Get(), nullptr, 0);

	ID3D11RenderTargetView* downscaleRtvs[] = { mDownscaledStateRTV.Get() };
	dc->OMSetRenderTargets(1, downscaleRtvs, nullptr);

	D3D11_VIEWPORT viewports[] = { mDownscaleViewport };
	dc->RSSetViewports(1, viewports);

	FLOAT clearColor[] = {0.0f, 0.0f, 0.0f, 0.0f};
	dc->ClearRenderTargetView(mDownscaledStateRTV.Get(), clearColor);

	dc->Draw(4, 0);

	dc->VSSetShader(nullptr, nullptr, 0);
	dc->PSSetShader(nullptr, nullptr, 0);

	ID3D11ShaderResourceView* nullSRVs[] = {nullptr};
	ID3D11SamplerState* nullSamplers[]   = {nullptr};

	dc->PSSetSamplers(0, 1, nullSamplers);
	dc->PSSetShaderResources(0, 1, nullSRVs);

	ID3D11RenderTargetView* nullRTV[] = {nullptr};
	dc->OMSetRenderTargets(1, nullRTV, nullptr);
}
