#include "EqualityChecker.hpp"
#include "Util.hpp"
#include <d3dcompiler.h>

EqualityChecker::EqualityChecker(ID3D11Device* device, uint32_t maxWidth, uint32_t maxHeight): mMaxWidth(maxWidth), mMaxHeight(maxHeight)
{
	CreateTextures(device);
	LoadShaderData(device);
}

EqualityChecker::~EqualityChecker()
{
}

bool EqualityChecker::CheckEquality(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* left, ID3D11ShaderResourceView* right)
{
	CreateEqualityTexture(dc, left, right);

	uint32_t curWidth  = mMaxWidth;
	uint32_t curHeight = mMaxHeight;
	while(curWidth > 32 || curHeight > 32)
	{
		uint32_t newWidth  = 0;
		uint32_t newHeight = 0;
		ShrinkEqualityTexture(dc, curWidth, curHeight, newWidth, newHeight);

		curWidth  = newWidth;
		curHeight = newHeight;
	}

	FinalizeEqualityTexture(dc, curWidth, curHeight);

	uint32_t compareRes = GetEqualityBufferData(dc);
	return (compareRes != 0);
}

void EqualityChecker::CreateTextures(ID3D11Device* device)
{
	D3D11_TEXTURE2D_DESC boardTexDesc;
	boardTexDesc.Width              = mMaxWidth;
	boardTexDesc.Height             = mMaxHeight;
	boardTexDesc.Format             = DXGI_FORMAT_R8_UINT;
	boardTexDesc.Usage              = D3D11_USAGE_DEFAULT;
	boardTexDesc.BindFlags          = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	boardTexDesc.CPUAccessFlags     = 0;
	boardTexDesc.ArraySize          = 1;
	boardTexDesc.MipLevels          = 1;
	boardTexDesc.SampleDesc.Count   = 1;
	boardTexDesc.SampleDesc.Quality = 0;
	boardTexDesc.MiscFlags          = 0;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> prevEqualityTex = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> nextEqualityTex = nullptr;

	ThrowIfFailed(device->CreateTexture2D(&boardTexDesc, nullptr, prevEqualityTex.GetAddressOf()));
	ThrowIfFailed(device->CreateTexture2D(&boardTexDesc, nullptr, nextEqualityTex.GetAddressOf()));

	D3D11_SHADER_RESOURCE_VIEW_DESC boardSrvDesc;
	boardSrvDesc.Format                    = DXGI_FORMAT_R8_UINT;
	boardSrvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
	boardSrvDesc.Texture2D.MipLevels       = 1;
	boardSrvDesc.Texture2D.MostDetailedMip = 0;

	ThrowIfFailed(device->CreateShaderResourceView(prevEqualityTex.Get(), &boardSrvDesc, mPrevEqualitySRV.GetAddressOf()));
	ThrowIfFailed(device->CreateShaderResourceView(nextEqualityTex.Get(), &boardSrvDesc, mNextEqualitySRV.GetAddressOf()));

	D3D11_UNORDERED_ACCESS_VIEW_DESC boardUavDesc;
	boardUavDesc.Format             = DXGI_FORMAT_R8_UINT;
	boardUavDesc.ViewDimension      = D3D11_UAV_DIMENSION_TEXTURE2D;
	boardUavDesc.Texture2D.MipSlice = 0;

	ThrowIfFailed(device->CreateUnorderedAccessView(prevEqualityTex.Get(), &boardUavDesc, mPrevEqualityUAV.GetAddressOf()));
	ThrowIfFailed(device->CreateUnorderedAccessView(nextEqualityTex.Get(), &boardUavDesc, mNextEqualityUAV.GetAddressOf()));

	D3D11_BUFFER_DESC equalityBufferDesc;
	equalityBufferDesc.ByteWidth           = sizeof(uint32_t);
	equalityBufferDesc.Usage               = D3D11_USAGE_DEFAULT;
	equalityBufferDesc.BindFlags           = D3D11_BIND_UNORDERED_ACCESS;
	equalityBufferDesc.CPUAccessFlags      = 0;
	equalityBufferDesc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	equalityBufferDesc.StructureByteStride = sizeof(uint32_t);

	ThrowIfFailed(device->CreateBuffer(&equalityBufferDesc, nullptr, &mEqualityBuffer));

	D3D11_UNORDERED_ACCESS_VIEW_DESC equalityUavDesc;
	equalityUavDesc.Format              = DXGI_FORMAT_UNKNOWN;
	equalityUavDesc.ViewDimension       = D3D11_UAV_DIMENSION_BUFFER;
	equalityUavDesc.Buffer.FirstElement = 0;
	equalityUavDesc.Buffer.NumElements  = 1;
	equalityUavDesc.Buffer.Flags        = 0;

	ThrowIfFailed(device->CreateUnorderedAccessView(mEqualityBuffer.Get(), &equalityUavDesc, mEqualityBufferUAV.GetAddressOf()));

	D3D11_BUFFER_DESC equalityBufferCopyDesc;
	equalityBufferCopyDesc.ByteWidth           = sizeof(uint32_t);
	equalityBufferCopyDesc.Usage               = D3D11_USAGE_STAGING;
	equalityBufferCopyDesc.BindFlags           = 0;
	equalityBufferCopyDesc.CPUAccessFlags      = D3D11_CPU_ACCESS_READ;
	equalityBufferCopyDesc.MiscFlags           = 0;
	equalityBufferCopyDesc.StructureByteStride = 0;

	ThrowIfFailed(device->CreateBuffer(&equalityBufferCopyDesc, nullptr, &mEqualityBufferCopy));

	D3D11_BUFFER_DESC cbDesc;
	cbDesc.Usage               = D3D11_USAGE_DYNAMIC;
	cbDesc.ByteWidth           = (sizeof(CBParamsStruct) + 0xff) & (~0xff);
	cbDesc.BindFlags           = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
	cbDesc.MiscFlags           = 0;
	cbDesc.StructureByteStride = 0;

	CBParamsStruct initData;
	initData.BoardSize = DirectX::XMUINT2(mMaxWidth, mMaxHeight);

	D3D11_SUBRESOURCE_DATA cbData;
	cbData.pSysMem          = &initData;
	cbData.SysMemPitch      = 0;
	cbData.SysMemSlicePitch = 0;

	ThrowIfFailed(device->CreateBuffer(&cbDesc, &cbData, &mCBufferParams));
}

