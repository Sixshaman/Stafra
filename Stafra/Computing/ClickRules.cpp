#include "ClickRules.hpp"
#include "..\Util.hpp"
#include "BoardLoader.hpp"
#include <vector>
#include <DirectXMath.h>

ClickRules::ClickRules(ID3D11Device* device): mClickRuleWidth(0), mClickRuleHeight(0), mbIsDefault(true)
{
	LoadShaders(device);
	CreateClickRuleBuffer(device);
}

ClickRules::~ClickRules()
{
}

uint32_t ClickRules::GetWidth() const
{
	return mClickRuleWidth;
}

uint32_t ClickRules::GetHeight() const
{
	return mClickRuleHeight;
}

void ClickRules::InitDefault(ID3D11Device* device)
{
	mPrevClickRuleSRV.Reset();
	mNextClickRuleSRV.Reset();
	mPrevClickRuleUAV.Reset();
	mNextClickRuleUAV.Reset();

	mClickRuleWidth  = 32;
	mClickRuleHeight = 32;

	D3D11_TEXTURE2D_DESC defaultClickRuleDesc;
	defaultClickRuleDesc.Usage              = D3D11_USAGE_DEFAULT;
	defaultClickRuleDesc.Width              = mClickRuleWidth;
	defaultClickRuleDesc.Height             = mClickRuleHeight;
	defaultClickRuleDesc.Format             = DXGI_FORMAT_R8_UINT;
	defaultClickRuleDesc.ArraySize          = 1;
	defaultClickRuleDesc.BindFlags          = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	defaultClickRuleDesc.CPUAccessFlags     = 0;
	defaultClickRuleDesc.MipLevels          = 1;
	defaultClickRuleDesc.SampleDesc.Count   = 1;
	defaultClickRuleDesc.SampleDesc.Quality = 0;
	defaultClickRuleDesc.MiscFlags          = 0;

	//Fill the default click rule(cross)
	const uint32_t width              = mClickRuleWidth;
	const uint32_t height             = mClickRuleHeight;
	const uint32_t clickRuleCellCount = width * height;
	const uint32_t centralCellX       = (width - 1)/ 2;
	const uint32_t centralCellY       = (height - 1) / 2;

	std::vector<uint8_t> initialTextureData(clickRuleCellCount, (uint8_t)0);
	initialTextureData[(centralCellY + 0) * width + (centralCellX + 0)] = 1; //Central cell
	initialTextureData[(centralCellY - 1) * width + (centralCellX + 0)] = 1; //Top-central cell
	initialTextureData[(centralCellY + 1) * width + (centralCellX + 0)] = 1; //Bottom-central cell
	initialTextureData[(centralCellY + 0) * width + (centralCellX - 1)] = 1; //Left-central cell
	initialTextureData[(centralCellY + 0) * width + (centralCellX + 1)] = 1; //Right-central cell

	D3D11_SUBRESOURCE_DATA clickRuleTexData;
	clickRuleTexData.pSysMem          = initialTextureData.data();
	clickRuleTexData.SysMemPitch      = mClickRuleWidth * sizeof(uint8_t);
	clickRuleTexData.SysMemSlicePitch = clickRuleCellCount * sizeof(uint8_t);

	Microsoft::WRL::ComPtr<ID3D11Texture2D> clickRulePrevImageTex = nullptr;
	ThrowIfFailed(device->CreateTexture2D(&defaultClickRuleDesc, &clickRuleTexData, clickRulePrevImageTex.GetAddressOf()));

	Microsoft::WRL::ComPtr<ID3D11Texture2D> clickRuleNextImageTex = nullptr;
	ThrowIfFailed(device->CreateTexture2D(&defaultClickRuleDesc, nullptr, clickRuleNextImageTex.GetAddressOf()));

	D3D11_SHADER_RESOURCE_VIEW_DESC clickRuleImageSRVDesc;
	clickRuleImageSRVDesc.Format                    = DXGI_FORMAT_R8_UINT;
	clickRuleImageSRVDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
	clickRuleImageSRVDesc.Texture2D.MipLevels       = 1;
	clickRuleImageSRVDesc.Texture2D.MostDetailedMip = 0;

	ThrowIfFailed(device->CreateShaderResourceView(clickRulePrevImageTex.Get(), &clickRuleImageSRVDesc, mPrevClickRuleSRV.GetAddressOf()));
	ThrowIfFailed(device->CreateShaderResourceView(clickRuleNextImageTex.Get(), &clickRuleImageSRVDesc, mNextClickRuleSRV.GetAddressOf()));

	D3D11_UNORDERED_ACCESS_VIEW_DESC clickRuleImageUAVDesc;
	clickRuleImageUAVDesc.Format             = DXGI_FORMAT_R8_UINT;
	clickRuleImageUAVDesc.ViewDimension      = D3D11_UAV_DIMENSION_TEXTURE2D;
	clickRuleImageUAVDesc.Texture2D.MipSlice = 0;

	ThrowIfFailed(device->CreateUnorderedAccessView(clickRulePrevImageTex.Get(), &clickRuleImageUAVDesc, mPrevClickRuleUAV.GetAddressOf()));
	ThrowIfFailed(device->CreateUnorderedAccessView(clickRuleNextImageTex.Get(), &clickRuleImageUAVDesc, mNextClickRuleUAV.GetAddressOf()));

	mbIsDefault = true;
}

