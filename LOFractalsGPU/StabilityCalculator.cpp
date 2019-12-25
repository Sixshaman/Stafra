#include "StabilityCalculator.hpp"
#include "Util.hpp"
#include <d3dcompiler.h>
#include "InitialState.hpp"
#include "EqualityChecker.hpp"

StabilityCalculator::StabilityCalculator(ID3D11Device* device, uint32_t width, uint32_t height, uint32_t spawnPeriod): mBoardWidth(width), mBoardHeight(height), mbUseClickRule(false), mSpawnPeriod(spawnPeriod), mCurrentStep(0)
{
	CreateTextures(device);
	LoadShaderData(device);
}

StabilityCalculator::~StabilityCalculator()
{
}

void StabilityCalculator::InitTextures(ID3D11Device* device, ID3D11DeviceContext* dc, uint32_t sizeLO)
{
	UINT clearVal[] = {1, 1, 1, 1};
	dc->ClearUnorderedAccessViewUint(mCurrStabilityUAV.Get(), clearVal);

	uint32_t newWidth  = 0;
	uint32_t newHeight = 0;

	int resLoad = 0;

	InitialState is(device);
	if(is.LoadFromFile(device, dc, L"InitialState.png", mInitialBoardUAV.Get(), newWidth, newHeight) == LoadError::LOAD_SUCCESS)
	{
		mBoardWidth  = newWidth;
		mBoardHeight = newHeight;

		resLoad |= CUSTOM_INITIAL_BOARD;
	}
	else
	{
		is.CreateDefault(dc, mInitialBoardUAV.Get(), mBoardWidth, mBoardHeight);
	}

	Microsoft::WRL::ComPtr<ID3D11Texture2D> initialBoardTex;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> currBoardTex;

	mInitialBoardSRV->GetResource(reinterpret_cast<ID3D11Resource**>(initialBoardTex.GetAddressOf()));
	mCurrBoardSRV->GetResource(reinterpret_cast<ID3D11Resource**>(currBoardTex.GetAddressOf()));

	dc->CopyResource(currBoardTex.Get(), initialBoardTex.Get());

	std::swap(mCurrStabilitySRV, mPrevStabilitySRV);
	std::swap(mCurrStabilityUAV, mPrevStabilityUAV);
	std::swap(mCurrBoardSRV,     mPrevBoardSRV);
	std::swap(mCurrBoardUAV,     mPrevBoardUAV);

	if(is.LoadClickRuleFromFile(device, dc, L"ClickRule.png", mClickRuleUAV.Get()) == LoadError::LOAD_SUCCESS)
	{
		//Click rule loaded!
		mbUseClickRule = true;
		resLoad |= CUSTOM_CLICK_RULE;
	}
	else
	{
		//Default one is fine too
		mbUseClickRule = false;
	}

	uint32_t restrictionWidth  = 0;
	uint32_t restrictionHeight = 0;
	if(is.LoadFromFile(device, dc, L"RESTRICTION.png", mRestrictionUAV.Get(), restrictionWidth, restrictionHeight) == LoadError::LOAD_SUCCESS)
	{
		if(mBoardWidth == restrictionWidth && mBoardHeight == restrictionHeight) //Only if the board and the restriction sizes are equal
		{
			//RESTRIIIIIIIIIIIICTION loaded!
			mbUseRestriction = true;
			resLoad |= CUSTOM_RESTRICTION;
		}
		else
		{
			//Restriction NOT LOADED :(
			mbUseRestriction = false;
		}
	}
	else
	{
		//Restriction NOT LOADED :(
		mbUseRestriction = false;
	}
}