void EqualityChecker::LoadShaderData(ID3D11Device* device)
{
	Microsoft::WRL::ComPtr<ID3DBlob> shaderBlob;

	ThrowIfFailed(D3DReadFileToBlob((GetShaderPath() + L"CompareBoardsStartCS.cso").c_str(), shaderBlob.GetAddressOf()));
	ThrowIfFailed(device->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, mCompareBoardsStartShader.GetAddressOf()));

	ThrowIfFailed(D3DReadFileToBlob((GetShaderPath() + L"CompareBoardsShrinkCS.cso").c_str(), shaderBlob.GetAddressOf()));
	ThrowIfFailed(device->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, mCompareBoardsShrinkShader.GetAddressOf()));

	ThrowIfFailed(D3DReadFileToBlob((GetShaderPath() + L"CompareBoardsFinalCS.cso").c_str(), shaderBlob.GetAddressOf()));
	ThrowIfFailed(device->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, mCompareBoardsFinalShader.GetAddressOf()));
}

void EqualityChecker::CreateEqualityTexture(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* left, ID3D11ShaderResourceView* right)
{
	ID3D11ShaderResourceView*  compareStartSRVs[] = { left, right };
	ID3D11UnorderedAccessView* compareStartUAVs[] = { mNextEqualityUAV.Get() };

	dc->CSSetShaderResources(0, 2, compareStartSRVs);
	dc->CSSetUnorderedAccessViews(0, 1, compareStartUAVs, nullptr);

	dc->CSSetShader(mCompareBoardsStartShader.Get(), nullptr, 0);
	dc->Dispatch((uint32_t)(ceilf(mMaxWidth / 32.0f)), (uint32_t)(ceilf(mMaxHeight / 32.0f)), 1);

	ID3D11Buffer*          nullCbuffers[] = { nullptr };
	ID3D11ShaderResourceView*  nullSRVs[] = { nullptr, nullptr };
	ID3D11UnorderedAccessView* nullUAVs[] = { nullptr };

	dc->CSSetShaderResources(0, 2, nullSRVs);
	dc->CSSetUnorderedAccessViews(0, 1, nullUAVs, nullptr);

	std::swap(mPrevEqualitySRV, mNextEqualitySRV);
	std::swap(mPrevEqualityUAV, mNextEqualityUAV);
}

