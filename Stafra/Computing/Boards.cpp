#include "Boards.hpp"
#include "DefaultBoards.hpp"
#include "..\Util.hpp"

Boards::Boards(ID3D11Device* device): mBoardWidth(1023), mBoardHeight(1023), mRestrictionWidth(0), mRestrictionHeight(0)
{
	mDefaultBoards = std::make_unique<DefaultBoards>(device);

	LoadShaderData(device);
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
	if(width != mRestrictionWidth || height != mRestrictionHeight)
	{
		mRestrictionSRV.Reset(); //Restriction is invalid for the new size
		mRestrictionWidth  = 0;
		mRestrictionHeight = 0;
	}

	mInitialBoardTex.Reset();
	mInitialBoardSRV.Reset();

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
	if(width != mRestrictionWidth || height != mRestrictionHeight)
	{
		mRestrictionSRV.Reset(); //Restriction is invalid for the new size
		mRestrictionWidth  = 0;
		mRestrictionHeight = 0;
	}

	mInitialBoardTex.Reset();
	mInitialBoardSRV.Reset();

	mBoardWidth  = width;
	mBoardHeight = height;

	mDefaultBoards->CreateBoard(device, context, width, height, BoardClearMode::FOUR_SIDES, mInitialBoardTex.GetAddressOf());

	D3D11_TEXTURE2D_DESC boardDesc;
	mInitialBoardTex->GetDesc(&boardDesc);

	D3D11_SHADER_RESOURCE_VIEW_DESC boardSrvDesc;
	boardSrvDesc.Format                    = boardDesc.Format;
	boardSrvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
	boardSrvDesc.Texture2D.MipLevels       = 1;
	boardSrvDesc.Texture2D.MostDetailedMip = 0;

	ThrowIfFailed(device->CreateShaderResourceView(mInitialBoardTex.Get(), &boardSrvDesc, mInitialBoardSRV.GetAddressOf()));
}

void Boards::InitCenterBoard(ID3D11Device* device, ID3D11DeviceContext* context, uint32_t width, uint32_t height)
{
	if(width != mRestrictionWidth || height != mRestrictionHeight)
	{
		mRestrictionSRV.Reset(); //Restriction is invalid for the new size
		mRestrictionWidth  = 0;
		mRestrictionHeight = 0;
	}

	mInitialBoardTex.Reset();
	mInitialBoardSRV.Reset();

	mBoardWidth  = width;
	mBoardHeight = height;

	mDefaultBoards->CreateBoard(device, context, width, height, BoardClearMode::CENTER, mInitialBoardTex.GetAddressOf());

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
	D3D11_TEXTURE2D_DESC boardDesc;
	boardTex->GetDesc(&boardDesc);

	if(boardDesc.Width != mRestrictionWidth || boardDesc.Height != mRestrictionHeight)
	{
		mRestrictionSRV.Reset(); //Restriction is invalid for the new size
		mRestrictionWidth  = 0;
		mRestrictionHeight = 0;
	}

	mInitialBoardTex.Reset();
	mInitialBoardSRV.Reset();

	mInitialBoardTex = Microsoft::WRL::ComPtr<ID3D11Texture2D>(boardTex);

	mBoardWidth  = boardDesc.Width;
	mBoardHeight = boardDesc.Height;

	D3D11_SHADER_RESOURCE_VIEW_DESC boardSrvDesc;
	boardSrvDesc.Format                    = boardDesc.Format;
	boardSrvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
	boardSrvDesc.Texture2D.MipLevels       = 1;
	boardSrvDesc.Texture2D.MostDetailedMip = 0;

	ThrowIfFailed(device->CreateShaderResourceView(boardTex, &boardSrvDesc, mInitialBoardSRV.GetAddressOf()));
}

