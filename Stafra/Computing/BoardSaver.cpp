#include "BoardSaver.hpp"
#include <wrl/client.h>
#include "../Util.hpp"
#include "../FileMgmt/PNGSaver.hpp"
#include <algorithm>

BoardSaver::BoardSaver(): mImageBigWidth(0), mImageBigHeight(0), mImageSmallWidth(0), mImageSmallHeight(0)
{
}

BoardSaver::~BoardSaver()
{
}

void BoardSaver::PrepareStagingTextures(ID3D11Device* device, uint32_t bigWidth, uint32_t bigHeight, uint32_t smallWidth, uint32_t smallHeight)
{
	mBigImage.Reset();
	mSmallImage.Reset();

	mImageBigWidth  = bigWidth;
	mImageBigHeight = bigHeight;

	mImageSmallWidth  = smallWidth;
	mImageSmallHeight = smallHeight;

	CreateStagingTexture(device, mImageBigWidth,   mImageBigHeight,   mBigImage.GetAddressOf());
	CreateStagingTexture(device, mImageSmallWidth, mImageSmallHeight, mSmallImage.GetAddressOf());
}

void BoardSaver::SaveBoardToFile(ID3D11Device* device, ID3D11DeviceContext* dc, ID3D11Texture2D* boardAfterFinalTransform, const std::wstring& filename)
{
	dc->CopyResource(mBigImage.Get(), boardAfterFinalTransform);

	uint32_t rowPitch = 0;
	std::vector<uint8_t> imageData;
	CopyTextureData(dc, mBigImage.Get(), mImageBigHeight, imageData, rowPitch);

	std::string filenameA;
	size_t filenameLengthNeeded = WideCharToMultiByte(CP_UTF8, WC_COMPOSITECHECK, filename.c_str(), filename.size(), nullptr, 0, nullptr, nullptr);
	filenameA.resize(filenameLengthNeeded);
	WideCharToMultiByte(CP_UTF8, WC_COMPOSITECHECK, filename.c_str(), filename.size(), filenameA.data(), filenameA.size(), nullptr, nullptr);

	PngSaver pngSaver;
	pngSaver.SavePngImage(filenameA, mImageBigWidth, mImageBigHeight, rowPitch, imageData);
}

void BoardSaver::SaveVideoFrameToFile(ID3D11Device* device, ID3D11DeviceContext* dc, ID3D11Texture2D* boardAfterDownscaling, const std::wstring& filename)
{
	dc->CopyResource(mSmallImage.Get(), boardAfterDownscaling);

	uint32_t rowPitch = 0;
	std::vector<uint8_t> imageData;
	CopyTextureData(dc, mSmallImage.Get(), mImageSmallHeight, imageData, rowPitch);

	std::string filenameA;
	size_t filenameLengthNeeded = WideCharToMultiByte(CP_UTF8, WC_COMPOSITECHECK, filename.c_str(), filename.size(), nullptr, 0, nullptr, nullptr);
	filenameA.resize(filenameLengthNeeded);
	WideCharToMultiByte(CP_UTF8, WC_COMPOSITECHECK, filename.c_str(), filename.size(), filenameA.data(), filenameA.size(), nullptr, nullptr);

	PngSaver pngSaver;
	pngSaver.SavePngImage(filenameA, mImageSmallWidth, mImageSmallHeight, rowPitch, imageData);
}

void BoardSaver::CreateStagingTexture(ID3D11Device* device, uint32_t width, uint32_t height, ID3D11Texture2D** stagingTex)
{
	D3D11_TEXTURE2D_DESC stagingBoardTexDesc;
	stagingBoardTexDesc.Width              = width;
	stagingBoardTexDesc.Height             = height;
	stagingBoardTexDesc.Format             = DXGI_FORMAT_R32_FLOAT;
	stagingBoardTexDesc.MipLevels          = 1;
	stagingBoardTexDesc.ArraySize          = 1;
	stagingBoardTexDesc.Usage              = D3D11_USAGE_STAGING;
	stagingBoardTexDesc.BindFlags          = 0;
	stagingBoardTexDesc.CPUAccessFlags     = D3D11_CPU_ACCESS_READ;
	stagingBoardTexDesc.MiscFlags          = 0;
	stagingBoardTexDesc.SampleDesc.Count   = 1;
	stagingBoardTexDesc.SampleDesc.Quality = 0;

	ThrowIfFailed(device->CreateTexture2D(&stagingBoardTexDesc, nullptr, stagingTex));
}

void BoardSaver::CopyTextureData(ID3D11DeviceContext* dc, ID3D11Texture2D* stagingTex, uint32_t imageHeight, std::vector<uint8_t>& imageData, uint32_t& rowPitch)
{
	imageData.clear();
	std::vector<float> textureData;

	D3D11_MAPPED_SUBRESOURCE mappedTex;
	ThrowIfFailed(dc->Map(stagingTex, 0, D3D11_MAP_READ, 0, &mappedTex));

	float* data = reinterpret_cast<float*>(mappedTex.pData);
	textureData.resize((size_t)mappedTex.RowPitch * imageHeight / sizeof(float));

	rowPitch = mappedTex.RowPitch / sizeof(float);

	std::copy(data, data + textureData.size(), textureData.begin());
	dc->Unmap(stagingTex, 0);

	imageData.resize(textureData.size());
	std::transform(textureData.begin(), textureData.end(), imageData.begin(), [](float val) {return (uint8_t)(val * 255.0f); });
}
