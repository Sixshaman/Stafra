#pragma once

#include <wrl/client.h>
#include <d3d11.h>
#include <cstdint>

class StabilityCalculator
{
	struct CBParamsStruct
	{
		uint32_t SpawnPeriod;
	};

public:
	StabilityCalculator(ID3D11Device* device, uint32_t width, uint32_t height, uint32_t spawnPeriod = 0);
	~StabilityCalculator();

	void InitTextures(ID3D11Device* device, ID3D11DeviceContext* dc, uint32_t sizeLO);
	void StabilityNextStep(ID3D11DeviceContext* dc);
	bool CheckEquality(ID3D11Device* device, ID3D11DeviceContext* dc);

	ID3D11ShaderResourceView* GetLastStabilityState() const;

private:
	void CreateTextures(ID3D11Device* device);
	void LoadShaderData(ID3D11Device* device);

	void StabilityNextStepNormal(ID3D11DeviceContext* dc);
	void StabilityNextStepClickRule(ID3D11DeviceContext* dc);
	void StabilityNextStepSpawn(ID3D11DeviceContext* dc);
	void StabilityNextStepClickRuleSpawn(ID3D11DeviceContext* dc);
	void StabilityNextStepRestricted(ID3D11DeviceContext* dc);
	void StabilityNextStepClickRuleRestricted(ID3D11DeviceContext* dc);
	void StabilityNextStepSpawnRestricted(ID3D11DeviceContext* dc);
	void StabilityNextStepClickRuleSpawnRestricted(ID3D11DeviceContext* dc);

private:
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  mPrevStabilitySRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  mCurrStabilitySRV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> mPrevStabilityUAV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> mCurrStabilityUAV;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  mPrevBoardSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  mCurrBoardSRV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> mPrevBoardUAV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> mCurrBoardUAV;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  mInitialBoardSRV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> mInitialBoardUAV;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  mClickRuleSRV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> mClickRuleUAV;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  mRestrictionSRV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> mRestrictionUAV;

	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mStabilityNextStepShader;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mStabilityNextStepClickRuleShader;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mStabilityNextStepSPAWNShader;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mStabilityNextStepSPAWNCLICKRUUUULEShader;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mStabilityNextStepRESTTERIVCTIONShader;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mStabilityNextStepClickRuleERTESTRICVTIONShader;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mStabilityNextStepSPAWNAAAAAAAARESTTRTICTIONShader;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mStabilityNextStepSPAWNCLICKRUUUULEATEWWEAFOSDFSTRESTRICTIONShader; //I'M TOO EXCITED! TOO EXCITED!!!!!!!

	uint32_t mBoardWidth;
	uint32_t mBoardHeight;

	Microsoft::WRL::ComPtr<ID3D11Buffer> mCBufferParams;
	CBParamsStruct                       mCBufferParamsCopy;

	uint32_t mSpawnPeriod;
	bool     mbUseClickRule;
	bool     mbUseRestriction;
};