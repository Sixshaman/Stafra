#include "FractalGen.hpp"
#include "../Util.hpp"
#include <iostream>
#include <d3dcompiler.h>
#include <vector>
#include <fstream>
#include <sstream>
#include "EqualityChecker.hpp"
#include "StabilityCalculator.hpp"
#include "Downscaler.hpp"
#include "FinalTransform.hpp"
#include "ClickRules.hpp"
#include "InitialStates.hpp"
#include "BoardLoader.hpp"
#include "BoardSaver.hpp"
#include "../App/Renderer.hpp"

FractalGen::FractalGen(Renderer* renderer): mRenderer(renderer), mDefaultBoardWidth(1023), mDefaultBoardHeight(1023), mVideoFrameWidth(1), mVideoFrameHeight(1), mSpawnPeriod(0), mbUseSmoothTransform(false)
{
	ID3D11Device*    device = mRenderer->GetDevice();
	ID3D11DeviceContext* dc = mRenderer->GetDeviceContext();

	mStabilityCalculator = std::make_unique<StabilityCalculator>(device);

	mDownscaler       = std::make_unique<Downscaler>(device);
	mFinalTransformer = std::make_unique<FinalTransformer>(device);
	mEqualityChecker  = std::make_unique<EqualityChecker>(device);

	mClickRules    = std::make_unique<ClickRules>(device);
	mInitialStates = std::make_unique<InitialStates>(device);
	mBoardLoader   = std::make_unique<BoardLoader>(device);
	mBoardSaver    = std::make_unique<BoardSaver>();

	mClickRules->InitDefault(mRenderer->GetDevice()); //WORKAROUND
}

FractalGen::~FractalGen()
{
}

void FractalGen::SetDefaultBoardWidth(uint32_t width)
{
	mDefaultBoardWidth = width;
}

void FractalGen::SetDefaultBoardHeight(uint32_t height)
{
	mDefaultBoardHeight = height;
}

void FractalGen::SetVideoFrameWidth(uint32_t width)
{
	mVideoFrameWidth = width;
}

void FractalGen::SetVideoFrameHeight(uint32_t height)
{
	mVideoFrameHeight = height;
}

void FractalGen::SetSpawnPeriod(uint32_t spawn)
{
	mSpawnPeriod = spawn;
}

void FractalGen::SetUseSmooth(bool smooth)
{
	mbUseSmoothTransform = smooth;
}

void FractalGen::InitDefaultClickRule()
{
	mClickRules->InitDefault(mRenderer->GetDevice());
	mRenderer->SetCurrentClickRule(mClickRules->GetClickRuleImageSRV());
	mRenderer->NeedRedrawClickRule();
}

bool FractalGen::LoadClickRuleFromFile(const std::wstring& clickRuleFile)
{
	Microsoft::WRL::ComPtr<ID3D11Texture2D> clickRuleTex;
	mBoardLoader->LoadClickRuleFromFile(mRenderer->GetDevice(), mRenderer->GetDeviceContext(), clickRuleFile, clickRuleTex.GetAddressOf());

	if(clickRuleTex)
	{
		mClickRules->CreateFromTexture(mRenderer->GetDevice(), clickRuleTex.Get());
		mRenderer->SetCurrentClickRule(mClickRules->GetClickRuleImageSRV());
		mRenderer->NeedRedrawClickRule();

		return true;
	}
	else
	{
		return false;
	}
}

uint32_t FractalGen::GetCurrentFrame() const
{
	return mStabilityCalculator->GetCurrentStep();
}

uint32_t FractalGen::GetSolutionPeriod() const
{
	return mStabilityCalculator->GetDefaultSolutionPeriod();
}

void FractalGen::EditClickRule(float normalizedX, float normalizedY)
{
	mCurrentlyComputing = true;

	uint32_t clickRuleWidth = mClickRules->GetWidth();
	uint32_t clickRuleHeight = mClickRules->GetHeight();

	mClickRules->EditCellState(mRenderer->GetDeviceContext(), (uint32_t)(clickRuleWidth * normalizedX), (uint32_t)(clickRuleHeight * normalizedY));
	mRenderer->SetCurrentClickRule(mClickRules->GetClickRuleImageSRV());
	mRenderer->NeedRedrawClickRule();

	mCurrentlyComputing = false;
}

