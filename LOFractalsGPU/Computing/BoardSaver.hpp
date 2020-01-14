#pragma once

#include <d3d11.h>
#include <vector>
#include <string>

/*
The class for saving a texture to a file.
Input:               ID3D11Texture2D containing floating-point texel values and desired filename
Output:              Saved image
Possible expansions: None ATM
*/
class BoardSaver
{
public:
	BoardSaver();
	~BoardSaver();

	void SaveBoardToFile(ID3D11Device* device, ID3D11DeviceContext* dc, ID3D11Texture2D* boardAfterFinalTransform, const std::wstring& filename);

private:
	void CreateStagingTexture(ID3D11Device* device, ID3D11Texture2D* boardTex, ID3D11Texture2D** stagingTex);
	void CopyTextureData(ID3D11DeviceContext* dc, ID3D11Texture2D* stagingTex, std::vector<uint8_t>& imageData, uint32_t& rowPitch);

private:
	uint32_t mImageWidth;
	uint32_t mImageHeight;
};