void ClickRules::CreateFromTexture(ID3D11Device* device, ID3D11Texture2D* clickRuleTex)
{
	if(!clickRuleTex)
	{
		return;
	}

	mPrevClickRuleSRV.Reset();
	mNextClickRuleSRV.Reset();
	mPrevClickRuleUAV.Reset();
	mNextClickRuleUAV.Reset();

	D3D11_TEXTURE2D_DESC clickRuleTexDesc;
	clickRuleTex->GetDesc(&clickRuleTexDesc);

	mClickRuleWidth  = clickRuleTexDesc.Width;
	mClickRuleHeight = clickRuleTexDesc.Height;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> clickRuleTexCopy;
	device->CreateTexture2D(&clickRuleTexDesc, nullptr, clickRuleTexCopy.GetAddressOf());

	D3D11_SHADER_RESOURCE_VIEW_DESC clickRuleSrvDesc;
	clickRuleSrvDesc.Format                    = clickRuleTexDesc.Format;
	clickRuleSrvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
	clickRuleSrvDesc.Texture2D.MipLevels       = 1;
	clickRuleSrvDesc.Texture2D.MostDetailedMip = 0;

	ThrowIfFailed(device->CreateShaderResourceView(clickRuleTex,           &clickRuleSrvDesc, mPrevClickRuleSRV.GetAddressOf()));
	ThrowIfFailed(device->CreateShaderResourceView(clickRuleTexCopy.Get(), &clickRuleSrvDesc, mNextClickRuleSRV.GetAddressOf()));

	D3D11_UNORDERED_ACCESS_VIEW_DESC clickRuleImageUAVDesc;
	clickRuleImageUAVDesc.Format             = clickRuleTexDesc.Format;
	clickRuleImageUAVDesc.ViewDimension      = D3D11_UAV_DIMENSION_TEXTURE2D;
	clickRuleImageUAVDesc.Texture2D.MipSlice = 0;

	ThrowIfFailed(device->CreateUnorderedAccessView(clickRuleTex,           &clickRuleImageUAVDesc, mPrevClickRuleUAV.GetAddressOf()));
	ThrowIfFailed(device->CreateUnorderedAccessView(clickRuleTexCopy.Get(), &clickRuleImageUAVDesc, mNextClickRuleUAV.GetAddressOf()));

	mbIsDefault = false;
}

void ClickRules::EditCellState(ID3D11DeviceContext* dc, uint32_t x, uint32_t y)
{
	mCBufferParamsCopy.ClickX = x;
	mCBufferParamsCopy.ClickY = y;
	Utils::UpdateBuffer(mCBufferParams.Get(), mCBufferParamsCopy, dc);

	ID3D11Buffer*          clickRuleCbuffers[] = { mCBufferParams.Get()    };
	ID3D11ShaderResourceView*  clickRuleSRVs[] = { mPrevClickRuleSRV.Get() };
	ID3D11UnorderedAccessView* clickRuleUAVs[] = { mNextClickRuleUAV.Get() };

	dc->CSSetConstantBuffers(0, 1, clickRuleCbuffers);
	dc->CSSetShaderResources(0, 1, clickRuleSRVs);
	dc->CSSetUnorderedAccessViews(0, 1, clickRuleUAVs, nullptr);

	dc->CSSetShader(mChangeCellStateShader.Get(), nullptr, 0);
	dc->Dispatch((UINT)ceilf(mClickRuleWidth / 16.0f), (UINT)ceilf(mClickRuleHeight / 16.0f), 1);

	ID3D11Buffer*          nullCbuffers[] = { nullptr };
	ID3D11ShaderResourceView*  nullSRVs[] = { nullptr };
	ID3D11UnorderedAccessView* nullUAVs[] = { nullptr };

	dc->CSSetConstantBuffers(0, 1, nullCbuffers);
	dc->CSSetShaderResources(0, 1, nullSRVs);
	dc->CSSetUnorderedAccessViews(0, 1, nullUAVs, nullptr);

	std::swap(mPrevClickRuleSRV, mNextClickRuleSRV);
	std::swap(mPrevClickRuleUAV, mNextClickRuleUAV);

	mbIsDefault = false;
}

