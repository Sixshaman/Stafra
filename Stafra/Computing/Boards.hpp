#pragma once

#include <wrl/client.h>
#include <d3d11.h>
#include <string>
#include <memory>

class DefaultBoards;

class Boards
{
public:
	Boards(ID3D11Device* device);
	~Boards();

	uint32_t GetWidth()  const;
	uint32_t GetHeight() const;

	void Init4CornersBoard(ID3D11Device* device, ID3D11DeviceContext* context, uint32_t width, uint32_t height);
	void Init4SidesBoard(ID3D11Device* device, ID3D11DeviceContext* context, uint32_t width, uint32_t height);
	void InitCenterBoard(ID3D11Device* device, ID3D11DeviceContext* context, uint32_t width, uint32_t height);

	void InitBoardFromTexture(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11Texture2D* boardTex);

	void ChangeBoardSize(ID3D11Device* device, ID3D11DeviceContext* context, uint32_t newWidth, uint32_t newHeight); //Changes the size of the textures while keeping the contents centered

	void InitDefaultRestriction(ID3D11Device* device, ID3D11DeviceContext* context); //Just resets the pointer
	void InitRestrictionFromTexture(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11Texture2D* restrictionTex);

	ID3D11Texture2D* GetInitialBoardTex() const;

	ID3D11ShaderResourceView* GetInitialBoardSRV() const;
	ID3D11ShaderResourceView* GetRestrictionSRV()  const;

private:
	void LoadShaderData(ID3D11Device* device);

private:
	Microsoft::WRL::ComPtr<ID3D11Texture2D>          mInitialBoardTex;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mInitialBoardSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mRestrictionSRV;

	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mSizeTransformShader;

	std::unique_ptr<DefaultBoards> mDefaultBoards;

	uint32_t mBoardWidth;
	uint32_t mBoardHeight;

	uint32_t mRestrictionWidth;
	uint32_t mRestrictionHeight;
};