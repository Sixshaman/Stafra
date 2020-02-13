#include "BoardLoader.hpp"
#include "..\Util.hpp"
#include "..\3rd party/WICTextureLoader.h"

BoardLoader::BoardLoader(ID3D11Device* device)
{
	LoadShaderData(device);
}

BoardLoader::~BoardLoader()
{
}

Utils::BoardLoadError BoardLoader::LoadBoardFromFile(ID3D11Device* device, ID3D11DeviceContext* dc, const std::wstring& filename, ID3D11Texture2D** outBoardTex)
{
	if(!device || !dc || !outBoardTex || *outBoardTex != nullptr)
	{
		return Utils::BoardLoadError::ERROR_INVALID_ARGUMENT;
	}

	//Attempt to load the initial state
	Microsoft::WRL::ComPtr<ID3D11Texture2D>          initialTex;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> initialStateSRV;
	HRESULT hr = DirectX::CreateWICTextureFromFile(device, filename.c_str(), reinterpret_cast<ID3D11Resource**>(initialTex.GetAddressOf()), initialStateSRV.GetAddressOf());
	if(FAILED(hr))
	{
		return Utils::BoardLoadError::ERROR_CANT_READ_FILE;
	}

	D3D11_TEXTURE2D_DESC texDesc;
	initialTex->GetDesc(&texDesc);

	if(texDesc.Width != texDesc.Height               //Check if board is a square
	|| ((texDesc.Width  + 1) & texDesc.Width)  != 0  //Check if texDesc.Width is power of 2 minus 1
	|| ((texDesc.Height + 1) & texDesc.Height) != 0) //Check if texDesc.Width is power of 2 minus 1
	{
		return Utils::BoardLoadError::ERROR_WRONG_SIZE;
	}

	uint32_t texWidth  = texDesc.Width;
	uint32_t texHeight = texDesc.Height;

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

	InitialStateTransform(dc, initialStateSRV.Get(), boardUAV.Get(), texWidth, texHeight);

	D3D11_TEXTURE2D_DESC outBoardTexDesc;
	memcpy_s(&outBoardTexDesc, sizeof(D3D11_TEXTURE2D_DESC), &boardTexDesc, sizeof(D3D11_TEXTURE2D_DESC));
	outBoardTexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE; //No UAVs for the loaded board

	ThrowIfFailed(device->CreateTexture2D(&outBoardTexDesc, nullptr, outBoardTex));
	dc->CopyResource(*outBoardTex, boardTex.Get());

	return Utils::BoardLoadError::LOAD_SUCCESS;
}

Utils::BoardLoadError BoardLoader::LoadClickRuleFromFile(ID3D11Device* device, ID3D11DeviceContext* dc, const std::wstring& filename, ID3D11Texture2D** outClickRuleTex)
{
	if(!device || !dc || !outClickRuleTex || *outClickRuleTex != nullptr)
	{
		return Utils::BoardLoadError::ERROR_INVALID_ARGUMENT;
	}

	//Attempt to load the initial state
	Microsoft::WRL::ComPtr<ID3D11Texture2D>          clickRuleTex;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> clickruleSRV;
	HRESULT hr = DirectX::CreateWICTextureFromFile(device, filename.c_str(), reinterpret_cast<ID3D11Resource**>(clickRuleTex.GetAddressOf()), clickruleSRV.GetAddressOf());
	if(FAILED(hr))
	{
		return Utils::BoardLoadError::ERROR_CANT_READ_FILE;
	}

	D3D11_TEXTURE2D_DESC texDesc;
	clickRuleTex->GetDesc(&texDesc);

	if(texDesc.Width != 32 || texDesc.Height != 32) //Acceptable click rule sizes: 32x32
	{
		return Utils::BoardLoadError::ERROR_WRONG_SIZE;
	}

	D3D11_TEXTURE2D_DESC clickRuleTexDesc;
	clickRuleTexDesc.Width              = 32;
	clickRuleTexDesc.Height             = 32;
	clickRuleTexDesc.Format             = DXGI_FORMAT_R8_UINT;
	clickRuleTexDesc.Usage              = D3D11_USAGE_DEFAULT;
	clickRuleTexDesc.BindFlags          = D3D11_BIND_UNORDERED_ACCESS;
	clickRuleTexDesc.CPUAccessFlags     = 0;
	clickRuleTexDesc.ArraySize          = 1;
	clickRuleTexDesc.MipLevels          = 1;
	clickRuleTexDesc.SampleDesc.Count   = 1;
	clickRuleTexDesc.SampleDesc.Quality = 0;
	clickRuleTexDesc.MiscFlags          = 0;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> clickRuleTransformedTex;
	ThrowIfFailed(device->CreateTexture2D(&clickRuleTexDesc, nullptr, clickRuleTransformedTex.GetAddressOf()));

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	uavDesc.Format             = DXGI_FORMAT_R8_UINT;
	uavDesc.ViewDimension      = D3D11_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> clickRuleUAV = nullptr;
	ThrowIfFailed(device->CreateUnorderedAccessView(clickRuleTransformedTex.Get(), &uavDesc, clickRuleUAV.GetAddressOf()));

	InitialStateTransform(dc, clickruleSRV.Get(), clickRuleUAV.Get(), 32, 32);

	//Out click rule should bindable as both SRV and UAV, since we need to edit it
	D3D11_TEXTURE2D_DESC outClickRuleTexDesc;
	memcpy_s(&outClickRuleTexDesc, sizeof(D3D11_TEXTURE2D_DESC), &clickRuleTexDesc, sizeof(D3D11_TEXTURE2D_DESC));
	outClickRuleTexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;

	ThrowIfFailed(device->CreateTexture2D(&outClickRuleTexDesc, nullptr, outClickRuleTex));
	dc->CopyResource(*outClickRuleTex, clickRuleTransformedTex.Get());

	return Utils::BoardLoadError::LOAD_SUCCESS;
}

void BoardLoader::InitialStateTransform(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* initialStateSRV, ID3D11UnorderedAccessView* initialBoardUAV, uint32_t width, uint32_t height)
{
	ID3D11ShaderResourceView* initialStateTransformSRVs[] = { initialStateSRV };
	dc->CSSetShaderResources(0, 1, initialStateTransformSRVs);

	ID3D11UnorderedAccessView* initialStateTransformUAVs[] = { initialBoardUAV };
	dc->CSSetUnorderedAccessViews(0, 1, initialStateTransformUAVs, nullptr);

	dc->CSSetShader(mInitialStateTransformShader.Get(), nullptr, 0);
	dc->Dispatch((uint32_t)(ceilf(width / 32.0f)), (uint32_t)(ceilf(height / 32.0f)), 1);

	ID3D11ShaderResourceView*  nullSRVs[] = { nullptr };
	ID3D11UnorderedAccessView* nullUAVs[] = { nullptr };

	dc->CSSetShaderResources(0, 1, nullSRVs);
	dc->CSSetUnorderedAccessViews(0, 1, nullUAVs, nullptr);
	dc->CSSetShader(nullptr, nullptr, 0);
}

void BoardLoader::LoadShaderData(ID3D11Device* device)
{
	const std::wstring shaderDir = Utils::GetShaderPath() + L"StateTransform\\";
	ThrowIfFailed(Utils::LoadShaderFromFile(device, shaderDir + L"InitialStateTransformCS.cso", mInitialStateTransformShader.GetAddressOf()));
}