void StabilityCalculator::StabilityNextStep(ID3D11DeviceContext* dc)
{
	if(mbUseClickRule)
	{
		if(mbUseRestriction)
		{
			if(mSpawnPeriod == 0)
			{
				StabilityNextStepClickRuleRestricted(dc);
			}
			else
			{
				StabilityNextStepClickRuleSpawnRestricted(dc);
			}
		}
		else
		{
			if(mSpawnPeriod == 0)
			{
				StabilityNextStepClickRule(dc);
			}
			else
			{
				StabilityNextStepClickRuleSpawn(dc);
			}
		}
	}
	else
	{
		if(mbUseRestriction)
		{
			if(mSpawnPeriod == 0)
			{
				StabilityNextStepRestricted(dc);
			}
			else
			{
				StabilityNextStepSpawnRestricted(dc);
			}
		}
		else
		{
			if (mSpawnPeriod == 0)
			{
				StabilityNextStepNormal(dc);
			}
			else
			{
				StabilityNextStepSpawn(dc);
			}
		}
	}

	mCurrentStep++;
	if(mCurrentStep >= 1)
	{
		dc->Flush();
		mCurrentStep = 0;
	}
}

bool StabilityCalculator::CheckEquality(ID3D11Device* device, ID3D11DeviceContext* dc)
{
	EqualityChecker checker(device, mBoardWidth, mBoardHeight);
	return checker.CheckEquality(dc, mPrevBoardSRV.Get(), mInitialBoardSRV.Get());
}

ID3D11ShaderResourceView* StabilityCalculator::GetLastStabilityState() const
{
	return mPrevStabilitySRV.Get();
}

