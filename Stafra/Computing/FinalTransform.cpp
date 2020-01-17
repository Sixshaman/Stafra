#include "FinalTransform.hpp"
#include "..\Util.hpp"

FinalTransformer::FinalTransformer(ID3D11Device* device): mBoardWidth(0), mBoardHeight(0)
{
	LoadShaderData(device);
}

FinalTransformer::~FinalTransformer()
{
}

void FinalTransformer::PrepareForTransform(ID3D11Device* device, uint32_t width, uint32_t height)
{
	mBoardWidth  = width;
	mBoardHeight = height;
	ReinitTextures(device, mBoardWidth, mBoardHeight);
}

void FinalTransformer::ComputeTransform(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* srv, uint32_t spawnPeriod, bool bUseSmooth)
{
	if(spawnPeriod == 0 || !bUseSmooth)
	{
		FinalStateTransform(dc, srv, mBoardWidth, mBoardHeight);
	}
	else
	{
		FinalStateTransformSmooth(dc, srv, spawnPeriod, mBoardWidth, mBoardHeight);
	}

	dc->GenerateMips(mFinalStateSRV.Get());
}

ID3D11ShaderResourceView* FinalTransformer::GetTransformedSRV() const
{
	return mFinalStateSRV.Get();
}

void FinalTransformer::ReinitTextures(ID3D11Device* device, uint32_t width, uint32_t height)
{
	mFinalStateSRV.Get();
	mFinalStateUAV.Get();

	D3D11_TEXTURE2D_DESC finalPictureTexDesc;
	finalPictureTexDesc.Width              = width;
	finalPictureTexDesc.Height             = height;
	finalPictureTexDesc.Format             = DXGI_FORMAT_R32_FLOAT;
	finalPictureTexDesc.Usage              = D3D11_USAGE_DEFAULT;
	finalPictureTexDesc.BindFlags          = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_RENDER_TARGET;
	finalPictureTexDesc.CPUAccessFlags     = 0;
	finalPictureTexDesc.ArraySize          = 1;
	finalPictureTexDesc.MipLevels          = 1;
	finalPictureTexDesc.SampleDesc.Count   = 1;
	finalPictureTexDesc.SampleDesc.Quality = 0;
	finalPictureTexDesc.MiscFlags          = D3D11_RESOURCE_MISC_GENERATE_MIPS;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> finalTex = nullptr;
	ThrowIfFailed(device->CreateTexture2D(&finalPictureTexDesc, nullptr, finalTex.GetAddressOf()));

	D3D11_SHADER_RESOURCE_VIEW_DESC finalSrvDesc;
	finalSrvDesc.Format                    = DXGI_FORMAT_R32_FLOAT;
	finalSrvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
	finalSrvDesc.Texture2D.MipLevels       = 1;
	finalSrvDesc.Texture2D.MostDetailedMip = 0;

	ThrowIfFailed(device->CreateShaderResourceView(finalTex.Get(), &finalSrvDesc, mFinalStateSRV.GetAddressOf()));

	D3D11_UNORDERED_ACCESS_VIEW_DESC finalUavDesc;
	finalUavDesc.Format             = DXGI_FORMAT_R32_FLOAT;
	finalUavDesc.ViewDimension      = D3D11_UAV_DIMENSION_TEXTURE2D;
	finalUavDesc.Texture2D.MipSlice = 0;

	ThrowIfFailed(device->CreateUnorderedAccessView(finalTex.Get(), &finalUavDesc, mFinalStateUAV.GetAddressOf()));
}

void FinalTransformer::LoadShaderData(ID3D11Device* device)
{
	const std::wstring shaderDir = Utils::GetShaderPath() + L"StateTransform\\";

	Utils::LoadShaderFromFile(device, shaderDir + L"FinalStateTransformCS.cso",       mFinalStateTransformShader.GetAddressOf());
	Utils::LoadShaderFromFile(device, shaderDir + L"FinalStateTransformSmoothCS.cso", mFinalStateTransformSmoothShader.GetAddressOf());

	D3D11_BUFFER_DESC cbDesc;
	cbDesc.Usage               = D3D11_USAGE_DYNAMIC;
	cbDesc.ByteWidth           = (sizeof(CBParamsStruct) + 0xff) & (~0xff);
	cbDesc.BindFlags           = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
	cbDesc.MiscFlags           = 0;
	cbDesc.StructureByteStride = 0;

	CBParamsStruct initData;
	initData.SpawnPeriod = 0;

	D3D11_SUBRESOURCE_DATA cbData;
	cbData.pSysMem          = &initData;
	cbData.SysMemPitch      = 0;
	cbData.SysMemSlicePitch = 0;

	ThrowIfFailed(device->CreateBuffer(&cbDesc, &cbData, &mCBufferParams));
}

void FinalTransformer::FinalStateTransform(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* srv, uint32_t boardWidth, uint32_t boardHeight)
{
	ID3D11ShaderResourceView*  finalStateTransformSRVs[] = { srv };
	ID3D11UnorderedAccessView* finalStateTransformUAVs[] = { mFinalStateUAV.Get() };

	dc->CSSetShaderResources(0, 1, finalStateTransformSRVs);
	dc->CSSetUnorderedAccessViews(0, 1, finalStateTransformUAVs, nullptr);

	dc->CSSetShader(mFinalStateTransformShader.Get(), nullptr, 0);
	dc->Dispatch((uint32_t)(ceilf(boardWidth / 32.0f)), (uint32_t)(ceilf(boardHeight / 32.0f)), 1);

	ID3D11ShaderResourceView*  nullSRVs[] = { nullptr };
	ID3D11UnorderedAccessView* nullUAVs[] = { nullptr };

	dc->CSSetShaderResources(0, 1, nullSRVs);
	dc->CSSetUnorderedAccessViews(0, 1, nullUAVs, nullptr);
	dc->CSSetShader(nullptr, nullptr, 0);
}

void FinalTransformer::FinalStateTransformSmooth(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* srv, uint32_t spawnPeriod, uint32_t boardWidth, uint32_t boardHeight)
{
	mCBufferParamsCopy.SpawnPeriod = spawnPeriod;
	Utils::UpdateBuffer(mCBufferParams.Get(), mCBufferParamsCopy, dc);

	ID3D11Buffer*              finalStateTransformCbuffers[] = {mCBufferParams.Get()};
	ID3D11ShaderResourceView*  finalStateTransformSRVs[]     = { srv };
	ID3D11UnorderedAccessView* finalStateTransformUAVs[]     = { mFinalStateUAV.Get() };

	dc->CSSetConstantBuffers(0, 1, finalStateTransformCbuffers);
	dc->CSSetShaderResources(0, 1, finalStateTransformSRVs);
	dc->CSSetUnorderedAccessViews(0, 1, finalStateTransformUAVs, nullptr);

	dc->CSSetShader(mFinalStateTransformSmoothShader.Get(), nullptr, 0);
	dc->Dispatch((uint32_t)(ceilf(boardWidth / 32.0f)), (uint32_t)(ceilf(boardHeight / 32.0f)), 1);

	ID3D11Buffer*          nullCBuffers[] = { nullptr };
	ID3D11ShaderResourceView*  nullSRVs[] = { nullptr };
	ID3D11UnorderedAccessView* nullUAVs[] = { nullptr };

	dc->CSSetConstantBuffers(0, 1, nullCBuffers);
	dc->CSSetShaderResources(0, 1, nullSRVs);
	dc->CSSetUnorderedAccessViews(0, 1, nullUAVs, nullptr);
	dc->CSSetShader(nullptr, nullptr, 0);
}
