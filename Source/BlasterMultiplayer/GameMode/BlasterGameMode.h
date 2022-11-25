// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BlasterGameMode.generated.h"

/**
 * 
 */
UCLASS()
class BLASTERMULTIPLAYER_API ABlasterGameMode : public AGameMode
{
	GENERATED_BODY()
	
public :
	ABlasterGameMode();

	virtual void Tick(float DeltaTime) override;

	virtual void PlayerEliminated(class ABlasterCharacter* ElimmedCharacter, class ABlasterPlayerController* VictimController,
		class ABlasterPlayerController* AttackerController);

	// What Character To Respawn
	virtual void RequestRespawn(class ACharacter* ElimmedCharacter, AController* ElimmedController);

	UPROPERTY(EditDefaultsOnly)
	float m_WarmUpTime = 10.f;

	float m_LevelStartingTime = 0.f;

protected :
	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;

private :
	float m_CountdownTime = 0.f;
};