void StabilityCalculator::CreateTextures(ID3D11Device* device)
{
	D3D11_TEXTURE2D_DESC boardTexDesc;
	boardTexDesc.Width              = mBoardWidth;
	boardTexDesc.Height             = mBoardHeight;
	boardTexDesc.Format             = DXGI_FORMAT_R8_UINT;
	boardTexDesc.Usage              = D3D11_USAGE_DEFAULT;
	boardTexDesc.BindFlags          = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	boardTexDesc.CPUAccessFlags     = 0;
	boardTexDesc.ArraySize          = 1;
	boardTexDesc.MipLevels          = 1;
	boardTexDesc.SampleDesc.Count   = 1;
	boardTexDesc.SampleDesc.Quality = 0;
	boardTexDesc.MiscFlags          = 0;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> prevStabilityTex = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> currStabilityTex = nullptr;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> prevBoardTex = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> currBoardTex = nullptr;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> initialBoardTex = nullptr; //For comparison with end state

	Microsoft::WRL::ComPtr<ID3D11Texture2D> restrictionTex = nullptr; //For comparison

	ThrowIfFailed(device->CreateTexture2D(&boardTexDesc, nullptr, prevStabilityTex.GetAddressOf()));
	ThrowIfFailed(device->CreateTexture2D(&boardTexDesc, nullptr, currStabilityTex.GetAddressOf()));

	ThrowIfFailed(device->CreateTexture2D(&boardTexDesc, nullptr, prevBoardTex.GetAddressOf()));
	ThrowIfFailed(device->CreateTexture2D(&boardTexDesc, nullptr, currBoardTex.GetAddressOf()));

	ThrowIfFailed(device->CreateTexture2D(&boardTexDesc, nullptr, initialBoardTex.GetAddressOf()));

	ThrowIfFailed(device->CreateTexture2D(&boardTexDesc, nullptr, restrictionTex.GetAddressOf()));

	D3D11_SHADER_RESOURCE_VIEW_DESC boardSrvDesc;
	boardSrvDesc.Format                    = DXGI_FORMAT_R8_UINT;
	boardSrvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
	boardSrvDesc.Texture2D.MipLevels       = 1;
	boardSrvDesc.Texture2D.MostDetailedMip = 0;

	ThrowIfFailed(device->CreateShaderResourceView(prevStabilityTex.Get(), &boardSrvDesc, mPrevStabilitySRV.GetAddressOf()));
	ThrowIfFailed(device->CreateShaderResourceView(currStabilityTex.Get(), &boardSrvDesc, mCurrStabilitySRV.GetAddressOf()));

	ThrowIfFailed(device->CreateShaderResourceView(prevBoardTex.Get(), &boardSrvDesc, mPrevBoardSRV.GetAddressOf()));
	ThrowIfFailed(device->CreateShaderResourceView(currBoardTex.Get(), &boardSrvDesc, mCurrBoardSRV.GetAddressOf()));

	ThrowIfFailed(device->CreateShaderResourceView(initialBoardTex.Get(), &boardSrvDesc, mInitialBoardSRV.GetAddressOf()));

	ThrowIfFailed(device->CreateShaderResourceView(restrictionTex.Get(), &boardSrvDesc, mRestrictionSRV.GetAddressOf()));

	D3D11_UNORDERED_ACCESS_VIEW_DESC boardUavDesc;
	boardUavDesc.Format             = DXGI_FORMAT_R8_UINT;
	boardUavDesc.ViewDimension      = D3D11_UAV_DIMENSION_TEXTURE2D;
	boardUavDesc.Texture2D.MipSlice = 0;

	ThrowIfFailed(device->CreateUnorderedAccessView(prevStabilityTex.Get(), &boardUavDesc, mPrevStabilityUAV.GetAddressOf()));
	ThrowIfFailed(device->CreateUnorderedAccessView(currStabilityTex.Get(), &boardUavDesc, mCurrStabilityUAV.GetAddressOf()));

	ThrowIfFailed(device->CreateUnorderedAccessView(prevBoardTex.Get(), &boardUavDesc, mPrevBoardUAV.GetAddressOf()));
	ThrowIfFailed(device->CreateUnorderedAccessView(currBoardTex.Get(), &boardUavDesc, mCurrBoardUAV.GetAddressOf()));

	ThrowIfFailed(device->CreateUnorderedAccessView(initialBoardTex.Get(), &boardUavDesc, mInitialBoardUAV.GetAddressOf()));

	ThrowIfFailed(device->CreateUnorderedAccessView(restrictionTex.Get(), &boardUavDesc, mRestrictionUAV.GetAddressOf()));

	D3D11_TEXTURE2D_DESC clickRuleTexDesc;
	clickRuleTexDesc.Width              = 32;
	clickRuleTexDesc.Height             = 32;
	clickRuleTexDesc.Format             = DXGI_FORMAT_R8_UINT;
	clickRuleTexDesc.Usage              = D3D11_USAGE_DEFAULT;
	clickRuleTexDesc.BindFlags          = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	clickRuleTexDesc.CPUAccessFlags     = 0;
	clickRuleTexDesc.ArraySize          = 1;
	clickRuleTexDesc.MipLevels          = 1;
	clickRuleTexDesc.SampleDesc.Count   = 1;
	clickRuleTexDesc.SampleDesc.Quality = 0;
	clickRuleTexDesc.MiscFlags          = 0;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> clickRuleTex = nullptr;
	ThrowIfFailed(device->CreateTexture2D(&clickRuleTexDesc, nullptr, clickRuleTex.GetAddressOf()));

	D3D11_SHADER_RESOURCE_VIEW_DESC clickRuleSrvDesc;
	clickRuleSrvDesc.Format                    = DXGI_FORMAT_R8_UINT;
	clickRuleSrvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
	clickRuleSrvDesc.Texture2D.MipLevels       = 1;
	clickRuleSrvDesc.Texture2D.MostDetailedMip = 0;

	ThrowIfFailed(device->CreateShaderResourceView(clickRuleTex.Get(), &clickRuleSrvDesc, mClickRuleSRV.GetAddressOf()));

	D3D11_UNORDERED_ACCESS_VIEW_DESC clickRuleUavDesc;
	clickRuleUavDesc.Format             = DXGI_FORMAT_R8_UINT;
	clickRuleUavDesc.ViewDimension      = D3D11_UAV_DIMENSION_TEXTURE2D;
	clickRuleUavDesc.Texture2D.MipSlice = 0;

	ThrowIfFailed(device->CreateUnorderedAccessView(clickRuleTex.Get(), &clickRuleUavDesc, mClickRuleUAV.GetAddressOf()));

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

void StabilityCalculator::LoadShaderData(ID3D11Device* device)
{
	Microsoft::WRL::ComPtr<ID3DBlob> shaderBlob;

	ThrowIfFailed(D3DReadFileToBlob((GetShaderPath() + L"StabilityNextStepCS.cso").c_str(), shaderBlob.GetAddressOf()));
	ThrowIfFailed(device->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, mStabilityNextStepShader.GetAddressOf()));
	shaderBlob.Reset();

	ThrowIfFailed(D3DReadFileToBlob((GetShaderPath() + L"StabilityNextStepClickRuleCS.cso").c_str(), shaderBlob.GetAddressOf()));
	ThrowIfFailed(device->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, mStabilityNextStepClickRuleShader.GetAddressOf()));
	shaderBlob.Reset();

	ThrowIfFailed(D3DReadFileToBlob((GetShaderPath() + L"StabilityNextStepSpawnCS.cso").c_str(), shaderBlob.GetAddressOf()));
	ThrowIfFailed(device->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, mStabilityNextStepSPAWNShader.GetAddressOf()));
	shaderBlob.Reset();

	ThrowIfFailed(D3DReadFileToBlob((GetShaderPath() + L"StabilityNextStepSpawnCLICKRuLECS.cso").c_str(), shaderBlob.GetAddressOf()));
	ThrowIfFailed(device->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, mStabilityNextStepSPAWNCLICKRUUUULEShader.GetAddressOf()));
	shaderBlob.Reset();

	ThrowIfFailed(D3DReadFileToBlob((GetShaderPath() + L"StabilityNextStepRESTRICTEDCS.cso").c_str(), shaderBlob.GetAddressOf()));
	ThrowIfFailed(device->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, mStabilityNextStepRESTTERIVCTIONShader.GetAddressOf()));
	shaderBlob.Reset();

	ThrowIfFailed(D3DReadFileToBlob((GetShaderPath() + L"StabilityNextStepClickRuleRESTRICTEDCS.cso").c_str(), shaderBlob.GetAddressOf()));
	ThrowIfFailed(device->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, mStabilityNextStepClickRuleERTESTRICVTIONShader.GetAddressOf()));
	shaderBlob.Reset();

	ThrowIfFailed(D3DReadFileToBlob((GetShaderPath() + L"StabilityNextStepSpawnRESTRICTEDCS.cso").c_str(), shaderBlob.GetAddressOf()));
	ThrowIfFailed(device->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, mStabilityNextStepSPAWNAAAAAAAARESTTRTICTIONShader.GetAddressOf()));
	shaderBlob.Reset();

	ThrowIfFailed(D3DReadFileToBlob((GetShaderPath() + L"StabilityNextStepSpawnCLICKRuLERESTRICTEDCS.cso").c_str(), shaderBlob.GetAddressOf()));
	ThrowIfFailed(device->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, mStabilityNextStepSPAWNCLICKRUUUULEATEWWEAFOSDFSTRESTRICTIONShader.GetAddressOf()));
	shaderBlob.Reset();
}

