// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameMode.h"
#include "../Character/BlasterCharacter.h"
#include "../PlayerController/BlasterPlayerController.h"

// Called From BlasterCharacter::ReceiveDamage (즉, Server 에서만 호출)
void ABlasterGameMode::PlayerEliminated(class ABlasterCharacter* ElimmedCharacter, class ABlasterPlayerController* VictimController,
	class ABlasterPlayerController* AttackerController)
{
	if (ElimmedCharacter)
	{
		// 서버에서만 Elim 함수 호출
		// 하지만 Elim 함수가 multicast 로 동작하므로, 결과적으로 모든 Server, Client 에서 실행될 것이다.
		ElimmedCharacter->Elim();
	}
}