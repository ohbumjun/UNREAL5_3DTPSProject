// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameMode.h"
#include "../Character/BlasterCharacter.h"
#include "../PlayerController/BlasterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "../PlayerState/BlasterPlayerState.h"

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