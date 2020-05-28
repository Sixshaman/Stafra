#pragma once

#include <wrl/client.h>
#include <d3d11.h>
#include <cstdint>

/*
The class for computing stability fractal iterations.
Input:               Initial board, click rule, restriction
Output:              Last computed stability iteration
Possible expansions: More variants of computations
*/

class StabilityCalculator
{
	struct CBParamsStruct
	{
		uint32_t SpawnPeriod;
	};

public:
	StabilityCalculator(ID3D11Device* device);
	~StabilityCalculator();

	void PrepareForCalculations(ID3D11Device* device, ID3D11DeviceContext* dc, ID3D11Texture2D* initialBoard);
	void StabilityNextStep(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* clickRuleBuffer, ID3D11ShaderResourceView* clickRuleCounterBuffer, ID3D11ShaderResourceView* restrictionSRV, uint32_t spawnPeriod);

	uint32_t GetBoardWidth()  const;
	uint32_t GetBoardHeight() const;

	uint32_t GetCurrentStep()                             const;
	uint32_t GetDefaultSolutionPeriod(uint32_t boardSize) const;

	ID3D11ShaderResourceView* GetLastStabilityState() const;
	ID3D11ShaderResourceView* GetLastBoardState()     const;

private:
	void LoadShaderData(ID3D11Device* device);
	void ReinitTextures(ID3D11Device* device, ID3D11Texture2D* initialBoard);

	void                   StabilityNextStepNormal(ID3D11DeviceContext* dc                                                                                                                                                             );
	void                StabilityNextStepClickRule(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* clickRuleBuffer, ID3D11ShaderResourceView* clickRuleCounterBuffer                                                                );
	void                    StabilityNextStepSpawn(ID3D11DeviceContext* dc,                                                                                                                                        uint32_t spawnPeriod);
	void           StabilityNextStepClickRuleSpawn(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* clickRuleBuffer, ID3D11ShaderResourceView* clickRuleCounterBuffer,                                           uint32_t spawnPeriod);
	void               StabilityNextStepRestricted(ID3D11DeviceContext* dc,                                                                                              ID3D11ShaderResourceView* restrictionSRV                      );
	void      StabilityNextStepClickRuleRestricted(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* clickRuleBuffer, ID3D11ShaderResourceView* clickRuleCounterBuffer, ID3D11ShaderResourceView* restrictionSRV                      );
	void          StabilityNextStepSpawnRestricted(ID3D11DeviceContext* dc,                                                                                              ID3D11ShaderResourceView* restrictionSRV, uint32_t spawnPeriod);
	void StabilityNextStepClickRuleSpawnRestricted(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* clickRuleBuffer, ID3D11ShaderResourceView* clickRuleCounterBuffer, ID3D11ShaderResourceView* restrictionSRV, uint32_t spawnPeriod);

private:
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  mPrevStabilitySRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  mCurrStabilitySRV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> mPrevStabilityUAV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> mCurrStabilityUAV;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  mPrevBoardSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  mCurrBoardSRV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> mPrevBoardUAV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> mCurrBoardUAV;

	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mStabilityNextStepShader;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mStabilityNextStepSpawnShader;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mStabilityNextStepRestrictionShader;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mStabilityNextStepSpawnRestrictionShader;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mStabilityNextStepClickRuleShader;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mStabilityNextStepClickRuleSpawnShader;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mStabilityNextStepClickRuleRestrictionShader;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mStabilityNextStepClickRuleSpawnRestrictionShader;

	uint32_t mBoardWidth;
	uint32_t mBoardHeight;

	uint32_t mCurrentStep;

	Microsoft::WRL::ComPtr<ID3D11Buffer> mCBufferParams;
	CBParamsStruct                       mCBufferParamsCopy;
};