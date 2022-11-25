// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterPlayerController.h"
#include "../HUD/BlasterHUD.h"
#include "../HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "../Character/BlasterCharacter.h"
#include "Net/UnrealNetwork.h" // Replicate Variable
#include "../GameMode/BlasterGameMode.h"

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	m_BlasterHUD = Cast<ABlasterHUD>(GetHUD());
}

void ABlasterPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetHUDTime();

	CheckTimeSync(DeltaTime);

	PollInit();
}

void ABlasterPlayerController::PollInit()
{
	if (m_CharacterOverlay == nullptr)
	{
		if (m_BlasterHUD && m_BlasterHUD->m_CharacterOverlay)
		{
			// Set Overlay Value
			m_CharacterOverlay = m_BlasterHUD->m_CharacterOverlay;

			// 그 다음 Health, Score, Defeat 등을 Init
			if (m_CharacterOverlay)
			{
				SetHUDHealth(m_HUDHealth, m_HUDMaxHealth);

				SetHUDScore(m_HUDScore);

				SetHUDDefeats(m_HUDDefeats);
			}
		}
	}
}

void ABlasterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterPlayerController, m_MatchState);
}


// 주기적으로 m_ClientServerDelta 를 구할 것이다.
void ABlasterPlayerController::CheckTimeSync(float DeltaTime) 
{
	TimeSyncRunningTime += DeltaTime;

	if (IsLocalController() && TimeSyncRunningTime >= m_TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());

		TimeSyncRunningTime = 0.f;
	}
}

void ABlasterPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	UE_LOG(LogTemp, Warning, TEXT("SetHUDHealth In ABlasterPlayerController"));

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
	else
	{
		m_bInitializeCharacterOverlay = true;
		m_HUDHealth = Health;
		m_HUDMaxHealth = MaxHealth;
	}
}

void ABlasterPlayerController::SetHUDScore(float Score)
{
	m_BlasterHUD = m_BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : m_BlasterHUD;

	bool bHUDValid = m_BlasterHUD &&
		m_BlasterHUD->m_CharacterOverlay &&
		m_BlasterHUD->m_CharacterOverlay->ScoreAmount;

	if (bHUDValid)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		
		m_BlasterHUD->m_CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}
	else
	{
		m_bInitializeCharacterOverlay = true;
		m_HUDScore = Score;
	}
}

void ABlasterPlayerController::SetHUDDefeats(int32 Defeats) 
{
	UE_LOG(LogTemp, Warning, TEXT("SetHUDDefeats In ABlasterPlayerController"));

	m_BlasterHUD = m_BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : m_BlasterHUD;

	bool bHUDValid = m_BlasterHUD &&
		m_BlasterHUD->m_CharacterOverlay &&
		m_BlasterHUD->m_CharacterOverlay->DefeatsAmount;

	if (bHUDValid)
	{
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);

		m_BlasterHUD->m_CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
	}
	else
	{
		m_bInitializeCharacterOverlay = true;
		m_HUDDefeats = Defeats;
	}
}

void ABlasterPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	m_BlasterHUD = m_BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : m_BlasterHUD;

	bool bHUDValid = m_BlasterHUD &&
		m_BlasterHUD->m_CharacterOverlay &&
		m_BlasterHUD->m_CharacterOverlay->WeaponAmmoAmount;

	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);

		m_BlasterHUD->m_CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
	}
}

void ABlasterPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
	m_BlasterHUD = m_BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : m_BlasterHUD;

	bool bHUDValid = m_BlasterHUD &&
		m_BlasterHUD->m_CharacterOverlay &&
		m_BlasterHUD->m_CharacterOverlay->CarriedAmmoAmount;

	if (bHUDValid)
	{
		FString CarriedAmmoAmountText = FString::Printf(TEXT("%d"), Ammo);

		m_BlasterHUD->m_CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(CarriedAmmoAmountText));
	}
}

void ABlasterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// Blaster Character 에 접근해야 한다.
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(InPawn);
	
	if (BlasterCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("OnPossess In ABlasterPlayerController"));

		SetHUDHealth(BlasterCharacter->GetHealth(), BlasterCharacter->GetMaxHealth());
	}
}

void ABlasterPlayerController::SetHUDMatchCountdown(float CountdownTime)
{
	m_BlasterHUD = m_BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : m_BlasterHUD;

	bool bHUDValid = m_BlasterHUD &&
		m_BlasterHUD->m_CharacterOverlay &&
		m_BlasterHUD->m_CharacterOverlay->MatchCountDownText;

	if (bHUDValid)
	{
		// float -> time format
		int32 Minutes  = FMath::FloorToInt(CountdownTime / 60);
		int32 Seconds = CountdownTime - Minutes * 60;
		
		FString CountDownText = FString::Printf(TEXT("%02d : %02d"), Minutes, Seconds);

		m_BlasterHUD->m_CharacterOverlay->MatchCountDownText->SetText(FText::FromString(CountDownText));
	}
}

// Called Every Frame In Tick Function
void ABlasterPlayerController::SetHUDTime()
{
	uint32 SecondsLeft = FMath::CeilToInt(m_MatchTime - GetServerTime());

	// Changed ! ==> 1 sc passed
	if (m_CountDownInt != SecondsLeft)
	{
		SetHUDMatchCountdown(m_MatchTime - GetServerTime());
	}

	m_CountDownInt = SecondsLeft;
}

// Called On Client, Executed From Server
void ABlasterPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequeset)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();

	ClientReportServerTime(TimeOfClientRequeset, ServerTimeOfReceipt);
}

// Called On Server, Executed From Client
void ABlasterPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequeset, float TimeServerReceivedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequeset;

	float CurrentServerTime = TimeServerReceivedClientRequest + RoundTripTime / 2.f;

	m_ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

float ABlasterPlayerController::GetServerTime()
{
	if (HasAuthority())
	{
		return GetWorld()->GetTimeSeconds();
	}
	return GetWorld()->GetTimeSeconds() + m_ClientServerDelta;
}

void ABlasterPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if (IsLocalController())
	{
		// pass in current Client Time
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

// 해당 함수는 BlasterGameMode 에서 호출할 것이고
// Server 에서만 실행할 것이다.
// 하지만 해당 내용을 Client 가 알아야 하기도 하다. 따라서 Replicate Variable 로 세팅할 것이다.
// 그리고 OnRep_MatchState 를 Rep Notify 로 설정할 것이다.
void ABlasterPlayerController::OnMatchStateSet(FName State)
{
	m_MatchState = State;

	if (m_MatchState == MatchState::InProgress)
	{
		m_BlasterHUD = m_BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : m_BlasterHUD;
	
		// InProgress 로 바뀌는 순간 HUD 를 화면에 띄울 것이다.
		if (m_BlasterHUD)
		{
			// m_BlasterHUD->AddCharacterOverlay();
		}
	}
}

void ABlasterPlayerController::OnRep_MatchState()
{
	if (m_MatchState == MatchState::InProgress)
	{
		m_BlasterHUD = m_BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : m_BlasterHUD;

		// InProgress 로 바뀌는 순간 HUD 를 화면에 띄울 것이다.
		if (m_BlasterHUD)
		{
			m_BlasterHUD->AddCharacterOverlay();
		}
	}
}