void EqualityChecker::ShrinkEqualityTexture(ID3D11DeviceContext* dc, uint32_t curWidth, uint32_t curHeight, uint32_t& outNewWidth, uint32_t& outNewHeight)
{
	ID3D11ShaderResourceView*  compareShrinkSRVs[] = { mPrevEqualitySRV.Get() };
	ID3D11UnorderedAccessView* compareShrinkUAVs[] = { mNextEqualityUAV.Get() };

	dc->CSSetShaderResources(0, 1, compareShrinkSRVs);
	dc->CSSetUnorderedAccessViews(0, 1, compareShrinkUAVs, nullptr);

	mCBufferParamsCopy.BoardSize.x = curWidth;
	mCBufferParamsCopy.BoardSize.y = curHeight;
	UpdateBuffer(mCBufferParams.Get(), mCBufferParamsCopy, dc);

	ID3D11Buffer* compareShrinkCBuffers[] = { mCBufferParams.Get() };
	dc->CSSetConstantBuffers(0, 1, compareShrinkCBuffers);

	dc->CSSetShader(mCompareBoardsShrinkShader.Get(), nullptr, 0);

	uint32_t shrankWidth  = (uint32_t)(ceilf(curWidth / 32.0f));
	uint32_t shrankHeight = (uint32_t)(ceilf(curHeight / 32.0f));
	dc->Dispatch(shrankWidth, shrankHeight, 1);

	ID3D11Buffer*          nullCbuffers[] = { nullptr };
	ID3D11ShaderResourceView*  nullSRVs[] = { nullptr };
	ID3D11UnorderedAccessView* nullUAVs[] = { nullptr };

	dc->CSSetShaderResources(0, 1, nullSRVs);
	dc->CSSetUnorderedAccessViews(0, 1, nullUAVs, nullptr);
	dc->CSSetConstantBuffers(0, 1, nullCbuffers);

	std::swap(mPrevEqualitySRV, mNextEqualitySRV);
	std::swap(mPrevEqualityUAV, mNextEqualityUAV);

	outNewWidth  = shrankWidth;
	outNewHeight = shrankHeight;
}

void EqualityChecker::FinalizeEqualityTexture(ID3D11DeviceContext* dc, uint32_t curWidth, uint32_t curHeight)
{
	ID3D11ShaderResourceView*  compareFinalSRVs[] = { mPrevEqualitySRV.Get() };
	ID3D11UnorderedAccessView* compareFinalUAVs[] = { mEqualityBufferUAV.Get() };

	dc->CSSetShaderResources(0, 1, compareFinalSRVs);
	dc->CSSetUnorderedAccessViews(0, 1, compareFinalUAVs, nullptr);

	mCBufferParamsCopy.BoardSize.x = curWidth;
	mCBufferParamsCopy.BoardSize.y = curHeight;
	UpdateBuffer(mCBufferParams.Get(), mCBufferParamsCopy, dc);

	ID3D11Buffer* compareFinalCBuffer[] = { mCBufferParams.Get() };
	dc->CSSetConstantBuffers(0, 1, compareFinalCBuffer);

	dc->CSSetShader(mCompareBoardsFinalShader.Get(), nullptr, 0);
	dc->Dispatch(1, 1, 1);

	ID3D11Buffer*          nullCbuffers[] = { nullptr };
	ID3D11ShaderResourceView*  nullSRVs[] = { nullptr };
	ID3D11UnorderedAccessView* nullUAVs[] = { nullptr };

	dc->CSSetShaderResources(0, 1, nullSRVs);
	dc->CSSetUnorderedAccessViews(0, 1, nullUAVs, nullptr);
	dc->CSSetConstantBuffers(0, 1, nullCbuffers);

	dc->CSSetShader(nullptr, nullptr, 0);
}

uint32_t EqualityChecker::GetEqualityBufferData(ID3D11DeviceContext* dc)
{
	dc->CopyResource(mEqualityBufferCopy.Get(), mEqualityBuffer.Get());

	D3D11_MAPPED_SUBRESOURCE mappedBuf;
	ThrowIfFailed(dc->Map(mEqualityBufferCopy.Get(), 0, D3D11_MAP_READ, 0, &mappedBuf));

	uint32_t* data  = reinterpret_cast<uint32_t*>(mappedBuf.pData);
	uint32_t result = *data;

	dc->Unmap(mEqualityBufferCopy.Get(), 0);
	return result;
}