void StabilityCalculator::StabilityNextStepNormal(ID3D11DeviceContext* dc)
{
	ID3D11ShaderResourceView*  stabilityNextStepSRVs[] = { mPrevBoardSRV.Get(), mPrevStabilitySRV.Get() };
	ID3D11UnorderedAccessView* stabilityNextStepUAVs[] = { mCurrBoardUAV.Get(), mCurrStabilityUAV.Get() };

	dc->CSSetShaderResources(0, 2, stabilityNextStepSRVs);
	dc->CSSetUnorderedAccessViews(0, 2, stabilityNextStepUAVs, nullptr);

	dc->CSSetShader(mStabilityNextStepShader.Get(), nullptr, 0);
	dc->Dispatch((uint32_t)(ceilf(mBoardWidth / 32.0f)), (uint32_t)(ceilf(mBoardHeight / 32.0f)), 1);

	ID3D11ShaderResourceView*  nullSRVs[] = { nullptr, nullptr };
	ID3D11UnorderedAccessView* nullUAVs[] = { nullptr, nullptr };

	dc->CSSetShaderResources(0, 2, nullSRVs);
	dc->CSSetUnorderedAccessViews(0, 2, nullUAVs, nullptr);
	dc->CSSetShader(nullptr, nullptr, 0);

	std::swap(mCurrStabilitySRV, mPrevStabilitySRV);
	std::swap(mCurrStabilityUAV, mPrevStabilityUAV);
	std::swap(mCurrBoardSRV,     mPrevBoardSRV);
	std::swap(mCurrBoardUAV,     mPrevBoardUAV);
}

