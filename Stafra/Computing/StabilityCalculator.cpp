#include "StabilityCalculator.hpp"
#include "../Util.hpp"
#include <d3dcompiler.h>
#include "EqualityChecker.hpp"

StabilityCalculator::StabilityCalculator(ID3D11Device* device): mBoardWidth(0), mBoardHeight(0), mCurrentStep(0)
{
	LoadShaderData(device);
}

StabilityCalculator::~StabilityCalculator()
{
}

void StabilityCalculator::PrepareForCalculations(ID3D11Device* device, ID3D11DeviceContext* dc, ID3D11Texture2D* initialBoard)
{
	ReinitTextures(device, initialBoard);

	UINT clearVal[] = { 1, 1, 1, 1 };
	dc->ClearUnorderedAccessViewUint(mCurrStabilityUAV.Get(), clearVal);

	Microsoft::WRL::ComPtr<ID3D11Texture2D> currBoardTex = nullptr;
	mCurrBoardSRV->GetResource(reinterpret_cast<ID3D11Resource**>(currBoardTex.GetAddressOf()));

	dc->CopyResource(currBoardTex.Get(), initialBoard);

	std::swap(mCurrStabilitySRV, mPrevStabilitySRV);
	std::swap(mCurrStabilityUAV, mPrevStabilityUAV);
	std::swap(mCurrBoardSRV, mPrevBoardSRV);
	std::swap(mCurrBoardUAV, mPrevBoardUAV);

	mCurrentStep = 0;
}

void StabilityCalculator::StabilityNextStep(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* clickRuleBuffer, ID3D11ShaderResourceView* clickRuleCounterBuffer, ID3D11ShaderResourceView* restrictionSRV, uint32_t spawnPeriod)
{
	if(clickRuleBuffer && clickRuleCounterBuffer)
	{
		if(restrictionSRV)
		{
			if(spawnPeriod == 0)
			{
				StabilityNextStepClickRuleRestricted(dc, clickRuleBuffer, clickRuleCounterBuffer, restrictionSRV);
			}
			else
			{
				StabilityNextStepClickRuleSpawnRestricted(dc, clickRuleBuffer, clickRuleCounterBuffer, restrictionSRV, spawnPeriod);
			}
		}
		else
		{
			if(spawnPeriod == 0)
			{
				StabilityNextStepClickRule(dc, clickRuleBuffer, clickRuleCounterBuffer);
			}
			else
			{
				StabilityNextStepClickRuleSpawn(dc, clickRuleBuffer, clickRuleCounterBuffer, spawnPeriod);
			}
		}
	}
	else
	{
		if(restrictionSRV)
		{
			if(spawnPeriod == 0)
			{
				StabilityNextStepRestricted(dc, restrictionSRV);
			}
			else
			{
				StabilityNextStepSpawnRestricted(dc, restrictionSRV, spawnPeriod);
			}
		}
		else
		{
			if (spawnPeriod == 0)
			{
				StabilityNextStepNormal(dc);
			}
			else
			{
				StabilityNextStepSpawn(dc, spawnPeriod);
			}
		}
	}

	ID3D11Buffer*          nullCBuffers[] = { nullptr };
	ID3D11ShaderResourceView*  nullSRVs[] = { nullptr, nullptr, nullptr, nullptr, nullptr };
	ID3D11UnorderedAccessView* nullUAVs[] = { nullptr, nullptr };

	dc->CSSetConstantBuffers(     0, 1, nullCBuffers);
	dc->CSSetShaderResources(     0, 5, nullSRVs);
	dc->CSSetUnorderedAccessViews(0, 2, nullUAVs, nullptr);

	dc->CSSetShader(nullptr, nullptr, 0);

	std::swap(mCurrStabilitySRV, mPrevStabilitySRV);
	std::swap(mCurrStabilityUAV, mPrevStabilityUAV);
	std::swap(mCurrBoardSRV,     mPrevBoardSRV);
	std::swap(mCurrBoardUAV,     mPrevBoardUAV);

	mCurrentStep++;
}

uint32_t StabilityCalculator::GetBoardWidth() const
{
	return mBoardWidth;
}

