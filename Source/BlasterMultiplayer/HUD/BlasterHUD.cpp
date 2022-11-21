// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterHUD.h"
#include "GameFramework/PlayerController.h"
#include "CharacterOverlay.h"

void ABlasterHUD::BeginPlay()
{
	Super::BeginPlay();

	AddCharacterOverlay();
}

void ABlasterHUD::AddCharacterOverlay()
{
	APlayerController* PlayerController = GetOwningPlayerController();

	if (PlayerController && m_CharacterOverlayClass)
	{
		m_CharacterOverlay = CreateWidget<UCharacterOverlay>(PlayerController, m_CharacterOverlayClass);

		m_CharacterOverlay->AddToViewport();
	}
}

void ABlasterHUD::DrawHUD()
{
	Super::DrawHUD();

	// Where To Draw HUD
	FVector2D ViewportSize;

	if (GEngine)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);

		const FVector2D ViewportCenter(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);

		// CrosshairSpread 는 실시간 바뀔 것이다. by CombatComponent
		float SpreadScaled = m_CrosshairSpreadMax * m_HUDPackage.CrosshairSpread;

		// No Textures If Not Equipped Weapon
		if (m_HUDPackage.CrosshairsCenter)
		{
			// Draw At Center -> Texture 의 왼쪽위가, 가운데 오게 되므로,
			// 왼쪽 + 위쪽. 으로 이동시켜야 한다.
			FVector2D Spread(0.f, 0.f);
			DrawCrosshair(m_HUDPackage.CrosshairsCenter, ViewportCenter, Spread, m_HUDPackage.CrosshairsColor);
		}

		if (m_HUDPackage.CrosshairsLeft)
		{
			// 오직 Left 로만 움직이게 하고 싶다
			FVector2D Spread(-1 * SpreadScaled, 0.f);

			DrawCrosshair(m_HUDPackage.CrosshairsLeft, ViewportCenter, Spread, m_HUDPackage.CrosshairsColor);
		}

		if (m_HUDPackage.CrosshairsRight)
		{
			// 오직 Right 로만 움직이게 하고 싶다
			FVector2D Spread(SpreadScaled, 0.f);
			DrawCrosshair(m_HUDPackage.CrosshairsRight, ViewportCenter, Spread, m_HUDPackage.CrosshairsColor);
		}

		if (m_HUDPackage.CrosshairsTop)
		{
			// 오직 Top 로만 움직이게 하고 싶다 (Texture Y 방향은 반대로 고려)
			FVector2D Spread(0.f, -1 * SpreadScaled);
			DrawCrosshair(m_HUDPackage.CrosshairsTop, ViewportCenter, Spread, m_HUDPackage.CrosshairsColor);
		}

		if (m_HUDPackage.CrosshairsBottom)
		{
			// 오직 Bottom 으로만 움직이게 하고 싶다. (Texture Y 방향은 반대로 고려)
			FVector2D Spread(0.f, SpreadScaled);
			DrawCrosshair(m_HUDPackage.CrosshairsBottom, ViewportCenter, Spread, m_HUDPackage.CrosshairsColor);
		}
	}
}

void ABlasterHUD::DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, 
	FVector2D Spread, FLinearColor CrosshairColor)
{
	const float TextureWidth  = Texture->GetSizeX();
	const float TextureHeight = Texture->GetSizeY();

	const FVector2D TextureDrawPoint(
		ViewportCenter.X - (TextureWidth / 2.f) + Spread.X,
		ViewportCenter.Y - (TextureHeight / 2.f) + Spread.Y
	);

	DrawTexture(Texture, TextureDrawPoint.X, TextureDrawPoint.Y, TextureWidth, TextureHeight, 
		0.f, 0.f, 
		1.f, 1.f,
		CrosshairColor);
}
