#pragma once

#include <d3d11.h>
#include <vector>
#include <string>
#include <wrl/client.h>

/*
The class for saving a texture to a file.
Input:               ID3D11Texture2D of one of 2 possible sizes containing floating-point texel values and desired filename
Output:              Saved image
Possible expansions: None ATM
*/
class BoardSaver
{
public:
	BoardSaver();
	~BoardSaver();

	void PrepareStagingTextures(ID3D11Device* device, uint32_t bigWidth, uint32_t bigHeight, uint32_t smallWidth, uint32_t smallHeight);

	void SaveBoardToFile(ID3D11Device* device,      ID3D11DeviceContext* dc, ID3D11Texture2D* boardAfterFinalTransform, const std::wstring& filename);
	void SaveVideoFrameToFile(ID3D11Device* device, ID3D11DeviceContext* dc, ID3D11Texture2D* boardAfterDownscaling,    const std::wstring& filename);

private:
	void CreateStagingTexture(ID3D11Device* device, uint32_t width, uint32_t height, ID3D11Texture2D** stagingTex);
	void CopyTextureData(ID3D11DeviceContext* dc, ID3D11Texture2D* stagingTex, uint32_t imageHeight, std::vector<uint8_t>& imageData, uint32_t& rowPitch);

private:
	uint32_t mImageBigWidth;
	uint32_t mImageBigHeight;

	uint32_t mImageSmallWidth;
	uint32_t mImageSmallHeight;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> mBigImage;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> mSmallImage;
};