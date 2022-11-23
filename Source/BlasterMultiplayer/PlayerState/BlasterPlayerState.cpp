// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterPlayerState.h"
#include "../Character/BlasterCharacter.h"
#include "Net/UnrealNetwork.h" 
#include "../PlayerController/BlasterPlayerController.h"

// Only Called From Server
void ABlasterPlayerState::AddToScore(float ScoreAmount)
{
	SetScore(GetScore() + ScoreAmount);

	m_Character = m_Character == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : m_Character;

	if (m_Character)
	{
		m_Controller = m_Controller == nullptr ? Cast<ABlasterPlayerController>(m_Character->Controller) : m_Controller;

		if (m_Controller)
		{
			m_Controller->SetHUDScore(GetScore());
		}
	}
}

void ABlasterPlayerState::AddToDefeats(int32 DefeatsAmount)
{
	// 해당 줄이 실행되자마자 OnRep_Defeats 가 실행될 것이다 (Replicated 변수로 등록)
	m_Defeats += DefeatsAmount;

	m_Character = m_Character == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : m_Character;

	if (m_Character)
	{
		m_Controller = m_Controller == nullptr ? 
			Cast<ABlasterPlayerController>(m_Character->Controller) : m_Controller;

		if (m_Controller)
		{
			m_Controller->SetHUDDefeats(m_Defeats);
		}
	}
}

void ABlasterPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterPlayerState, m_Defeats);
}

// Replicated -> To All Client ( Not Called On Server)
void ABlasterPlayerState::OnRep_Score()
{
	Super::OnRep_Score();

	m_Character = m_Character == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : m_Character;

	if (m_Character)
	{
		m_Controller = m_Controller == nullptr ? 
			Cast<ABlasterPlayerController>(m_Character->Controller) : m_Controller;

		if (m_Controller)
		{
			m_Controller->SetHUDScore(GetScore());
		}
	}
};

void ABlasterPlayerState::OnRep_Defeats()
{
	m_Character = m_Character == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : m_Character;

	if (m_Character)
	{
		m_Controller = m_Controller == nullptr ? 
			Cast<ABlasterPlayerController>(m_Character->Controller) : m_Controller;

		if (m_Controller)
		{
			m_Controller->SetHUDDefeats(m_Defeats);
		}
	}
}


