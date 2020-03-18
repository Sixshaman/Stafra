#include "DefaultBoards.hpp"
#include "..\Util.hpp"

DefaultBoards::DefaultBoards(ID3D11Device* device)
{
	LoadShaderData(device);
}

DefaultBoards::~DefaultBoards()
{
}

void DefaultBoards::CreateBoard(ID3D11Device* device, ID3D11DeviceContext* dc, uint32_t texWidth, uint32_t texHeight, BoardClearMode clearMode, ID3D11Texture2D** outBoardTex)
{
	if(!device || !dc || !outBoardTex || *outBoardTex != nullptr)
	{
		return;
	}

	D3D11_TEXTURE2D_DESC boardTexDesc;
	boardTexDesc.Width              = texWidth;
	boardTexDesc.Height             = texHeight;
	boardTexDesc.Format             = DXGI_FORMAT_R8_UINT;
	boardTexDesc.Usage              = D3D11_USAGE_DEFAULT;
	boardTexDesc.BindFlags          = D3D11_BIND_UNORDERED_ACCESS;
	boardTexDesc.CPUAccessFlags     = 0;
	boardTexDesc.ArraySize          = 1;
	boardTexDesc.MipLevels          = 1;
	boardTexDesc.SampleDesc.Count   = 1;
	boardTexDesc.SampleDesc.Quality = 0;
	boardTexDesc.MiscFlags          = 0;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> boardTex = nullptr;
	ThrowIfFailed(device->CreateTexture2D(&boardTexDesc, nullptr, boardTex.GetAddressOf()));

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	uavDesc.Format             = DXGI_FORMAT_R8_UINT;
	uavDesc.ViewDimension      = D3D11_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> boardUAV = nullptr;
	ThrowIfFailed(device->CreateUnorderedAccessView(boardTex.Get(), &uavDesc, boardUAV.GetAddressOf()));

	switch (clearMode)
	{
	case BoardClearMode::FOUR_CORNERS:
		InitialState4Corners(dc, boardUAV.Get(), texWidth, texHeight);
		break;
	case BoardClearMode::FOUR_SIDES:
		InitialState4Sides(dc, boardUAV.Get(), texWidth, texHeight);
		break;
	case BoardClearMode::CENTER:
		InitialStateCenter(dc, boardUAV.Get(), texWidth, texHeight);
		break;
	default:
		InitialState4Corners(dc, boardUAV.Get(), texWidth, texHeight);
		break;
	}

	D3D11_TEXTURE2D_DESC outBoardTexDesc;
	memcpy_s(&outBoardTexDesc, sizeof(D3D11_TEXTURE2D_DESC), &boardTexDesc, sizeof(D3D11_TEXTURE2D_DESC));
	outBoardTexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE; //No UAVs for the initial board

	ThrowIfFailed(device->CreateTexture2D(&outBoardTexDesc, nullptr, outBoardTex));
	dc->CopyResource(*outBoardTex, boardTex.Get());
}

void DefaultBoards::InitialState4Corners(ID3D11DeviceContext* dc, ID3D11UnorderedAccessView* initialBoardUAV, uint32_t width, uint32_t height)
{
	mCBufferParamsCopy.BoardSize.x = width;
	mCBufferParamsCopy.BoardSize.y = height;
	Utils::UpdateBuffer(mCBufferParams.Get(), mCBufferParamsCopy, dc);

	ID3D11Buffer* clear4CornersCbuffers[] = { mCBufferParams.Get() };
	dc->CSSetConstantBuffers(0, 1, clear4CornersCbuffers);

	ID3D11UnorderedAccessView* clear4CornersInitUAVs[] = { initialBoardUAV };
	dc->CSSetUnorderedAccessViews(0, 1, clear4CornersInitUAVs, nullptr);

	dc->CSSetShader(mClear4CornersShader.Get(), nullptr, 0);
	dc->Dispatch((uint32_t)(ceilf(width / 32.0f)), (uint32_t)(ceilf(height / 32.0f)), 1);

	ID3D11Buffer*          nullCbuffers[] = { nullptr };
	ID3D11UnorderedAccessView* nullUAVs[] = { nullptr };

	dc->CSSetConstantBuffers(0, 1, nullCbuffers);
	dc->CSSetUnorderedAccessViews(0, 1, nullUAVs, nullptr);
	dc->CSSetShader(nullptr, nullptr, 0);
}

