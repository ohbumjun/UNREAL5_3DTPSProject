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
	// �ش� �Լ� �ȿ��� Ư�� ������ replicated �ǵ��� ����ϴ� ������ �����Ѵ�.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// PlayerState �� Score �� Replicate �ǰ�, OnRep_Score �� Rap Notify �� �����Ѵ�.
	// Update HUD
	virtual void OnRep_Score() override;

	// Rap_Notify �� ���ֱ� ���ؼ��� UFUNCTION ��ũ�θ� �ٿ�����Ѵ�.
	// OnRep_Score �� �ٿ����� ���� ������, �츮�� ���� override �� ���̰�
	// �θ� Ŭ�������� �̹� ������־��� �����̴�.
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