void ClickRules::Bake(ID3D11DeviceContext* dc)
{
	ID3D11ShaderResourceView* srvs[] = {mPrevClickRuleSRV.Get()};
	dc->CSSetShaderResources(0, 1, srvs);

	uint32_t          counterValues[] = {0};
	ID3D11UnorderedAccessView* uavs[] = {mClickRuleBufferUAV.Get()};
	dc->CSSetUnorderedAccessViews(0, 1, uavs, counterValues);

	dc->CSSetShader(mBakeClickRuleShader.Get(), nullptr, 0);

	dc->Dispatch((UINT)ceilf(mClickRuleWidth / 16.0f), (UINT)ceilf(mClickRuleHeight / 16.0f), 1);

	ID3D11ShaderResourceView* nullSrvs[] = { nullptr };
	dc->CSSetShaderResources(0, 1, nullSrvs);

	ID3D11UnorderedAccessView* nullUavs[] = { nullptr };
	dc->CSSetUnorderedAccessViews(0, 1, nullUavs, nullptr);

	dc->CopyStructureCount(mClickRuleCellCounterBuffer.Get(), 0, mClickRuleBufferUAV.Get());
}

bool ClickRules::IsDefault() const
{
	return mbIsDefault;
}

ID3D11ShaderResourceView* ClickRules::GetClickRuleImageSRV() const
{
	return mPrevClickRuleSRV.Get();
}

ID3D11ShaderResourceView* ClickRules::GetClickRuleBufferSRV() const
{
	return mClickRuleBufferSRV.Get();
}

ID3D11ShaderResourceView* ClickRules::GetClickRuleBufferCounterSRV() const
{
	return mClickRuleCellCounterBufferSRV.Get();
}

void ClickRules::LoadShaders(ID3D11Device* device)
{
	const std::wstring shaderDir = Utils::GetShaderPath() + L"ClickRules\\";
	Microsoft::WRL::ComPtr<ID3DBlob> shaderBlob;

	ThrowIfFailed(Utils::LoadShaderFromFile(device, shaderDir + L"BakeClickRuleCS.cso",   mBakeClickRuleShader.GetAddressOf()));
	ThrowIfFailed(Utils::LoadShaderFromFile(device, shaderDir + L"ChangeCellStateCS.cso", mChangeCellStateShader.GetAddressOf()));

	D3D11_BUFFER_DESC cbDesc;
	cbDesc.Usage               = D3D11_USAGE_DYNAMIC;
	cbDesc.ByteWidth           = (sizeof(CBParamsStruct) + 0xff) & (~0xff);
	cbDesc.BindFlags           = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
	cbDesc.MiscFlags           = 0;
	cbDesc.StructureByteStride = 0;

	CBParamsStruct initData;
	initData.ClickX = 0;
	initData.ClickY = 0;

	D3D11_SUBRESOURCE_DATA cbData;
	cbData.pSysMem          = &initData;
	cbData.SysMemPitch      = 0;
	cbData.SysMemSlicePitch = 0;

	ThrowIfFailed(device->CreateBuffer(&cbDesc, &cbData, &mCBufferParams));	
}

