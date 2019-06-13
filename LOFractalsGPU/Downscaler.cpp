#include "Downscaler.hpp"
#include "Util.hpp"
#include <d3dcompiler.h>
#include <algorithm>

Downscaler::Downscaler(ID3D11Device* device, uint32_t downscaleWidth, uint32_t downscaleHeight): mDownscaledWidth(downscaleWidth), mDownscaledHeight(downscaleHeight)
{
	CreateTextures(device, downscaleWidth, downscaleHeight);
	LoadShaderData(device);
}

Downscaler::~Downscaler()
{
}

void Downscaler::DownscalePicture(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* srv, uint32_t oldWidth, uint32_t oldHeight)
{
	FinalStateTransform(dc, srv, oldWidth, oldHeight);

	dc->GenerateMips(mFinalStateSRV.Get());

	ActualDownscaling(dc);
}

void Downscaler::CopyDownscaledTextureData(ID3D11DeviceContext* dc, std::vector<uint8_t>& downscaledData, uint32_t& downscaledWidth, uint32_t& downscaledHeight, uint32_t& rowPitch)
{
	dc->CopyResource(mDownscaledStabilityTexCopy.Get(), mDownscaledStabilityTex.Get());

	downscaledData.clear();
	std::vector<float> textureData;

	D3D11_MAPPED_SUBRESOURCE mappedTex;
	ThrowIfFailed(dc->Map(mDownscaledStabilityTexCopy.Get(), 0, D3D11_MAP_READ, 0, &mappedTex));

	float* data = reinterpret_cast<float*>(mappedTex.pData);
	textureData.resize((size_t)mappedTex.RowPitch * mDownscaledHeight / sizeof(float));

	rowPitch = mappedTex.RowPitch / sizeof(float);

	std::copy(data, data + textureData.size(), textureData.begin());
	dc->Unmap(mDownscaledStabilityTexCopy.Get(), 0);

	downscaledData.resize(textureData.size());
	std::transform(textureData.begin(), textureData.end(), downscaledData.begin(), [](float val) {return (uint8_t)(val * 255.0f); });

	downscaledWidth  = mDownscaledWidth;
	downscaledHeight = mDownscaledHeight;
}

void Downscaler::CreateTextures(ID3D11Device* device, uint32_t downscaleWidth, uint32_t downscaleHeight)
{
	D3D11_TEXTURE2D_DESC finalPictureTexDesc;
	finalPictureTexDesc.Width              = mDownscaledWidth;
	finalPictureTexDesc.Height             = mDownscaledHeight;
	finalPictureTexDesc.Format             = DXGI_FORMAT_R32_FLOAT;
	finalPictureTexDesc.Usage              = D3D11_USAGE_DEFAULT;
	finalPictureTexDesc.BindFlags          = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_RENDER_TARGET;
	finalPictureTexDesc.CPUAccessFlags     = 0;
	finalPictureTexDesc.ArraySize          = 1;
	finalPictureTexDesc.MipLevels          = 1;
	finalPictureTexDesc.SampleDesc.Count   = 1;
	finalPictureTexDesc.SampleDesc.Quality = 0;
	finalPictureTexDesc.MiscFlags          = D3D11_RESOURCE_MISC_GENERATE_MIPS;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> finalTex = nullptr;
	ThrowIfFailed(device->CreateTexture2D(&finalPictureTexDesc, nullptr, finalTex.GetAddressOf()));

	D3D11_SHADER_RESOURCE_VIEW_DESC finalSrvDesc;
	finalSrvDesc.Format                    = DXGI_FORMAT_R32_FLOAT;
	finalSrvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
	finalSrvDesc.Texture2D.MipLevels       = 1;
	finalSrvDesc.Texture2D.MostDetailedMip = 0;

	ThrowIfFailed(device->CreateShaderResourceView(finalTex.Get(), &finalSrvDesc, mFinalStateSRV.GetAddressOf()));

	D3D11_UNORDERED_ACCESS_VIEW_DESC finalUavDesc;
	finalUavDesc.Format             = DXGI_FORMAT_R32_FLOAT;
	finalUavDesc.ViewDimension      = D3D11_UAV_DIMENSION_TEXTURE2D;
	finalUavDesc.Texture2D.MipSlice = 0;

	ThrowIfFailed(device->CreateUnorderedAccessView(finalTex.Get(), &finalUavDesc, mFinalStateUAV.GetAddressOf()));

	D3D11_TEXTURE2D_DESC downscaledPictureTexDesc;
	downscaledPictureTexDesc.Width              = downscaleWidth;
	downscaledPictureTexDesc.Height             = downscaleHeight;
	downscaledPictureTexDesc.Format             = DXGI_FORMAT_R32_FLOAT;
	downscaledPictureTexDesc.Usage              = D3D11_USAGE_DEFAULT;
	downscaledPictureTexDesc.BindFlags          = D3D11_BIND_RENDER_TARGET;
	downscaledPictureTexDesc.CPUAccessFlags     = 0;
	downscaledPictureTexDesc.ArraySize          = 1;
	downscaledPictureTexDesc.MipLevels          = 1;
	downscaledPictureTexDesc.SampleDesc.Count   = 1;
	downscaledPictureTexDesc.SampleDesc.Quality = 0;
	downscaledPictureTexDesc.MiscFlags          = 0;

	ThrowIfFailed(device->CreateTexture2D(&downscaledPictureTexDesc, nullptr, mDownscaledStabilityTex.GetAddressOf()));
	
	D3D11_RENDER_TARGET_VIEW_DESC downscaledRTVDesc;
	downscaledRTVDesc.Format             = DXGI_FORMAT_R32_FLOAT;
	downscaledRTVDesc.ViewDimension      = D3D11_RTV_DIMENSION_TEXTURE2D;
	downscaledRTVDesc.Texture2D.MipSlice = 0;

	ThrowIfFailed(device->CreateRenderTargetView(mDownscaledStabilityTex.Get(), &downscaledRTVDesc, mDownscaledStateRTV.GetAddressOf()));

	D3D11_TEXTURE2D_DESC downscaledPictureTexCopyDesc;
	downscaledPictureTexCopyDesc.Width              = mDownscaledWidth;
	downscaledPictureTexCopyDesc.Height             = mDownscaledHeight;
	downscaledPictureTexCopyDesc.Format             = DXGI_FORMAT_R32_FLOAT;
	downscaledPictureTexCopyDesc.Usage              = D3D11_USAGE_STAGING;
	downscaledPictureTexCopyDesc.BindFlags          = 0;
	downscaledPictureTexCopyDesc.CPUAccessFlags     = D3D11_CPU_ACCESS_READ;
	downscaledPictureTexCopyDesc.ArraySize          = 1;
	downscaledPictureTexCopyDesc.MipLevels          = 1;
	downscaledPictureTexCopyDesc.SampleDesc.Count   = 1;
	downscaledPictureTexCopyDesc.SampleDesc.Quality = 0;
	downscaledPictureTexCopyDesc.MiscFlags          = 0;

	ThrowIfFailed(device->CreateTexture2D(&downscaledPictureTexCopyDesc, nullptr, mDownscaledStabilityTexCopy.GetAddressOf()));
}

