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

	void PrepareStagingTextures(ID3D11Device* device, uint32_t bigWidth, uint32_t bigHeight, uint32_t smallWidth, uint32_t smallHeight, uint32_t clickRuleWidth, uint32_t clickRuleHeight);

	void SaveBoardToFile(ID3D11Device* device,      ID3D11DeviceContext* dc, ID3D11Texture2D* boardAfterFinalTransform, const std::wstring& filename); //Texture format - R32_FLOAT (better suited for down-/upscaling)
	void SaveVideoFrameToFile(ID3D11Device* device, ID3D11DeviceContext* dc, ID3D11Texture2D* boardAfterDownscaling,    const std::wstring& filename); //Texture format - R32_FLOAT (better suited for down-/upscaling)
	void SaveClickRuleToFile(ID3D11Device* device,  ID3D11DeviceContext* dc, ID3D11Texture2D* clickRuleTex,             const std::wstring& filename); //Texture format - R8_UINT   (click rules don't need down-/upscaling)

private:
	void CreateStagingTexture(ID3D11Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format, ID3D11Texture2D** stagingTex);

	void CopyTextureData(ID3D11DeviceContext* dc, ID3D11Texture2D* stagingTex, uint32_t imageHeight, std::vector<uint8_t>& imageData, uint32_t& rowPitch);
	void CopyClickRuleData(ID3D11DeviceContext* dc, ID3D11Texture2D* stagingTex, uint32_t imageWidth, uint32_t imageHeight, std::vector<uint8_t>& imageData, uint32_t& rowPitch);

private:
	uint32_t mImageBigWidth;
	uint32_t mImageBigHeight;

	uint32_t mImageSmallWidth;
	uint32_t mImageSmallHeight;

	uint32_t mImageClickRuleWidth;
	uint32_t mImageClickRuleHeight;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> mBigImage;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> mSmallImage;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> mClickRuleImage;
};