void StabilityCalculator::StabilityNextStepClickRule(ID3D11DeviceContext* dc)
{
	ID3D11ShaderResourceView*  stabilityNextStepClickRuleSRVs[] = { mPrevBoardSRV.Get(), mPrevStabilitySRV.Get(), mClickRuleSRV.Get() };
	ID3D11UnorderedAccessView* stabilityNextStepClickRuleUAVs[] = { mCurrBoardUAV.Get(), mCurrStabilityUAV.Get() };

	dc->CSSetShaderResources(0, 3, stabilityNextStepClickRuleSRVs);
	dc->CSSetUnorderedAccessViews(0, 2, stabilityNextStepClickRuleUAVs, nullptr);

	dc->CSSetShader(mStabilityNextStepClickRuleShader.Get(), nullptr, 0);
	dc->Dispatch((uint32_t)(ceilf(mBoardWidth / 32.0f)), (uint32_t)(ceilf(mBoardHeight / 32.0f)), 1);

	ID3D11ShaderResourceView*  nullSRVs[] = { nullptr, nullptr, nullptr };
	ID3D11UnorderedAccessView* nullUAVs[] = { nullptr, nullptr };

	dc->CSSetShaderResources(0, 3, nullSRVs);
	dc->CSSetUnorderedAccessViews(0, 2, nullUAVs, nullptr);
	dc->CSSetShader(nullptr, nullptr, 0);

	std::swap(mCurrStabilitySRV, mPrevStabilitySRV);
	std::swap(mCurrStabilityUAV, mPrevStabilityUAV);
	std::swap(mCurrBoardSRV,     mPrevBoardSRV);
	std::swap(mCurrBoardUAV,     mPrevBoardUAV);
}

void StabilityCalculator::StabilityNextStepSpawn(ID3D11DeviceContext* dc)
{
	mCBufferParamsCopy.SpawnPeriod = mSpawnPeriod;
	UpdateBuffer(mCBufferParams.Get(), mCBufferParamsCopy, dc);

	ID3D11Buffer*          stabilityNextStepSpawnCBuffers[] = { mCBufferParams.Get() };
	ID3D11ShaderResourceView*  stabilityNextStepSpawnSRVs[] = { mPrevBoardSRV.Get(), mPrevStabilitySRV.Get() };
	ID3D11UnorderedAccessView* stabilityNextStepSpawnUAVs[] = { mCurrBoardUAV.Get(), mCurrStabilityUAV.Get() };

	dc->CSSetConstantBuffers(0, 1, stabilityNextStepSpawnCBuffers);
	dc->CSSetShaderResources(0, 2, stabilityNextStepSpawnSRVs);
	dc->CSSetUnorderedAccessViews(0, 2, stabilityNextStepSpawnUAVs, nullptr);

	dc->CSSetShader(mStabilityNextStepSPAWNShader.Get(), nullptr, 0);
	dc->Dispatch((uint32_t)(ceilf(mBoardWidth / 32.0f)), (uint32_t)(ceilf(mBoardHeight / 32.0f)), 1);

	ID3D11Buffer*          nullCBuffers[] = { nullptr };
	ID3D11ShaderResourceView*  nullSRVs[] = { nullptr, nullptr };
	ID3D11UnorderedAccessView* nullUAVs[] = { nullptr, nullptr };

	dc->CSSetConstantBuffers(0, 1, nullCBuffers);
	dc->CSSetShaderResources(0, 2, nullSRVs);
	dc->CSSetUnorderedAccessViews(0, 2, nullUAVs, nullptr);
	dc->CSSetShader(nullptr, nullptr, 0);

	std::swap(mCurrStabilitySRV, mPrevStabilitySRV);
	std::swap(mCurrStabilityUAV, mPrevStabilityUAV);
	std::swap(mCurrBoardSRV,     mPrevBoardSRV);
	std::swap(mCurrBoardUAV,     mPrevBoardUAV);
}