void FractalGen::ResetComputingParameters(const std::wstring& initialBoardFile, const std::wstring& restrictionFile)
{
	mCurrentlyComputing = true;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> initialBoardTex;
	mBoardLoader->LoadBoardFromFile(mRenderer->GetDevice(), mRenderer->GetDeviceContext(), initialBoardFile, initialBoardTex.GetAddressOf());

	if(!initialBoardTex)
	{
		mInitialStates->CreateBoard(mRenderer->GetDevice(), mRenderer->GetDeviceContext(), mDefaultBoardWidth, mDefaultBoardHeight, BoardClearMode::FOUR_CORNERS, initialBoardTex.GetAddressOf());
	}

	mClickRules->Bake(mRenderer->GetDeviceContext());

	Microsoft::WRL::ComPtr<ID3D11Texture2D> restrictionTex;
	mBoardLoader->LoadBoardFromFile(mRenderer->GetDevice(), mRenderer->GetDeviceContext(), restrictionFile, restrictionTex.GetAddressOf());

	mStabilityCalculator->PrepareForCalculations(mRenderer->GetDevice(), mRenderer->GetDeviceContext(), initialBoardTex.Get(), restrictionTex.Get());

	uint32_t boardWidth  = mStabilityCalculator->GetBoardWidth();
	uint32_t boardHeight = mStabilityCalculator->GetBoardHeight();

	uint32_t clickRuleWidth  = mClickRules->GetWidth();
	uint32_t clickRuleHeight = mClickRules->GetHeight();

	mDownscaler->PrepareForDownscaling(mRenderer->GetDevice(), mVideoFrameWidth, mVideoFrameHeight);
	mFinalTransformer->PrepareForTransform(mRenderer->GetDevice(), boardWidth, boardHeight);
	mEqualityChecker->PrepareForCalculations(mRenderer->GetDevice(), boardWidth, boardHeight);

	mBoardSaver->PrepareStagingTextures(mRenderer->GetDevice(), boardWidth, boardHeight, mVideoFrameWidth, mVideoFrameHeight, clickRuleWidth, clickRuleHeight);

	mRenderer->SetCurrentClickRule(mClickRules->GetClickRuleImageSRV());
	mRenderer->NeedRedrawClickRule();

	mCurrentlyComputing = false;
}

void FractalGen::Tick()
{
	mCurrentlyComputing = true;

	ID3D11ShaderResourceView* clickRuleBufferSRV  = nullptr;
	ID3D11ShaderResourceView* clickRuleCounterSRV = nullptr;
	if(!mClickRules->IsDefault())
	{
		clickRuleBufferSRV  = mClickRules->GetClickRuleBufferSRV();
		clickRuleCounterSRV = mClickRules->GetClickRuleBufferCounterSRV();
	}

	mStabilityCalculator->StabilityNextStep(mRenderer->GetDeviceContext(), clickRuleBufferSRV, clickRuleCounterSRV, mSpawnPeriod);
	mFinalTransformer->ComputeTransform(mRenderer->GetDeviceContext(), mStabilityCalculator->GetLastStabilityState(), mSpawnPeriod, mbUseSmoothTransform);

	mRenderer->SetCurrentBoard(mFinalTransformer->GetTransformedSRV());
	mRenderer->NeedRedraw();

	mCurrentlyComputing = false;
}

void FractalGen::SaveCurrentVideoFrame(const std::wstring& videoFrameFile)
{
	mCurrentlyComputing = true;

	mDownscaler->DownscalePicture(mRenderer->GetDeviceContext(), mFinalTransformer->GetTransformedSRV());

	Microsoft::WRL::ComPtr<ID3D11Texture2D> downscaledTex;
	mDownscaler->GetDownscaledSRV()->GetResource(reinterpret_cast<ID3D11Resource**>(downscaledTex.GetAddressOf()));

	mBoardSaver->SaveVideoFrameToFile(mRenderer->GetDevice(), mRenderer->GetDeviceContext(), downscaledTex.Get(), videoFrameFile);

	mCurrentlyComputing = false;
}

void FractalGen::SaveCurrentStep(const std::wstring& stabilityFile)
{
	mCurrentlyComputing = true;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> stabilityTex;
	mFinalTransformer->GetTransformedSRV()->GetResource(reinterpret_cast<ID3D11Resource**>(stabilityTex.GetAddressOf()));

	mBoardSaver->SaveBoardToFile(mRenderer->GetDevice(), mRenderer->GetDeviceContext(), stabilityTex.Get(), stabilityFile);

	mCurrentlyComputing = false;
}

void FractalGen::SaveClickRule(const std::wstring& clickRuleFile)
{
	mCurrentlyComputing = true;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> clickRuleTex;
	mClickRules->GetClickRuleImageSRV()->GetResource(reinterpret_cast<ID3D11Resource**>(clickRuleTex.GetAddressOf()));

	mBoardSaver->SaveClickRuleToFile(mRenderer->GetDevice(), mRenderer->GetDeviceContext(), clickRuleTex.Get(), clickRuleFile);
	
	mCurrentlyComputing = false;
}
