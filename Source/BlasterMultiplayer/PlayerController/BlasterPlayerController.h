// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BlasterPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class BLASTERMULTIPLAYER_API ABlasterPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public :
	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDScore(float Score);
	void SetHUDDefeats(int32 Defeats);
	void SetHUDWeaponAmmo(int32 Ammo);
	void SetHUDCarriedAmmo(int32 Ammo);
	void SetHUDMatchCountdown(float CountdownTime);
	virtual void OnPossess(APawn* InPawn) override;
	void OnMatchStateSet(FName State);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected :
	virtual void BeginPlay() override;
	void SetHUDTime();
	void PollInit();
	virtual void Tick(float DeltaTime) override;

	// Synced with server World Clock
	virtual float GetServerTime();

	// Server �κ��� Time �� ���� �� �ִ� ���� ���� �κ�
	// Called after this PlayerController's viewport/net connection is associated with this player controller.
	// ��, PlayerController �� �� ������ �� �Ŀ� ȣ���Ѵ�.
	// ���� �ش� �Լ��� Client �� �ſ� �ʹݿ� ȣ��� ���̴�.
	// ���� : Sync with server clock as soon as possible
	// - �ֱ������� �ش� �Լ��� ȣ���� ���̴�. ��Ʈ��ũ ��Ȳ�� ���� ClientServerDelta �� �޶��� �� �ֱ� �����̴�.
	virtual void ReceivedPlayer() override;
	/*
	* Sync time between client and server
	*/
	// Client�� Server ���� ������ RPC
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequeset);

	// ServerRequestServerTime �Լ��� ���� �������� Server �� Client ���� �ٽ� ������ �ð�
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequeset, float TimeServerReceivedClientRequest);

	// Diff Between Client and Server Time
	float m_ClientServerDelta = 0.f;

	// �� 5�� ���� m_ClientServerDelta �� �ٽ� ���� ���̴�.
	UPROPERTY(EditAnywhere, Category = Time)
	float m_TimeSyncFrequency = 5.f;

	float TimeSyncRunningTime = 0.f;

	void CheckTimeSync(float DeltaTime);

private :
	UPROPERTY()
	class ABlasterHUD* m_BlasterHUD;

	float m_MatchTime = 120.f;
	uint32 m_CountDownInt = 0.f;

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName m_MatchState;

	UFUNCTION()
	void OnRep_MatchState();

	UPROPERTY()
	class UCharacterOverlay* m_CharacterOverlay;

	bool m_bInitializeCharacterOverlay = false;

	float m_HUDHealth;
	float m_HUDMaxHealth;
	float m_HUDScore;
	int32 m_HUDDefeats;
};