void Downscaler::LoadShaderData(ID3D11Device* device)
{
	Microsoft::WRL::ComPtr<ID3DBlob> shaderBlob;

	ThrowIfFailed(D3DReadFileToBlob((GetShaderPath() + L"FinalStateTransformCS.cso").c_str(), shaderBlob.GetAddressOf()));
	ThrowIfFailed(device->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, mFinalStateTransformShader.GetAddressOf()));

	ThrowIfFailed(D3DReadFileToBlob((GetShaderPath() + L"DownscaleBigPictureVS.cso").c_str(), shaderBlob.GetAddressOf()));
	ThrowIfFailed(device->CreateVertexShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, mDownscaleVertexShader.GetAddressOf()));

	ThrowIfFailed(D3DReadFileToBlob((GetShaderPath() + L"DownscaleBigPicturePS.cso").c_str(), shaderBlob.GetAddressOf()));
	ThrowIfFailed(device->CreatePixelShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, mDownscalePixelShader.GetAddressOf()));

	D3D11_SAMPLER_DESC samDesc;
	ZeroMemory(&samDesc, sizeof(D3D11_SAMPLER_DESC));
	samDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samDesc.MaxAnisotropy = 4;

	ThrowIfFailed(device->CreateSamplerState(&samDesc, mBestSamplerEver.GetAddressOf()));

	mDownscaleViewport.TopLeftX = 0.0f;
	mDownscaleViewport.TopLeftY = 0.0f;
	mDownscaleViewport.Width    = (float)mDownscaledWidth;
	mDownscaleViewport.Height   = (float)mDownscaledHeight;
	mDownscaleViewport.MinDepth = 0.0f;
	mDownscaleViewport.MaxDepth = 1.0f;
}

void Downscaler::FinalStateTransform(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* srv, uint32_t oldWidth, uint32_t oldHeight)
{
	ID3D11ShaderResourceView*  finalStateTransformSRVs[] = { srv };
	ID3D11UnorderedAccessView* finalStateTransformUAVs[] = { mFinalStateUAV.Get() };

	dc->CSSetShaderResources(0, 1, finalStateTransformSRVs);
	dc->CSSetUnorderedAccessViews(0, 1, finalStateTransformUAVs, nullptr);

	dc->CSSetShader(mFinalStateTransformShader.Get(), nullptr, 0);
	dc->Dispatch((uint32_t)(ceilf(oldWidth / 32.0f)), (uint32_t)(ceilf(oldHeight / 32.0f)), 1);

	ID3D11ShaderResourceView*  nullSRVs[] = { nullptr };
	ID3D11UnorderedAccessView* nullUAVs[] = { nullptr };

	dc->CSSetShaderResources(0, 1, nullSRVs);
	dc->CSSetUnorderedAccessViews(0, 1, nullUAVs, nullptr);
	dc->CSSetShader(nullptr, nullptr, 0);
}

void Downscaler::ActualDownscaling(ID3D11DeviceContext* dc)
{
	ID3D11Buffer* vertexBuffers[] = {nullptr};
	UINT          strides[]       = {0};
	UINT          offsets[]       = {0};

	dc->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
	dc->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);

	dc->IASetInputLayout(nullptr);
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	dc->VSSetShader(mDownscaleVertexShader.Get(), nullptr, 0);

	ID3D11ShaderResourceView* downscaleSRVs[] = {mFinalStateSRV.Get()};
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