uint32_t StabilityCalculator::GetBoardHeight() const
{
	return mBoardHeight;
}

uint32_t StabilityCalculator::GetCurrentStep() const
{
	return mCurrentStep;
}

uint32_t StabilityCalculator::GetDefaultSolutionPeriod() const
{
	//For any normal Lights Out game of size (2^n - 1) x (2^n - 1), the solution period is 2^(n - 1).  
	//For example, for the normal 127 x 127 Lights Out the period is 64, for the normal 255 x 255 Lights Out the period is 128 and so on.
	//However, for the custom click rules, spawn stability and many other things this formula doesn't work anymore. 
	//But since we need the default solution period anyway, part of it stays. To control the larger periods, I added enlonging multiplier.
	return ((mBoardWidth + 1) / 2);
}

ID3D11ShaderResourceView* StabilityCalculator::GetLastStabilityState() const
{
	return mPrevStabilitySRV.Get();
}

ID3D11ShaderResourceView* StabilityCalculator::GetLastBoardState() const
{
	return mPrevBoardSRV.Get();
}

void StabilityCalculator::LoadShaderData(ID3D11Device* device)
{
	const std::wstring shaderDir = Utils::GetShaderPath() + L"NextStep\\";
	Microsoft::WRL::ComPtr<ID3DBlob> shaderBlob;

	ThrowIfFailed(Utils::LoadShaderFromFile(device, shaderDir + L"StabilityNextStepCS.cso",                         mStabilityNextStepShader.GetAddressOf()));
	ThrowIfFailed(Utils::LoadShaderFromFile(device, shaderDir + L"StabilityNextStepSpawnCS.cso",                    mStabilityNextStepSpawnShader.GetAddressOf()));
	ThrowIfFailed(Utils::LoadShaderFromFile(device, shaderDir + L"StabilityNextStepRestrictedCS.cso",               mStabilityNextStepRestrictionShader.GetAddressOf()));
	ThrowIfFailed(Utils::LoadShaderFromFile(device, shaderDir + L"StabilityNextStepSpawnRestrictedCS.cso",          mStabilityNextStepSpawnRestrictionShader.GetAddressOf()));
	ThrowIfFailed(Utils::LoadShaderFromFile(device, shaderDir + L"StabilityNextStepClickRuleCS.cso",                mStabilityNextStepClickRuleShader.GetAddressOf()));
	ThrowIfFailed(Utils::LoadShaderFromFile(device, shaderDir + L"StabilityNextStepSpawnClickRuleCS.cso",           mStabilityNextStepClickRuleSpawnShader.GetAddressOf()));
	ThrowIfFailed(Utils::LoadShaderFromFile(device, shaderDir + L"StabilityNextStepClickRuleRestrictedCS.cso",      mStabilityNextStepClickRuleRestrictionShader.GetAddressOf()));
	ThrowIfFailed(Utils::LoadShaderFromFile(device, shaderDir + L"StabilityNextStepSpawnClickRuleRestrictedCS.cso", mStabilityNextStepClickRuleSpawnRestrictionShader.GetAddressOf()));

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

void StabilityCalculator::ReinitTextures(ID3D11Device* device, ID3D11Texture2D* initialBoard)
{
	mPrevStabilitySRV.Reset();
	mCurrStabilitySRV.Reset();
	mPrevStabilityUAV.Reset();
	mCurrStabilityUAV.Reset();

	mPrevBoardSRV.Reset();
	mCurrBoardSRV.Reset();
	mPrevBoardUAV.Reset();
	mCurrBoardUAV.Reset();

	D3D11_TEXTURE2D_DESC boardTexDesc;
	initialBoard->GetDesc(&boardTexDesc);
	boardTexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;

	mBoardWidth  = boardTexDesc.Width;
	mBoardHeight = boardTexDesc.Height;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> prevStabilityTex = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> currStabilityTex = nullptr;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> prevBoardTex = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> currBoardTex = nullptr;

	ThrowIfFailed(device->CreateTexture2D(&boardTexDesc, nullptr, prevStabilityTex.GetAddressOf()));
	ThrowIfFailed(device->CreateTexture2D(&boardTexDesc, nullptr, currStabilityTex.GetAddressOf()));

	ThrowIfFailed(device->CreateTexture2D(&boardTexDesc, nullptr, prevBoardTex.GetAddressOf()));
	ThrowIfFailed(device->CreateTexture2D(&boardTexDesc, nullptr, currBoardTex.GetAddressOf()));

	D3D11_SHADER_RESOURCE_VIEW_DESC boardSrvDesc;
	boardSrvDesc.Format                    = DXGI_FORMAT_R8_UINT;
	boardSrvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
	boardSrvDesc.Texture2D.MipLevels       = 1;
	boardSrvDesc.Texture2D.MostDetailedMip = 0;

	ThrowIfFailed(device->CreateShaderResourceView(prevStabilityTex.Get(), &boardSrvDesc, mPrevStabilitySRV.GetAddressOf()));
	ThrowIfFailed(device->CreateShaderResourceView(currStabilityTex.Get(), &boardSrvDesc, mCurrStabilitySRV.GetAddressOf()));

	ThrowIfFailed(device->CreateShaderResourceView(prevBoardTex.Get(), &boardSrvDesc, mPrevBoardSRV.GetAddressOf()));
	ThrowIfFailed(device->CreateShaderResourceView(currBoardTex.Get(), &boardSrvDesc, mCurrBoardSRV.GetAddressOf()));

	D3D11_UNORDERED_ACCESS_VIEW_DESC boardUavDesc;
	boardUavDesc.Format             = DXGI_FORMAT_R8_UINT;
	boardUavDesc.ViewDimension      = D3D11_UAV_DIMENSION_TEXTURE2D;
	boardUavDesc.Texture2D.MipSlice = 0;

	ThrowIfFailed(device->CreateUnorderedAccessView(prevStabilityTex.Get(), &boardUavDesc, mPrevStabilityUAV.GetAddressOf()));
	ThrowIfFailed(device->CreateUnorderedAccessView(currStabilityTex.Get(), &boardUavDesc, mCurrStabilityUAV.GetAddressOf()));

	ThrowIfFailed(device->CreateUnorderedAccessView(prevBoardTex.Get(), &boardUavDesc, mPrevBoardUAV.GetAddressOf()));
	ThrowIfFailed(device->CreateUnorderedAccessView(currBoardTex.Get(), &boardUavDesc, mCurrBoardUAV.GetAddressOf()));
}

void StabilityCalculator::StabilityNextStepNormal(ID3D11DeviceContext* dc)
{
	ID3D11ShaderResourceView*  stabilityNextStepSRVs[] = { mPrevBoardSRV.Get(), mPrevStabilitySRV.Get() };
	ID3D11UnorderedAccessView* stabilityNextStepUAVs[] = { mCurrBoardUAV.Get(), mCurrStabilityUAV.Get() };

	dc->CSSetShaderResources(0, 2, stabilityNextStepSRVs);
	dc->CSSetUnorderedAccessViews(0, 2, stabilityNextStepUAVs, nullptr);

	dc->CSSetShader(mStabilityNextStepShader.Get(), nullptr, 0);
	dc->Dispatch((uint32_t)(ceilf(mBoardWidth / 32.0f)), (uint32_t)(ceilf(mBoardHeight / 32.0f)), 1);
}

void StabilityCalculator::StabilityNextStepClickRule(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* clickRuleBuffer, ID3D11ShaderResourceView* clickRuleCounterBuffer)
{
	ID3D11ShaderResourceView*  stabilityNextStepClickRuleSRVs[] = { mPrevBoardSRV.Get(), mPrevStabilitySRV.Get(), clickRuleBuffer, clickRuleCounterBuffer };
	ID3D11UnorderedAccessView* stabilityNextStepClickRuleUAVs[] = { mCurrBoardUAV.Get(), mCurrStabilityUAV.Get() };

	dc->CSSetShaderResources(0, 4, stabilityNextStepClickRuleSRVs);
	dc->CSSetUnorderedAccessViews(0, 2, stabilityNextStepClickRuleUAVs, nullptr);

	dc->CSSetShader(mStabilityNextStepClickRuleShader.Get(), nullptr, 0);
	dc->Dispatch((uint32_t)(ceilf(mBoardWidth / 32.0f)), (uint32_t)(ceilf(mBoardHeight / 32.0f)), 1);
}

void StabilityCalculator::StabilityNextStepSpawn(ID3D11DeviceContext* dc, uint32_t spawnPeriod)
{
	mCBufferParamsCopy.SpawnPeriod = spawnPeriod;
	Utils::UpdateBuffer(mCBufferParams.Get(), mCBufferParamsCopy, dc);

	ID3D11Buffer*          stabilityNextStepSpawnCBuffers[] = { mCBufferParams.Get() };
	ID3D11ShaderResourceView*  stabilityNextStepSpawnSRVs[] = { mPrevBoardSRV.Get(), mPrevStabilitySRV.Get() };
	ID3D11UnorderedAccessView* stabilityNextStepSpawnUAVs[] = { mCurrBoardUAV.Get(), mCurrStabilityUAV.Get() };

	dc->CSSetConstantBuffers(0, 1, stabilityNextStepSpawnCBuffers);
	dc->CSSetShaderResources(0, 2, stabilityNextStepSpawnSRVs);
	dc->CSSetUnorderedAccessViews(0, 2, stabilityNextStepSpawnUAVs, nullptr);

	dc->CSSetShader(mStabilityNextStepSpawnShader.Get(), nullptr, 0);
	dc->Dispatch((uint32_t)(ceilf(mBoardWidth / 32.0f)), (uint32_t)(ceilf(mBoardHeight / 32.0f)), 1);
}

void StabilityCalculator::StabilityNextStepClickRuleSpawn(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* clickRuleBuffer, ID3D11ShaderResourceView* clickRuleCounterBuffer, uint32_t spawnPeriod)
{
	mCBufferParamsCopy.SpawnPeriod = spawnPeriod;
	Utils::UpdateBuffer(mCBufferParams.Get(), mCBufferParamsCopy, dc);

	ID3D11Buffer*              stabilityNextStepSpawnClickRuleCBuffers[] = { mCBufferParams.Get() };
	ID3D11ShaderResourceView*  stabilityNextStepSpawnClickRuleSRVs[]     = { mPrevBoardSRV.Get(), mPrevStabilitySRV.Get(), clickRuleBuffer, clickRuleCounterBuffer };
	ID3D11UnorderedAccessView* stabilityNextStepSpawnClickRuleUAVs[]     = { mCurrBoardUAV.Get(), mCurrStabilityUAV.Get() };

	dc->CSSetConstantBuffers(0, 1, stabilityNextStepSpawnClickRuleCBuffers);
	dc->CSSetShaderResources(0, 4, stabilityNextStepSpawnClickRuleSRVs);
	dc->CSSetUnorderedAccessViews(0, 2, stabilityNextStepSpawnClickRuleUAVs, nullptr);

	dc->CSSetShader(mStabilityNextStepClickRuleSpawnShader.Get(), nullptr, 0);
	dc->Dispatch((uint32_t)(ceilf(mBoardWidth / 32.0f)), (uint32_t)(ceilf(mBoardHeight / 32.0f)), 1);
}

void StabilityCalculator::StabilityNextStepRestricted(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* restrictionSRV)
{
	ID3D11ShaderResourceView*  stabilityNextStepSRVs[] = { mPrevBoardSRV.Get(), mPrevStabilitySRV.Get(), restrictionSRV };
	ID3D11UnorderedAccessView* stabilityNextStepUAVs[] = { mCurrBoardUAV.Get(), mCurrStabilityUAV.Get() };

	dc->CSSetShaderResources(0, 3, stabilityNextStepSRVs);
	dc->CSSetUnorderedAccessViews(0, 2, stabilityNextStepUAVs, nullptr);

	dc->CSSetShader(mStabilityNextStepRestrictionShader.Get(), nullptr, 0);
	dc->Dispatch((uint32_t)(ceilf(mBoardWidth / 32.0f)), (uint32_t)(ceilf(mBoardHeight / 32.0f)), 1);
}

void StabilityCalculator::StabilityNextStepClickRuleRestricted(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* clickRuleBuffer, ID3D11ShaderResourceView* clickRuleCounterBuffer, ID3D11ShaderResourceView* restrictionSRV)
{
	ID3D11ShaderResourceView*  stabilityNextStepClickRuleSRVs[] = { mPrevBoardSRV.Get(), mPrevStabilitySRV.Get(), restrictionSRV, clickRuleBuffer, clickRuleCounterBuffer };
	ID3D11UnorderedAccessView* stabilityNextStepClickRuleUAVs[] = { mCurrBoardUAV.Get(), mCurrStabilityUAV.Get() };

	dc->CSSetShaderResources(0, 5, stabilityNextStepClickRuleSRVs);
	dc->CSSetUnorderedAccessViews(0, 2, stabilityNextStepClickRuleUAVs, nullptr);

	dc->CSSetShader(mStabilityNextStepClickRuleRestrictionShader.Get(), nullptr, 0);
	dc->Dispatch((uint32_t)(ceilf(mBoardWidth / 32.0f)), (uint32_t)(ceilf(mBoardHeight / 32.0f)), 1);
}

void StabilityCalculator::StabilityNextStepSpawnRestricted(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* restrictionSRV, uint32_t spawnPeriod)
{
	mCBufferParamsCopy.SpawnPeriod = spawnPeriod;
	Utils::UpdateBuffer(mCBufferParams.Get(), mCBufferParamsCopy, dc);

	ID3D11Buffer*          stabilityNextStepSpawnCBuffers[] = { mCBufferParams.Get() };
	ID3D11ShaderResourceView*  stabilityNextStepSpawnSRVs[] = { mPrevBoardSRV.Get(), mPrevStabilitySRV.Get(), restrictionSRV };
	ID3D11UnorderedAccessView* stabilityNextStepSpawnUAVs[] = { mCurrBoardUAV.Get(), mCurrStabilityUAV.Get() };

	dc->CSSetConstantBuffers(0, 1, stabilityNextStepSpawnCBuffers);
	dc->CSSetShaderResources(0, 3, stabilityNextStepSpawnSRVs);
	dc->CSSetUnorderedAccessViews(0, 2, stabilityNextStepSpawnUAVs, nullptr);

	dc->CSSetShader(mStabilityNextStepSpawnRestrictionShader.Get(), nullptr, 0);
	dc->Dispatch((uint32_t)(ceilf(mBoardWidth / 32.0f)), (uint32_t)(ceilf(mBoardHeight / 32.0f)), 1);
}

void StabilityCalculator::StabilityNextStepClickRuleSpawnRestricted(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* clickRuleBuffer, ID3D11ShaderResourceView* clickRuleCounterBuffer, ID3D11ShaderResourceView* restrictionSRV, uint32_t spawnPeriod)
{
	mCBufferParamsCopy.SpawnPeriod = spawnPeriod;
	Utils::UpdateBuffer(mCBufferParams.Get(), mCBufferParamsCopy, dc);

	ID3D11Buffer*              stabilityNextStepSpawnClickRuleCBuffers[] = { mCBufferParams.Get() };
	ID3D11ShaderResourceView*  stabilityNextStepSpawnClickRuleSRVs[]     = { mPrevBoardSRV.Get(), mPrevStabilitySRV.Get(), restrictionSRV, clickRuleBuffer, clickRuleCounterBuffer };
	ID3D11UnorderedAccessView* stabilityNextStepSpawnClickRuleUAVs[]     = { mCurrBoardUAV.Get(), mCurrStabilityUAV.Get() };

	dc->CSSetConstantBuffers(0, 1, stabilityNextStepSpawnClickRuleCBuffers);
	dc->CSSetShaderResources(0, 5, stabilityNextStepSpawnClickRuleSRVs);
	dc->CSSetUnorderedAccessViews(0, 2, stabilityNextStepSpawnClickRuleUAVs, nullptr);

	dc->CSSetShader(mStabilityNextStepClickRuleSpawnRestrictionShader.Get(), nullptr, 0);
	dc->Dispatch((uint32_t)(ceilf(mBoardWidth / 32.0f)), (uint32_t)(ceilf(mBoardHeight / 32.0f)), 1);
}
