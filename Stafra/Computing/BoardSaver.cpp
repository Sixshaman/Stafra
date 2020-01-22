#include "BoardSaver.hpp"
#include <wrl/client.h>
#include "../Util.hpp"
#include "../FileMgmt/PNGSaver.hpp"
#include <algorithm>

BoardSaver::BoardSaver(): mImageBigWidth(0), mImageBigHeight(0), mImageSmallWidth(0), mImageSmallHeight(0), mImageClickRuleWidth(0), mImageClickRuleHeight(0)
{
}

BoardSaver::~BoardSaver()
{
}

void BoardSaver::PrepareStagingTextures(ID3D11Device* device, uint32_t bigWidth, uint32_t bigHeight, uint32_t smallWidth, uint32_t smallHeight, uint32_t clickRuleWidth, uint32_t clickRuleHeight)
{
	mBigImage.Reset();
	mSmallImage.Reset();

	mImageBigWidth  = bigWidth;
	mImageBigHeight = bigHeight;

	mImageSmallWidth  = smallWidth;
	mImageSmallHeight = smallHeight;

	mImageClickRuleWidth  = clickRuleWidth;
	mImageClickRuleHeight = clickRuleHeight;

	CreateStagingTexture(device, mImageBigWidth,       mImageBigHeight,       DXGI_FORMAT_R32_FLOAT, mBigImage.GetAddressOf());
	CreateStagingTexture(device, mImageSmallWidth,     mImageSmallHeight,     DXGI_FORMAT_R32_FLOAT, mSmallImage.GetAddressOf());
	CreateStagingTexture(device, mImageClickRuleWidth, mImageClickRuleHeight, DXGI_FORMAT_R8_UINT,   mClickRuleImage.GetAddressOf());
}

void BoardSaver::SaveBoardToFile(ID3D11Device* device, ID3D11DeviceContext* dc, ID3D11Texture2D* boardAfterFinalTransform, const std::wstring& filename)
{
	dc->CopyResource(mBigImage.Get(), boardAfterFinalTransform);

	uint32_t rowPitch = 0;
	std::vector<uint8_t> imageData;
	CopyTextureData(dc, mBigImage.Get(), mImageBigHeight, imageData, rowPitch);

	RGBCOLOR stabilityColor(1.0f, 0.0f, 1.0f);

	PngSaver pngSaver;
	pngSaver.SavePngImage(filename, mImageBigWidth, mImageBigHeight, rowPitch, stabilityColor, imageData);
}

void BoardSaver::SaveVideoFrameToFile(ID3D11Device* device, ID3D11DeviceContext* dc, ID3D11Texture2D* boardAfterDownscaling, const std::wstring& filename)
{
	dc->CopyResource(mSmallImage.Get(), boardAfterDownscaling);

	uint32_t rowPitch = 0;
	std::vector<uint8_t> imageData;
	CopyTextureData(dc, mSmallImage.Get(), mImageSmallHeight, imageData, rowPitch);

	RGBCOLOR stabilityColor(1.0f, 0.0f, 1.0f);

	PngSaver pngSaver;
	pngSaver.SavePngImage(filename, mImageSmallWidth, mImageSmallHeight, rowPitch, stabilityColor, imageData);
}

void BoardSaver::SaveClickRuleToFile(ID3D11Device* device, ID3D11DeviceContext* dc, ID3D11Texture2D* clickRuleTex, const std::wstring& filename)
{
	dc->CopyResource(mClickRuleImage.Get(), clickRuleTex);

	uint32_t rowPitch = 0;
	std::vector<uint8_t> imageData;
	CopyClickRuleData(dc, mClickRuleImage.Get(), mImageClickRuleWidth, mImageClickRuleHeight, imageData, rowPitch);

	RGBCOLOR clickRuleColor(0.0f, 1.0f, 0.0f);

	PngSaver pngSaver;
	pngSaver.SavePngImage(filename, mImageClickRuleWidth, mImageClickRuleHeight, rowPitch, clickRuleColor, imageData);
}

void BoardSaver::CreateStagingTexture(ID3D11Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format, ID3D11Texture2D** stagingTex)
{
	D3D11_TEXTURE2D_DESC stagingBoardTexDesc;
	stagingBoardTexDesc.Width              = width;
	stagingBoardTexDesc.Height             = height;
	stagingBoardTexDesc.Format             = format;
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

void BoardSaver::CopyClickRuleData(ID3D11DeviceContext* dc, ID3D11Texture2D* stagingTex, uint32_t imageWidth, uint32_t imageHeight, std::vector<uint8_t>& imageData, uint32_t& rowPitch)
{
	imageData.clear();
	std::vector<uint8_t> clickRuleData;

	D3D11_MAPPED_SUBRESOURCE mappedTex;
	ThrowIfFailed(dc->Map(stagingTex, 0, D3D11_MAP_READ, 0, &mappedTex));

	uint8_t* data = reinterpret_cast<uint8_t*>(mappedTex.pData);
	clickRuleData.resize((size_t)mappedTex.RowPitch * imageHeight / sizeof(uint8_t));

	rowPitch = mappedTex.RowPitch / sizeof(uint8_t);

	std::copy(data, data + clickRuleData.size(), clickRuleData.begin());
	dc->Unmap(stagingTex, 0);

	//Mark highlight the edges of the click rule
	for(uint32_t y = 0; y < imageHeight; y++)
	{
		uint32_t rightCellindex = (uint32_t)(y * rowPitch + imageWidth - 1);
		clickRuleData[rightCellindex] = 2;
	}

	for(uint32_t x = 0; x < imageWidth; x++)
	{
		uint32_t bottomCellindex = (uint32_t)((imageHeight - 1) * rowPitch + x);
		clickRuleData[bottomCellindex] = 2;
	}

	imageData.resize(clickRuleData.size());
	for(size_t i = 0; i < clickRuleData.size(); i++)
	{
		if(clickRuleData[i] == 1)
		{
			imageData[i] = 255;
		}
		else if(clickRuleData[i] == 2)
		{
			imageData[i] = 30; //Very slightly
		}
		else
		{
			imageData[i] = 0;
		}
	}
}