void DefaultBoards::InitialState4Sides(ID3D11DeviceContext* dc, ID3D11UnorderedAccessView* initialBoardUAV, uint32_t width, uint32_t height)
{
	mCBufferParamsCopy.BoardSize.x = width;
	mCBufferParamsCopy.BoardSize.y = height;
	Utils::UpdateBuffer(mCBufferParams.Get(), mCBufferParamsCopy, dc);

	ID3D11Buffer* clear4SidesCbuffers[] = { mCBufferParams.Get() };
	dc->CSSetConstantBuffers(0, 1, clear4SidesCbuffers);

	ID3D11UnorderedAccessView* clear4SidesInitUAVs[] = { initialBoardUAV };
	dc->CSSetUnorderedAccessViews(0, 1, clear4SidesInitUAVs, nullptr);

	dc->CSSetShader(mClear4SidesShader.Get(), nullptr, 0);
	dc->Dispatch((uint32_t)(ceilf(width / 32.0f)), (uint32_t)(ceilf(height / 32.0f)), 1);

	ID3D11Buffer*          nullCbuffers[] = { nullptr };
	ID3D11UnorderedAccessView* nullUAVs[] = { nullptr };

	dc->CSSetConstantBuffers(0, 1, nullCbuffers);
	dc->CSSetUnorderedAccessViews(0, 1, nullUAVs, nullptr);
	dc->CSSetShader(nullptr, nullptr, 0);
}

void DefaultBoards::InitialStateCenter(ID3D11DeviceContext* dc, ID3D11UnorderedAccessView* initialBoardUAV, uint32_t width, uint32_t height)
{
	mCBufferParamsCopy.BoardSize.x = width;
	mCBufferParamsCopy.BoardSize.y = height;
	Utils::UpdateBuffer(mCBufferParams.Get(), mCBufferParamsCopy, dc);

	ID3D11Buffer* clear4CornersCbuffers[] = { mCBufferParams.Get() };
	dc->CSSetConstantBuffers(0, 1, clear4CornersCbuffers);

	ID3D11UnorderedAccessView* clear4CornersInitUAVs[] = { initialBoardUAV };
	dc->CSSetUnorderedAccessViews(0, 1, clear4CornersInitUAVs, nullptr);

	dc->CSSetShader(mClearCenterShader.Get(), nullptr, 0);
	dc->Dispatch((uint32_t)(ceilf(width / 32.0f)), (uint32_t)(ceilf(height / 32.0f)), 1);

	ID3D11Buffer*          nullCbuffers[] = { nullptr };
	ID3D11UnorderedAccessView* nullUAVs[] = { nullptr };

	dc->CSSetConstantBuffers(0, 1, nullCbuffers);
	dc->CSSetUnorderedAccessViews(0, 1, nullUAVs, nullptr);
	dc->CSSetShader(nullptr, nullptr, 0);
}

void DefaultBoards::LoadShaderData(ID3D11Device* device)
{
	const std::wstring shaderDir = Utils::GetShaderPath() + L"ClearBoard\\";
	ThrowIfFailed(Utils::LoadShaderFromFile(device, shaderDir + L"Clear4CornersCS.cso", mClear4CornersShader.GetAddressOf()));
	ThrowIfFailed(Utils::LoadShaderFromFile(device, shaderDir + L"Clear4SidesCS.cso",   mClear4SidesShader.GetAddressOf()));
	ThrowIfFailed(Utils::LoadShaderFromFile(device, shaderDir + L"ClearCenterCS.cso",   mClearCenterShader.GetAddressOf()));

	D3D11_BUFFER_DESC cbDesc;
	cbDesc.Usage               = D3D11_USAGE_DYNAMIC;
	cbDesc.ByteWidth           = (sizeof(CBParamsStruct) + 0xff) & (~0xff);
	cbDesc.BindFlags           = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
	cbDesc.MiscFlags           = 0;
	cbDesc.StructureByteStride = 0;

	CBParamsStruct initData;
	initData.BoardSize = DirectX::XMUINT2(0, 0);

	D3D11_SUBRESOURCE_DATA cbData;
	cbData.pSysMem          = &initData;
	cbData.SysMemPitch      = 0;
	cbData.SysMemSlicePitch = 0;

	ThrowIfFailed(device->CreateBuffer(&cbDesc, &cbData, &mCBufferParams));
}