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
#include "Boards.hpp"
#include "BoardLoader.hpp"
#include "BoardSaver.hpp"
#include "../App/Renderer.hpp"

FractalGen::FractalGen(Renderer* renderer): mRenderer(renderer), mVideoFrameWidth(1), mVideoFrameHeight(1), mSpawnPeriod(0), mbUseSmoothTransform(false)
{
	ID3D11Device*    device = mRenderer->GetDevice();
	ID3D11DeviceContext* dc = mRenderer->GetDeviceContext();

	mStabilityCalculator = std::make_unique<StabilityCalculator>(device);

	mDownscaler       = std::make_unique<Downscaler>(device);
	mFinalTransformer = std::make_unique<FinalTransformer>(device);
	mEqualityChecker  = std::make_unique<EqualityChecker>(device);

	mBoards        = std::make_unique<Boards>(device);
	mClickRules    = std::make_unique<ClickRules>(device);
	mBoardLoader   = std::make_unique<BoardLoader>(device);
	mBoardSaver    = std::make_unique<BoardSaver>();

	Init4CornersBoard(1023, 1023);
	mBoards->InitDefaultRestriction(mRenderer->GetDevice(), mRenderer->GetDeviceContext());
	InitDefaultClickRule();
}

FractalGen::~FractalGen()
{
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

void FractalGen::ChangeSize(uint32_t newWidth, uint32_t newHeight)
{
	mBoards->ChangeBoardSize(mRenderer->GetDevice(), mRenderer->GetDeviceContext(), newWidth, newHeight);
}

void FractalGen::InitDefaultClickRule()
{
	mClickRules->InitDefault(mRenderer->GetDevice());
	mRenderer->SetCurrentClickRule(mClickRules->GetClickRuleImageSRV());
	mRenderer->NeedRedrawClickRule();
}

Utils::BoardLoadError FractalGen::LoadClickRuleFromFile(const std::wstring& clickRuleFile)
{
	Microsoft::WRL::ComPtr<ID3D11Texture2D> clickRuleTex;
	Utils::BoardLoadError loadErr = mBoardLoader->LoadClickRuleFromFile(mRenderer->GetDevice(), mRenderer->GetDeviceContext(), clickRuleFile, clickRuleTex.GetAddressOf());

	if(loadErr == Utils::BoardLoadError::LOAD_SUCCESS)
	{
		mClickRules->CreateFromTexture(mRenderer->GetDevice(), clickRuleTex.Get());
		mRenderer->SetCurrentClickRule(mClickRules->GetClickRuleImageSRV());
		mRenderer->NeedRedrawClickRule();
	}

	return loadErr;
}

uint32_t FractalGen::GetLastFrameNumber() const
{
	return mStabilityCalculator->GetCurrentStep();
}

uint32_t FractalGen::GetDefaultSolutionPeriod(uint32_t boardSize) const
{
	return mStabilityCalculator->GetDefaultSolutionPeriod(boardSize);
}

uint32_t FractalGen::GetWidth() const
{
	return mBoards->GetWidth();
}

uint32_t FractalGen::GetHeight() const
{
	return mBoards->GetHeight();
}

void FractalGen::EditClickRule(float normalizedX, float normalizedY)
{
	uint32_t clickRuleWidth  = mClickRules->GetWidth();
	uint32_t clickRuleHeight = mClickRules->GetHeight();

	mClickRules->EditCellState(mRenderer->GetDeviceContext(), (uint32_t)(clickRuleWidth * normalizedX), (uint32_t)(clickRuleHeight * normalizedY));
	mRenderer->SetCurrentClickRule(mClickRules->GetClickRuleImageSRV());
	mRenderer->NeedRedrawClickRule();
}

void FractalGen::Init4CornersBoard(uint32_t width, uint32_t height)
{
	mBoards->Init4CornersBoard(mRenderer->GetDevice(), mRenderer->GetDeviceContext(), width, height);
}

void FractalGen::Init4SidesBoard(uint32_t width, uint32_t height)
{
	mBoards->Init4SidesBoard(mRenderer->GetDevice(), mRenderer->GetDeviceContext(), width, height);
}

void FractalGen::InitCenterBoard(uint32_t width, uint32_t height)
{
	mBoards->InitCenterBoard(mRenderer->GetDevice(), mRenderer->GetDeviceContext(), width, height);
}

Utils::BoardLoadError FractalGen::LoadBoardFromFile(const std::wstring& boardFile)
{
	Microsoft::WRL::ComPtr<ID3D11Texture2D> initialBoardTex;
	Utils::BoardLoadError loadErr = mBoardLoader->LoadBoardFromFile(mRenderer->GetDevice(), mRenderer->GetDeviceContext(), boardFile, initialBoardTex.GetAddressOf());

	if(loadErr == Utils::BoardLoadError::LOAD_SUCCESS)
	{
		mBoards->InitBoardFromTexture(mRenderer->GetDevice(), mRenderer->GetDeviceContext(), initialBoardTex.Get());
	}

	return loadErr;
}

void FractalGen::InitDefaultRestriction()
{
	mBoards->InitDefaultRestriction(mRenderer->GetDevice(), mRenderer->GetDeviceContext());
}

Utils::BoardLoadError FractalGen::LoadRestrictionFromFile(const std::wstring& restrictionFile)
{
	Microsoft::WRL::ComPtr<ID3D11Texture2D> restrictionTex;
	Utils::BoardLoadError loadErr = mBoardLoader->LoadBoardFromFile(mRenderer->GetDevice(), mRenderer->GetDeviceContext(), restrictionFile, restrictionTex.GetAddressOf());
	
	if(loadErr == Utils::BoardLoadError::LOAD_SUCCESS)
	{
		D3D11_TEXTURE2D_DESC restrictionTexDesc;
		restrictionTex->GetDesc(&restrictionTexDesc);

		if(restrictionTexDesc.Width != GetWidth() || restrictionTexDesc.Height != GetHeight()) //Restriction size is more restricted (Ha!), it should be the same size as board 
		{
			return Utils::BoardLoadError::ERROR_WRONG_SIZE;
		}

		mBoards->InitRestrictionFromTexture(mRenderer->GetDevice(), mRenderer->GetDeviceContext(), restrictionTex.Get());
	}

	return loadErr;
}

void FractalGen::ResetComputingParameters()
{
	mClickRules->Bake(mRenderer->GetDeviceContext());
	mStabilityCalculator->PrepareForCalculations(mRenderer->GetDevice(), mRenderer->GetDeviceContext(), mBoards->GetInitialBoardTex());

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

	mFinalTransformer->ComputeTransform(mRenderer->GetDeviceContext(), mStabilityCalculator->GetLastStabilityState(), mSpawnPeriod, mbUseSmoothTransform);
	mRenderer->SetCurrentBoard(mFinalTransformer->GetTransformedSRV());
	mRenderer->NeedRedraw();
}

void FractalGen::Tick()
{
	ID3D11ShaderResourceView* clickRuleBufferSRV  = nullptr;
	ID3D11ShaderResourceView* clickRuleCounterSRV = nullptr;
	if(!mClickRules->IsDefault())
	{
		clickRuleBufferSRV  = mClickRules->GetClickRuleBufferSRV();
		clickRuleCounterSRV = mClickRules->GetClickRuleBufferCounterSRV();
	}

	mStabilityCalculator->StabilityNextStep(mRenderer->GetDeviceContext(), clickRuleBufferSRV, clickRuleCounterSRV, mBoards->GetRestrictionSRV(), mSpawnPeriod);
	mFinalTransformer->ComputeTransform(mRenderer->GetDeviceContext(), mStabilityCalculator->GetLastStabilityState(), mSpawnPeriod, mbUseSmoothTransform);

	mRenderer->SetCurrentBoard(mFinalTransformer->GetTransformedSRV());
	mRenderer->NeedRedraw();
}

void FractalGen::SaveCurrentVideoFrame(const std::wstring& videoFrameFile)
{
	mDownscaler->DownscalePicture(mRenderer->GetDeviceContext(), mFinalTransformer->GetTransformedSRV());

	Microsoft::WRL::ComPtr<ID3D11Texture2D> downscaledTex;
	mDownscaler->GetDownscaledSRV()->GetResource(reinterpret_cast<ID3D11Resource**>(downscaledTex.GetAddressOf()));

	mBoardSaver->SaveVideoFrameToFile(mRenderer->GetDevice(), mRenderer->GetDeviceContext(), downscaledTex.Get(), videoFrameFile);
}

void FractalGen::SaveCurrentStep(const std::wstring& stabilityFile)
{
	Microsoft::WRL::ComPtr<ID3D11Texture2D> stabilityTex;
	mFinalTransformer->GetTransformedSRV()->GetResource(reinterpret_cast<ID3D11Resource**>(stabilityTex.GetAddressOf()));

	mBoardSaver->SaveBoardToFile(mRenderer->GetDevice(), mRenderer->GetDeviceContext(), stabilityTex.Get(), stabilityFile);
}

void FractalGen::SaveClickRule(const std::wstring& clickRuleFile)
{
	Microsoft::WRL::ComPtr<ID3D11Texture2D> clickRuleTex;
	mClickRules->GetClickRuleImageSRV()->GetResource(reinterpret_cast<ID3D11Resource**>(clickRuleTex.GetAddressOf()));

	mBoardSaver->SaveClickRuleToFile(mRenderer->GetDevice(), mRenderer->GetDeviceContext(), clickRuleTex.Get(), clickRuleFile);
}
