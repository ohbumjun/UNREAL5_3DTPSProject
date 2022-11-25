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

	// Server 로부터 Time 을 받을 수 있는 가장 빠른 부분
	// Called after this PlayerController's viewport/net connection is associated with this player controller.
	// 즉, PlayerController 의 넷 연결이 된 후에 호출한다.
	// 따라서 해당 함수는 Client 의 매우 초반에 호출될 것이다.
	// 역할 : Sync with server clock as soon as possible
	// - 주기적으로 해당 함수를 호출할 것이다. 네트워크 상황에 따라 ClientServerDelta 가 달라질 수 있기 때문이다.
	virtual void ReceivedPlayer() override;
	/*
	* Sync time between client and server
	*/
	// Client가 Server 에게 보내는 RPC
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequeset);

	// ServerRequestServerTime 함수에 대한 응답으로 Server 가 Client 에게 다시 보내는 시간
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequeset, float TimeServerReceivedClientRequest);

	// Diff Between Client and Server Time
	float m_ClientServerDelta = 0.f;

	// 매 5초 마다 m_ClientServerDelta 를 다시 구할 것이다.
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
