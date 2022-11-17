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
	// Game �� Join �� Player �� Safe �ϰ� ������ �� �ִ� �Լ�
	virtual void PostLogin(APlayerController* NewPlayer) override;
};
