// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameMode.h"
#include "../Character/BlasterCharacter.h"
#include "../PlayerController/BlasterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"

// Called From BlasterCharacter::ReceiveDamage (��, Server ������ ȣ��)
void ABlasterGameMode::PlayerEliminated(class ABlasterCharacter* ElimmedCharacter, 
	class ABlasterPlayerController* VictimController,
	class ABlasterPlayerController* AttackerController)
{
	if (ElimmedCharacter)
	{
		// ���������� Elim �Լ� ȣ��
		// ������ Elim �Լ��� multicast �� �����ϹǷ�, ��������� ��� Server, Client ���� ����� ���̴�.
		ElimmedCharacter->Elim();
	}
}

void ABlasterGameMode::RequestRespawn(class ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	if (ElimmedCharacter)
	{
		// Pawn �� ���ǵǾ� �ִ�. => Detach Character From Controller
		ElimmedCharacter->Reset();

		ElimmedCharacter->Destroy();

		// ������ PlayerController �� ����ִ� ����
	}

	if (ElimmedController)
	{
		// PlayerStart �� �ش��ϴ� World ���� ��� Actor ���� �迭
		TArray<AActor*> PlayerStarts;

		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);

		int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);

		// ������ Player Start ��ġ�� �����ϱ� 
		RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]);
	}
}