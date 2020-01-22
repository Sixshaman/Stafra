#include "Boards.hpp"
#include "DefaultBoards.hpp"
#include "..\Util.hpp"

Boards::Boards(ID3D11Device* device): mBoardWidth(1023), mBoardHeight(1023)
{
	mDefaultBoards = std::make_unique<DefaultBoards>(device);
}

Boards::~Boards()
{
}

uint32_t Boards::GetWidth() const
{
	return mBoardWidth;
}

uint32_t Boards::GetHeight() const
{
	return mBoardHeight;
}

void Boards::Init4CornersBoard(ID3D11Device* device, ID3D11DeviceContext* context, uint32_t width, uint32_t height)
{
	mRestrictionSRV.Reset(); //Restriction may be invalid for the new size
	mInitialBoardTex.Reset();

	mBoardWidth  = width;
	mBoardHeight = height;

	mDefaultBoards->CreateBoard(device, context, width, height, BoardClearMode::FOUR_CORNERS, mInitialBoardTex.GetAddressOf());

	D3D11_TEXTURE2D_DESC boardDesc;
	mInitialBoardTex->GetDesc(&boardDesc);

	D3D11_SHADER_RESOURCE_VIEW_DESC boardSrvDesc;
	boardSrvDesc.Format                    = boardDesc.Format;
	boardSrvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
	boardSrvDesc.Texture2D.MipLevels       = 1;
	boardSrvDesc.Texture2D.MostDetailedMip = 0;

	ThrowIfFailed(device->CreateShaderResourceView(mInitialBoardTex.Get(), &boardSrvDesc, mInitialBoardSRV.GetAddressOf()));
}

void Boards::Init4SidesBoard(ID3D11Device* device, ID3D11DeviceContext* context, uint32_t width, uint32_t height)
{
	mRestrictionSRV.Reset(); //Restriction may be invalid for the new size
	mInitialBoardTex.Reset();

	mBoardWidth  = width;
	mBoardHeight = height;

	mDefaultBoards->CreateBoard(device, context, width, height, BoardClearMode::FOUR_CORNERS, mInitialBoardTex.GetAddressOf());

	D3D11_TEXTURE2D_DESC boardDesc;
	mInitialBoardTex->GetDesc(&boardDesc);

	D3D11_SHADER_RESOURCE_VIEW_DESC boardSrvDesc;
	boardSrvDesc.Format                    = boardDesc.Format;
	boardSrvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
	boardSrvDesc.Texture2D.MipLevels       = 1;
	boardSrvDesc.Texture2D.MostDetailedMip = 0;

	ThrowIfFailed(device->CreateShaderResourceView(mInitialBoardTex.Get(), &boardSrvDesc, mInitialBoardSRV.GetAddressOf()));
}

void Boards::InitBoardFromTexture(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11Texture2D* boardTex)
{
	mRestrictionSRV.Reset(); //Restriction may be invalid for the new size
	mInitialBoardTex.Reset();

	mInitialBoardTex = Microsoft::WRL::ComPtr<ID3D11Texture2D>(boardTex);

	D3D11_TEXTURE2D_DESC boardDesc;
	boardTex->GetDesc(&boardDesc);

	mBoardWidth  = boardDesc.Width;
	mBoardHeight = boardDesc.Height;

	D3D11_SHADER_RESOURCE_VIEW_DESC boardSrvDesc;
	boardSrvDesc.Format                    = boardDesc.Format;
	boardSrvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
	boardSrvDesc.Texture2D.MipLevels       = 1;
	boardSrvDesc.Texture2D.MostDetailedMip = 0;

	ThrowIfFailed(device->CreateShaderResourceView(boardTex, &boardSrvDesc, mInitialBoardSRV.GetAddressOf()));
}

void Boards::InitDefaultRestriction(ID3D11Device* device, ID3D11DeviceContext* context)
{
	mRestrictionSRV.Reset();
}

void Boards::InitRestrictionFromTexture(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11Texture2D* restrictionTex)
{
	mRestrictionSRV.Reset();

	D3D11_TEXTURE2D_DESC restrictionDesc;
	restrictionTex->GetDesc(&restrictionDesc);
	if(restrictionDesc.Width == mBoardWidth && restrictionDesc.Height == mBoardHeight)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC restrictionSrvDesc;
		restrictionSrvDesc.Format                    = restrictionDesc.Format;
		restrictionSrvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
		restrictionSrvDesc.Texture2D.MipLevels       = 1;
		restrictionSrvDesc.Texture2D.MostDetailedMip = 0;

		ThrowIfFailed(device->CreateShaderResourceView(restrictionTex, &restrictionSrvDesc, mRestrictionSRV.GetAddressOf()));
	}
}

ID3D11Texture2D* Boards::GetInitialBoardTex() const
{
	return mInitialBoardTex.Get();
}

ID3D11ShaderResourceView* Boards::GetInitialBoardSRV() const
{
	return mInitialBoardSRV.Get();
}

ID3D11ShaderResourceView* Boards::GetRestrictionSRV() const
{
	return mRestrictionSRV.Get();
}