void StabilityCalculator::StabilityNextStepClickRuleSpawn(ID3D11DeviceContext* dc)
{
	mCBufferParamsCopy.SpawnPeriod = mSpawnPeriod;
	UpdateBuffer(mCBufferParams.Get(), mCBufferParamsCopy, dc);

	ID3D11Buffer*              stabilityNextStepSpawnClickRuleCBuffers[] = { mCBufferParams.Get() };
	ID3D11ShaderResourceView*  stabilityNextStepSpawnClickRuleSRVs[]     = { mPrevBoardSRV.Get(), mPrevStabilitySRV.Get(), mClickRuleSRV.Get() };
	ID3D11UnorderedAccessView* stabilityNextStepSpawnClickRuleUAVs[]     = { mCurrBoardUAV.Get(), mCurrStabilityUAV.Get() };

	dc->CSSetConstantBuffers(0, 1, stabilityNextStepSpawnClickRuleCBuffers);
	dc->CSSetShaderResources(0, 3, stabilityNextStepSpawnClickRuleSRVs);
	dc->CSSetUnorderedAccessViews(0, 2, stabilityNextStepSpawnClickRuleUAVs, nullptr);

	dc->CSSetShader(mStabilityNextStepSPAWNCLICKRUUUULEShader.Get(), nullptr, 0);
	dc->Dispatch((uint32_t)(ceilf(mBoardWidth / 32.0f)), (uint32_t)(ceilf(mBoardHeight / 32.0f)), 1);

	ID3D11Buffer*          nullCBuffers[] = { nullptr};
	ID3D11ShaderResourceView*  nullSRVs[] = { nullptr, nullptr, nullptr };
	ID3D11UnorderedAccessView* nullUAVs[] = { nullptr, nullptr };

	dc->CSSetConstantBuffers(0, 1, nullCBuffers);
	dc->CSSetShaderResources(0, 3, nullSRVs);
	dc->CSSetUnorderedAccessViews(0, 2, nullUAVs, nullptr);
	dc->CSSetShader(nullptr, nullptr, 0);

	std::swap(mCurrStabilitySRV, mPrevStabilitySRV);
	std::swap(mCurrStabilityUAV, mPrevStabilityUAV);
	std::swap(mCurrBoardSRV,     mPrevBoardSRV);
	std::swap(mCurrBoardUAV,     mPrevBoardUAV);
}