void ClickRules::CreateClickRuleBuffer(ID3D11Device* device)
{
	mClickRuleBufferSRV.Reset();
	mClickRuleBufferUAV.Reset();

	mClickRuleCellCounterBuffer.Reset();
	mClickRuleCellCounterBufferSRV.Reset();

	const uint32_t stride      = sizeof(DirectX::XMINT2);
	const size_t   maxElements = 1024; //Max. 1024 elements

	D3D11_BUFFER_DESC clickRuleCoordBufferDesc;
	clickRuleCoordBufferDesc.ByteWidth           = stride * maxElements;
	clickRuleCoordBufferDesc.Usage               = D3D11_USAGE_DEFAULT;
	clickRuleCoordBufferDesc.BindFlags           = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	clickRuleCoordBufferDesc.CPUAccessFlags      = 0;
	clickRuleCoordBufferDesc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	clickRuleCoordBufferDesc.StructureByteStride = stride;

	std::vector<DirectX::XMINT2> initData(maxElements, DirectX::XMINT2(0, 0)); //Init as default, just in case
	initData[0] = DirectX::XMINT2( 0,  0);
	initData[1] = DirectX::XMINT2(-1,  0);
	initData[2] = DirectX::XMINT2( 1,  0);
	initData[3] = DirectX::XMINT2( 0, -1);
	initData[4] = DirectX::XMINT2( 0,  1);

	D3D11_SUBRESOURCE_DATA initialData;
	initialData.pSysMem          = initData.data();
	initialData.SysMemPitch      = 0;
	initialData.SysMemSlicePitch = 0;

	Microsoft::WRL::ComPtr<ID3D11Buffer> clickRuleBuffer;
	ThrowIfFailed(device->CreateBuffer(&clickRuleCoordBufferDesc, &initialData, clickRuleBuffer.GetAddressOf()));

	D3D11_SHADER_RESOURCE_VIEW_DESC clickRuleSRVDesc;
	clickRuleSRVDesc.Format                = DXGI_FORMAT_UNKNOWN;
	clickRuleSRVDesc.ViewDimension         = D3D11_SRV_DIMENSION_BUFFEREX;
	clickRuleSRVDesc.BufferEx.FirstElement = 0;
	clickRuleSRVDesc.BufferEx.NumElements  = maxElements;
	clickRuleSRVDesc.BufferEx.Flags        = 0;

	ThrowIfFailed(device->CreateShaderResourceView(clickRuleBuffer.Get(), &clickRuleSRVDesc, mClickRuleBufferSRV.GetAddressOf()));

	D3D11_UNORDERED_ACCESS_VIEW_DESC clickRuleUAVDesc;
	clickRuleUAVDesc.Format              = DXGI_FORMAT_UNKNOWN;
	clickRuleUAVDesc.ViewDimension       = D3D11_UAV_DIMENSION_BUFFER;
	clickRuleUAVDesc.Buffer.FirstElement = 0;
	clickRuleUAVDesc.Buffer.NumElements  = maxElements;
	clickRuleUAVDesc.Buffer.Flags        = D3D11_BUFFER_UAV_FLAG_APPEND;

	ThrowIfFailed(device->CreateUnorderedAccessView(clickRuleBuffer.Get(), &clickRuleUAVDesc, mClickRuleBufferUAV.GetAddressOf()));

	D3D11_BUFFER_DESC clickRuleBufferCounterDesc;
	clickRuleBufferCounterDesc.ByteWidth           = sizeof(uint32_t);
	clickRuleBufferCounterDesc.Usage               = D3D11_USAGE_DEFAULT;
	clickRuleBufferCounterDesc.BindFlags           = D3D11_BIND_SHADER_RESOURCE;
	clickRuleBufferCounterDesc.CPUAccessFlags      = 0;
	clickRuleBufferCounterDesc.MiscFlags           = 0;
	clickRuleBufferCounterDesc.StructureByteStride = 0;

	uint32_t initCounterData = 5; //Default 5 cross cells

	D3D11_SUBRESOURCE_DATA counterBufferData;
	counterBufferData.pSysMem          = &initCounterData;
	counterBufferData.SysMemPitch      = 0;
	counterBufferData.SysMemSlicePitch = 0;

	ThrowIfFailed(device->CreateBuffer(&clickRuleBufferCounterDesc, &counterBufferData, mClickRuleCellCounterBuffer.GetAddressOf()));

	D3D11_SHADER_RESOURCE_VIEW_DESC counterBufferSRVDesc;
	counterBufferSRVDesc.Format                = DXGI_FORMAT_R32_UINT;
	counterBufferSRVDesc.ViewDimension         = D3D11_SRV_DIMENSION_BUFFEREX;
	counterBufferSRVDesc.BufferEx.FirstElement = 0;
	counterBufferSRVDesc.BufferEx.NumElements  = 1;
	counterBufferSRVDesc.BufferEx.Flags        = 0;

	ThrowIfFailed(device->CreateShaderResourceView(mClickRuleCellCounterBuffer.Get(), &counterBufferSRVDesc, mClickRuleCellCounterBufferSRV.GetAddressOf()));
}
