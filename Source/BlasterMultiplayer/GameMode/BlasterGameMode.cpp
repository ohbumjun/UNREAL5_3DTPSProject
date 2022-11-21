// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameMode.h"
#include "../Character/BlasterCharacter.h"
#include "../PlayerController/BlasterPlayerController.h"

// Called From BlasterCharacter::ReceiveDamage (��, Server ������ ȣ��)
void ABlasterGameMode::PlayerEliminated(class ABlasterCharacter* ElimmedCharacter, class ABlasterPlayerController* VictimController,
	class ABlasterPlayerController* AttackerController)
{
	if (ElimmedCharacter)
	{
		// ���������� Elim �Լ� ȣ��
		// ������ Elim �Լ��� multicast �� �����ϹǷ�, ��������� ��� Server, Client ���� ����� ���̴�.
		ElimmedCharacter->Elim();
	}
}