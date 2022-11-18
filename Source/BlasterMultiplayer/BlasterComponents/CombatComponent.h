// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

#define TRACE_LENGTH 80000.f

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTERMULTIPLAYER_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

	friend class ABlasterCharacter;

public:	
	UCombatComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 해당 함수 안에서 특정 변수가 replicated 되도록 등록하는 역할을 수행한다. 
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void EquipWeapon(class AWeapon* WeaponToEquip);
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	// Called From Character => Cover Both Server, Client Change //
	void SetAiming(bool bIsAiming);

	// Server RPC
	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);

	UFUNCTION()
	void OnRep_EquippedWeapon();

	void FireButtonPressed(bool bPressed);

	// Server RPC : Called From Client -> Execute From Sever -> Replicate
	UFUNCTION(Server, Reliable)
		void ServerFire(const FVector_NetQuantize& TraceHitTarget);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

	void TraceUnderCrosshairs(FHitResult& TraceHit);
private :
	class ABlasterCharacter* m_BlasterCharacter;

	// 해당 변수가 nullptr 인지 아닌지를 BlasterAnimInstance 의 NativeUpdateAnimation 가 실시간으로 체크한다.
	// 따라서 EquipWeapon 을 할 때마다 Replicated  되도록 해야 한다.
	UPROPERTY(ReplicatedUSing = OnRep_EquippedWeapon)
	class AWeapon* m_EquippedWeapon;

	// Aim 변수를 update 하여 AnimInstance 를 통해서도 적절하게 Animation 이 변하게 할 것이다
	// 그리고 그러한 변화를 모든 Client 에 적용할 것이다.
	UPROPERTY(Replicated)
	bool m_bAiming = false;

	UPROPERTY(EditAnywhere)
		float m_BaseWalkSpeed;
	UPROPERTY(EditAnywhere)
		float m_AimWalkSpeed;
	
	bool m_FireButtonPressed;

	// 해당 변수 대신, 각 Client 에서 Fire시에 HitTarget 을 구한 다음, FVector_NetQuantize 를 통해 RPC 호출하여
	// BroadCasting 할 것이다 -> 그래야만 모든 기계에서 동일한 HitTarget 으로 동기화 될 수 있기 때문이다.
	// FVector m_HitTarget;

public :
};
