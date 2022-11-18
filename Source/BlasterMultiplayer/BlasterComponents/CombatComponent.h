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

	// �ش� �Լ� �ȿ��� Ư�� ������ replicated �ǵ��� ����ϴ� ������ �����Ѵ�. 
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

	// �ش� ������ nullptr ���� �ƴ����� BlasterAnimInstance �� NativeUpdateAnimation �� �ǽð����� üũ�Ѵ�.
	// ���� EquipWeapon �� �� ������ Replicated  �ǵ��� �ؾ� �Ѵ�.
	UPROPERTY(ReplicatedUSing = OnRep_EquippedWeapon)
	class AWeapon* m_EquippedWeapon;

	// Aim ������ update �Ͽ� AnimInstance �� ���ؼ��� �����ϰ� Animation �� ���ϰ� �� ���̴�
	// �׸��� �׷��� ��ȭ�� ��� Client �� ������ ���̴�.
	UPROPERTY(Replicated)
	bool m_bAiming = false;

	UPROPERTY(EditAnywhere)
		float m_BaseWalkSpeed;
	UPROPERTY(EditAnywhere)
		float m_AimWalkSpeed;
	
	bool m_FireButtonPressed;

	// �ش� ���� ���, �� Client ���� Fire�ÿ� HitTarget �� ���� ����, FVector_NetQuantize �� ���� RPC ȣ���Ͽ�
	// BroadCasting �� ���̴� -> �׷��߸� ��� ��迡�� ������ HitTarget ���� ����ȭ �� �� �ֱ� �����̴�.
	// FVector m_HitTarget;

public :
};
