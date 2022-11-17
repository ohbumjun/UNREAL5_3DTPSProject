// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "LobbyGameMode.generated.h"

/**
 1) Track How Many Players Have Connected To Lobby Level
 2) Once Certain Num Of Players Have Come In, Travel To Next Level
 */
UCLASS()
class BLASTERMULTIPLAYER_API ALobbyGameMode : public AGameMode
{
	GENERATED_BODY()
	
public :
	// Game 에 Join 한 Player 에 Safe 하게 접근할 수 있는 함수
	virtual void PostLogin(APlayerController* NewPlayer) override;
};