void Boards::ChangeBoardSize(ID3D11Device* device, ID3D11DeviceContext* dc, uint32_t newWidth, uint32_t newHeight)
{
	mRestrictionSRV.Reset(); //Restriction is invalid for the new size

	//------------------------------------------------------------------------------------------------------------------------------
	// Step 1: Create new empty texture with size newWidth x newHeight and UNORDERED_ACCESS bind flag set, also create an UAV for it
	//------------------------------------------------------------------------------------------------------------------------------

	D3D11_TEXTURE2D_DESC currBoardDesc;
	mInitialBoardTex->GetDesc(&currBoardDesc);

	D3D11_TEXTURE2D_DESC newBoardDesc = currBoardDesc;
	newBoardDesc.Width                = newWidth;
	newBoardDesc.Height               = newHeight;
	newBoardDesc.BindFlags            = D3D11_BIND_UNORDERED_ACCESS;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> newInitialBoard;
	device->CreateTexture2D(&newBoardDesc, nullptr, newInitialBoard.GetAddressOf());

	D3D11_UNORDERED_ACCESS_VIEW_DESC newBoardUavDesc;
	newBoardUavDesc.ViewDimension      = D3D11_UAV_DIMENSION_TEXTURE2D;
	newBoardUavDesc.Format             = newBoardDesc.Format;
	newBoardUavDesc.Texture2D.MipSlice = 0;

	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> newInitialBoardUAV;
	device->CreateUnorderedAccessView(newInitialBoard.Get(), &newBoardUavDesc, newInitialBoardUAV.GetAddressOf());

	//----------------------------------------------------------------------------------------------------------------------------------------------
	// Step 2: Run the compute shader that copies the texture contents to the center of the bigger/smaller image and leaves unfilled entries at zero
	//----------------------------------------------------------------------------------------------------------------------------------------------

	ID3D11ShaderResourceView*  srvs[] = {mInitialBoardSRV.Get()};
	ID3D11UnorderedAccessView* uavs[] = {newInitialBoardUAV.Get()};

	dc->CSSetShaderResources(0, 1, srvs);
	dc->CSSetUnorderedAccessViews(0, 1, uavs, nullptr);

	dc->CSSetShader(mSizeTransformShader.Get(), nullptr, 0);
	
	dc->Dispatch((uint32_t)(ceilf(newWidth / 16.0f)), (uint32_t)(ceilf(newHeight / 16.0f)), 1);

	dc->CSSetShader(nullptr, nullptr, 0);

	ID3D11ShaderResourceView*  nullSrvs[] = {nullptr};
	ID3D11UnorderedAccessView* nullUavs[] = {nullptr};

	dc->CSSetShaderResources(0, 1, nullSrvs);
	dc->CSSetUnorderedAccessViews(0, 1, nullUavs, nullptr);

	//---------------------------------------------------------------------------------------------------------------
	// Step 3: Create a new texture without UNORDERED_ACCES bind flag and copy the contents of the bigger image there
	//---------------------------------------------------------------------------------------------------------------

	mInitialBoardTex.Reset();
	mInitialBoardSRV.Reset();

	currBoardDesc.Width  = newWidth;
	currBoardDesc.Height = newHeight;

	device->CreateTexture2D(&currBoardDesc, nullptr, mInitialBoardTex.GetAddressOf());

	D3D11_SHADER_RESOURCE_VIEW_DESC boardSrvDesc;
	boardSrvDesc.Format                    = currBoardDesc.Format;
	boardSrvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
	boardSrvDesc.Texture2D.MipLevels       = 1;
	boardSrvDesc.Texture2D.MostDetailedMip = 0;

	ThrowIfFailed(device->CreateShaderResourceView(mInitialBoardTex.Get(), &boardSrvDesc, mInitialBoardSRV.GetAddressOf()));

	dc->CopyResource(mInitialBoardTex.Get(), newInitialBoard.Get());

	mBoardWidth  = newWidth;
	mBoardHeight = newHeight;
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
		mRestrictionWidth  = restrictionDesc.Width;
		mRestrictionHeight = restrictionDesc.Height;

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

void Boards::LoadShaderData(ID3D11Device* device)
{
	const std::wstring shaderDir = Utils::GetShaderPath() + L"StateTransform\\";
	Utils::LoadShaderFromFile(device, shaderDir + L"InitialStateSizeTransformCS.cso", mSizeTransformShader.GetAddressOf());
}
