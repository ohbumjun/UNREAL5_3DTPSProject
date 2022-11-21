// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterPlayerController.h"
#include "../HUD/BlasterHUD.h"
#include "../HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"


void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	m_BlasterHUD = Cast<ABlasterHUD>(GetHUD());
}

void ABlasterPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	m_BlasterHUD = m_BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : m_BlasterHUD;

	bool bHUDValid = m_BlasterHUD && 
		m_BlasterHUD->m_CharacterOverlay && 
		m_BlasterHUD->m_CharacterOverlay->Healthbar &&
		m_BlasterHUD->m_CharacterOverlay->HealthText;
	
	if (bHUDValid)
	{
		// Set Health Percent
		const float HealthPercent = Health / MaxHealth;

		m_BlasterHUD->m_CharacterOverlay->Healthbar->SetPercent(HealthPercent);

		FString HealthText = FString::Printf(TEXT("%d / %d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));

		m_BlasterHUD->m_CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
}
