#include "BoardSaver.hpp"
#include <wrl/client.h>
#include "../Util.hpp"
#include "../FileMgmt/PNGSaver.hpp"
#include <algorithm>

BoardSaver::BoardSaver()
{
}

BoardSaver::~BoardSaver()
{
}

void BoardSaver::SaveBoardToFile(ID3D11Device* device, ID3D11DeviceContext* dc, ID3D11Texture2D* boardAfterFinalTransform, const std::wstring& filename)
{
	Microsoft::WRL::ComPtr<ID3D11Texture2D> stagingBoardTex;
	CreateStagingTexture(device, boardAfterFinalTransform, stagingBoardTex.GetAddressOf());

	dc->CopyResource(stagingBoardTex.Get(), boardAfterFinalTransform);

	uint32_t rowPitch = 0;
	std::vector<uint8_t> imageData;
	CopyTextureData(dc, stagingBoardTex.Get(), imageData, rowPitch);

	std::string filenameA;
	size_t filenameLengthNeeded = WideCharToMultiByte(CP_UTF8, WC_COMPOSITECHECK, filename.c_str(), filename.size(), nullptr, 0, nullptr, nullptr);
	filenameA.resize(filenameLengthNeeded);
	WideCharToMultiByte(CP_UTF8, WC_COMPOSITECHECK, filename.c_str(), filename.size(), filenameA.data(), filenameA.size(), nullptr, nullptr);

	PngSaver pngSaver;
	pngSaver.SavePngImage(filenameA, mImageWidth, mImageHeight, rowPitch, imageData);
}

void BoardSaver::CreateStagingTexture(ID3D11Device* device, ID3D11Texture2D* boardTex, ID3D11Texture2D** stagingTex)
{
	D3D11_TEXTURE2D_DESC boardTexDesc;
	boardTex->GetDesc(&boardTexDesc);

	mImageWidth  = boardTexDesc.Width;
	mImageHeight = boardTexDesc.Height;

	D3D11_TEXTURE2D_DESC stagingBoardTexDesc;
	memcpy_s(&stagingBoardTexDesc, sizeof(D3D11_TEXTURE2D_DESC), &boardTexDesc, sizeof(D3D11_TEXTURE2D_DESC));
	stagingBoardTexDesc.Usage          = D3D11_USAGE_STAGING;
	stagingBoardTexDesc.BindFlags      = 0;
	stagingBoardTexDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

	ThrowIfFailed(device->CreateTexture2D(&stagingBoardTexDesc, nullptr, stagingTex));
}

void BoardSaver::CopyTextureData(ID3D11DeviceContext* dc, ID3D11Texture2D* stagingTex, std::vector<uint8_t>& imageData, uint32_t& rowPitch)
{
	imageData.clear();
	std::vector<float> textureData;

	D3D11_MAPPED_SUBRESOURCE mappedTex;
	ThrowIfFailed(dc->Map(stagingTex, 0, D3D11_MAP_READ, 0, &mappedTex));

	float* data = reinterpret_cast<float*>(mappedTex.pData);
	textureData.resize((size_t)mappedTex.RowPitch * mImageHeight / sizeof(float));

	rowPitch = mappedTex.RowPitch / sizeof(float);

	std::copy(data, data + textureData.size(), textureData.begin());
	dc->Unmap(stagingTex, 0);

	imageData.resize(textureData.size());
	std::transform(textureData.begin(), textureData.end(), imageData.begin(), [](float val) {return (uint8_t)(val * 255.0f); });
}
