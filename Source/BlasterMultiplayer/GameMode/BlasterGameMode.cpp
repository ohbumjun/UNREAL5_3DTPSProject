// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameMode.h"
#include "../Character/BlasterCharacter.h"
#include "../PlayerController/BlasterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"

// Called From BlasterCharacter::ReceiveDamage (즉, Server 에서만 호출)
void ABlasterGameMode::PlayerEliminated(class ABlasterCharacter* ElimmedCharacter, 
	class ABlasterPlayerController* VictimController,
	class ABlasterPlayerController* AttackerController)
{
	if (ElimmedCharacter)
	{
		// 서버에서만 Elim 함수 호출
		// 하지만 Elim 함수가 multicast 로 동작하므로, 결과적으로 모든 Server, Client 에서 실행될 것이다.
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