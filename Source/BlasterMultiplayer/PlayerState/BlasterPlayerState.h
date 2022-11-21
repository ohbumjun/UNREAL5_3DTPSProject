// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "BlasterPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class BLASTERMULTIPLAYER_API ABlasterPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public :
	// 해당 함수 안에서 특정 변수가 replicated 되도록 등록하는 역할을 수행한다.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// PlayerState 의 Score 는 Replicate 되고, OnRep_Score 를 Rap Notify 로 실행한다.
	// Update HUD
	virtual void OnRep_Score() override;

	// Rap_Notify 를 해주기 위해서는 UFUNCTION 매크로를 붙여줘야한다.
	// OnRep_Score 에 붙여주지 않은 이유는, 우리는 현재 override 한 것이고
	// 부모 클래스에서 이미 명시해주었기 때문이다.
	UFUNCTION()
	virtual void OnRep_Defeats();

	void AddToScore(float ScoreAmount);
	void AddToDefeats(int32 DefeatsAmount);

private :
	UPROPERTY()
	class ABlasterCharacter* m_Character;
	UPROPERTY()
	class ABlasterPlayerController* m_Controller;

	UPROPERTY(ReplicatedUsing = OnRep_Defeats)
	int32 m_Defeats;

};