void StabilityCalculator::StabilityNextStepRestricted(ID3D11DeviceContext* dc)
{
	ID3D11ShaderResourceView*  stabilityNextStepSRVs[] = { mPrevBoardSRV.Get(), mPrevStabilitySRV.Get(), mRestrictionSRV.Get() };
	ID3D11UnorderedAccessView* stabilityNextStepUAVs[] = { mCurrBoardUAV.Get(), mCurrStabilityUAV.Get() };

	dc->CSSetShaderResources(0, 3, stabilityNextStepSRVs);
	dc->CSSetUnorderedAccessViews(0, 2, stabilityNextStepUAVs, nullptr);

	dc->CSSetShader(mStabilityNextStepRESTTERIVCTIONShader.Get(), nullptr, 0);
	dc->Dispatch((uint32_t)(ceilf(mBoardWidth / 32.0f)), (uint32_t)(ceilf(mBoardHeight / 32.0f)), 1);

	ID3D11ShaderResourceView*  nullSRVs[] = { nullptr, nullptr, nullptr };
	ID3D11UnorderedAccessView* nullUAVs[] = { nullptr, nullptr };

	dc->CSSetShaderResources(0, 3, nullSRVs);
	dc->CSSetUnorderedAccessViews(0, 2, nullUAVs, nullptr);
	dc->CSSetShader(nullptr, nullptr, 0);

	std::swap(mCurrStabilitySRV, mPrevStabilitySRV);
	std::swap(mCurrStabilityUAV, mPrevStabilityUAV);
	std::swap(mCurrBoardSRV,     mPrevBoardSRV);
	std::swap(mCurrBoardUAV,     mPrevBoardUAV);
}

void StabilityCalculator::StabilityNextStepClickRuleRestricted(ID3D11DeviceContext* dc)
{
	ID3D11ShaderResourceView*  stabilityNextStepClickRuleSRVs[] = { mPrevBoardSRV.Get(), mPrevStabilitySRV.Get(), mClickRuleSRV.Get(), mRestrictionSRV.Get() };
	ID3D11UnorderedAccessView* stabilityNextStepClickRuleUAVs[] = { mCurrBoardUAV.Get(), mCurrStabilityUAV.Get() };

	dc->CSSetShaderResources(0, 4, stabilityNextStepClickRuleSRVs);
	dc->CSSetUnorderedAccessViews(0, 2, stabilityNextStepClickRuleUAVs, nullptr);

	dc->CSSetShader(mStabilityNextStepClickRuleERTESTRICVTIONShader.Get(), nullptr, 0);
	dc->Dispatch((uint32_t)(ceilf(mBoardWidth / 32.0f)), (uint32_t)(ceilf(mBoardHeight / 32.0f)), 1);

	ID3D11ShaderResourceView*  nullSRVs[] = { nullptr, nullptr, nullptr, nullptr };
	ID3D11UnorderedAccessView* nullUAVs[] = { nullptr, nullptr };

	dc->CSSetShaderResources(0, 4, nullSRVs);
	dc->CSSetUnorderedAccessViews(0, 2, nullUAVs, nullptr);
	dc->CSSetShader(nullptr, nullptr, 0);

	std::swap(mCurrStabilitySRV, mPrevStabilitySRV);
	std::swap(mCurrStabilityUAV, mPrevStabilityUAV);
	std::swap(mCurrBoardSRV,     mPrevBoardSRV);
	std::swap(mCurrBoardUAV,     mPrevBoardUAV);
}

