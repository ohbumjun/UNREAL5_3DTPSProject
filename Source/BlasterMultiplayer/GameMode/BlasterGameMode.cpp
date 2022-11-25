// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameMode.h"
#include "../Character/BlasterCharacter.h"
#include "../PlayerController/BlasterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "../PlayerState/BlasterPlayerState.h"

ABlasterGameMode::ABlasterGameMode()
{
	// InProgress 이전에, WaitingToStart 단계에 머무르게 된다.
	// 별도로 StartMatch() 함수를 호출해야만 다음 단계로 넘어갈 수 있게 된다.
	// 단, DefaultPawn은 생성해주고, Mesh 는 생성해주지 않아서, 이리저리 움직여 다닐 수는 있다.
	bDelayedStart = true;
}

void ABlasterGameMode::BeginPlay() 
{
	Super::BeginPlay();
	
	// ABlasterGameMode 는 BlasterMap 에서만 사용된다.
	// 따라서 해당 Map에 입장했을 때의 시간을 계산하여 사용할 것이다.
	m_LevelStartingTime = GetWorld()->GetTimeSeconds();
}


void ABlasterGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// GetWorld()->GetTimeSeconds() : 게임 Launch 하자마자 세기 시작한다. 
	// (시작 버튼 누를때부터 시작, BlasterMap 입장할 때는 이미 0이 아님)
	// 따라서 m_LevelStartingTime 를 더해서, Map 에 입장한 시점부처 Countdown 을 계산할 것이다.
	if (MatchState == MatchState::WaitingToStart)
	{
		m_CountdownTime = m_WarmUpTime - GetWorld()->GetTimeSeconds() + m_LevelStartingTime;

		if (m_CountdownTime < 0.f)
		{
			// Spawn All Character, Controller Character
			StartMatch();
		}
	}
}

// 즉, MatchState 가 바뀔 때마다 아래 함수를 실행 -> 모든 PlayerController 에게 MatchState 정보를 알려줄 것이다.
// 단, 서버 측에서만 실행하게 되는 것이다.
void ABlasterGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	// 모든 PlayerController 에게 MatchState 가 무엇인지를 알려줄 것이다.
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ABlasterPlayerController* BlasterPlayer = Cast<ABlasterPlayerController>(*It);

		if (BlasterPlayer)
		{
			BlasterPlayer->OnMatchStateSet(MatchState);
		}
	}
}

// Called From BlasterCharacter::ReceiveDamage (즉, Server 에서만 호출)
void ABlasterGameMode::PlayerEliminated(class ABlasterCharacter* ElimmedCharacter, 
	class ABlasterPlayerController* VictimController,
	class ABlasterPlayerController* AttackerController)
{
	ABlasterPlayerState* AttackerPlayerState = AttackerController ?
		Cast<ABlasterPlayerState>(AttackerController->PlayerState) :
		nullptr;

	ABlasterPlayerState* VictimPlayerState = VictimController ?
		Cast<ABlasterPlayerState>(VictimController->PlayerState) :
		nullptr;

	// Attacker, Victim 모두의 PlayerState 에 접근하여 Score 를 Update 해줄 것이다.
	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState)
	{
		AttackerPlayerState->AddToScore(1.f);
	}

	if (VictimPlayerState && VictimPlayerState != AttackerPlayerState)
	{
		VictimPlayerState->AddToDefeats(1);
	}

	if (ElimmedCharacter)
	{
		// 서버에서만 Elim 함수 호출
		ElimmedCharacter->Elim();

		
	}
}

void ABlasterGameMode::RequestRespawn(class ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	if (ElimmedCharacter)
	{
		// Pawn 에 정의되어 있다. => Detach Character From Controller
		ElimmedCharacter->Reset();

		ElimmedCharacter->Destroy();

		// 하지만 PlayerController 는 살아있는 상태
	}

	if (ElimmedController)
	{
		// PlayerStart 에 해당하는 World 상의 모든 Actor 들의 배열
		TArray<AActor*> PlayerStarts;

		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);

		int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);

		// 랜덤한 Player Start 위치에 생성하기 
		RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]);
	}
}