void StabilityCalculator::StabilityNextStepSpawnRestricted(ID3D11DeviceContext* dc)
{
	mCBufferParamsCopy.SpawnPeriod = mSpawnPeriod;
	UpdateBuffer(mCBufferParams.Get(), mCBufferParamsCopy, dc);

	ID3D11Buffer*          stabilityNextStepSpawnCBuffers[] = { mCBufferParams.Get() };
	ID3D11ShaderResourceView*  stabilityNextStepSpawnSRVs[] = { mPrevBoardSRV.Get(), mPrevStabilitySRV.Get(), mRestrictionSRV.Get() };
	ID3D11UnorderedAccessView* stabilityNextStepSpawnUAVs[] = { mCurrBoardUAV.Get(), mCurrStabilityUAV.Get() };

	dc->CSSetConstantBuffers(0, 1, stabilityNextStepSpawnCBuffers);
	dc->CSSetShaderResources(0, 3, stabilityNextStepSpawnSRVs);
	dc->CSSetUnorderedAccessViews(0, 2, stabilityNextStepSpawnUAVs, nullptr);

	dc->CSSetShader(mStabilityNextStepSPAWNAAAAAAAARESTTRTICTIONShader.Get(), nullptr, 0);
	dc->Dispatch((uint32_t)(ceilf(mBoardWidth / 32.0f)), (uint32_t)(ceilf(mBoardHeight / 32.0f)), 1);

	ID3D11Buffer*          nullCBuffers[] = { nullptr };
	ID3D11ShaderResourceView*  nullSRVs[] = { nullptr, nullptr, nullptr };
	ID3D11UnorderedAccessView* nullUAVs[] = { nullptr, nullptr };

	dc->CSSetConstantBuffers(0, 1, nullCBuffers);
	dc->CSSetShaderResources(0, 3, nullSRVs);
	dc->CSSetUnorderedAccessViews(0, 2, nullUAVs, nullptr);
	dc->CSSetShader(nullptr, nullptr, 0);

	std::swap(mCurrStabilitySRV, mPrevStabilitySRV);
	std::swap(mCurrStabilityUAV, mPrevStabilityUAV);
	std::swap(mCurrBoardSRV,     mPrevBoardSRV);
	std::swap(mCurrBoardUAV,     mPrevBoardUAV);
}

void StabilityCalculator::StabilityNextStepClickRuleSpawnRestricted(ID3D11DeviceContext* dc)
{
	mCBufferParamsCopy.SpawnPeriod = mSpawnPeriod;
	UpdateBuffer(mCBufferParams.Get(), mCBufferParamsCopy, dc);

	ID3D11Buffer*              stabilityNextStepSpawnClickRuleCBuffers[] = { mCBufferParams.Get() };
	ID3D11ShaderResourceView*  stabilityNextStepSpawnClickRuleSRVs[]     = { mPrevBoardSRV.Get(), mPrevStabilitySRV.Get(), mClickRuleSRV.Get(), mRestrictionSRV.Get() };
	ID3D11UnorderedAccessView* stabilityNextStepSpawnClickRuleUAVs[]     = { mCurrBoardUAV.Get(), mCurrStabilityUAV.Get() };

	dc->CSSetConstantBuffers(0, 1, stabilityNextStepSpawnClickRuleCBuffers);
	dc->CSSetShaderResources(0, 4, stabilityNextStepSpawnClickRuleSRVs);
	dc->CSSetUnorderedAccessViews(0, 2, stabilityNextStepSpawnClickRuleUAVs, nullptr);

	dc->CSSetShader(mStabilityNextStepSPAWNCLICKRUUUULEATEWWEAFOSDFSTRESTRICTIONShader.Get(), nullptr, 0);
	dc->Dispatch((uint32_t)(ceilf(mBoardWidth / 32.0f)), (uint32_t)(ceilf(mBoardHeight / 32.0f)), 1);

	ID3D11Buffer*          nullCBuffers[] = { nullptr};
	ID3D11ShaderResourceView*  nullSRVs[] = { nullptr, nullptr, nullptr, nullptr };
	ID3D11UnorderedAccessView* nullUAVs[] = { nullptr, nullptr };

	dc->CSSetConstantBuffers(0, 1, nullCBuffers);
	dc->CSSetShaderResources(0, 4, nullSRVs);
	dc->CSSetUnorderedAccessViews(0, 2, nullUAVs, nullptr);
	dc->CSSetShader(nullptr, nullptr, 0);

	std::swap(mCurrStabilitySRV, mPrevStabilitySRV);
	std::swap(mCurrStabilityUAV, mPrevStabilityUAV);
	std::swap(mCurrBoardSRV,     mPrevBoardSRV);
	std::swap(mCurrBoardUAV,     mPrevBoardUAV);
}
