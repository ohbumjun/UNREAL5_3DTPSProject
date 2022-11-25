// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../HUD/BlasterHUD.h"
#include "../Weapon/WeaponTypes.h"
#include "../BlasterTypes/CombatState.h"
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

	void Reload();

	UFUNCTION(BlueprintCallable)
	void FinishReloading();
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

	// TMap �� Replicate �� �� ����.
	// �ֳ��ϸ� TMap �� Hash �˰����� ���µ�, �ؽ� �Լ��� ������
	// Ŭ���̾�Ʈ���� ���� ������� ���������� �ʱ� �����̴�.
	// Ư�� Weapon �� ���� Ammo�� �����Ѵٰ� �Ͽ� ��ü TMap �� Replicate ���� ���� ���̴�.
	// ���� ����ϴ� Weapon �� ���� m_CarriedAmmo ���� Replicate ������ ���̴�.
	TMap<EWeaponType, int32> m_CarriedAmmoMap;

	void InitializeCarriedAmmo();
	
	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	ECombatState m_CombatState = ECombatState::ECS_Unoccupied;

	UFUNCTION()
	void OnRep_CombatState();
	
	UPROPERTY(EditAnywhere)
	int32 m_StartingARAmmo = 30;

	void FireButtonPressed(bool bPressed);

	// Server RPC : Called From Client -> Execute From Sever -> Replicate
	UFUNCTION(Server, Reliable)
		void ServerFire(const FVector_NetQuantize& TraceHitTarget);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

	void TraceUnderCrosshairs(FHitResult& TraceHit);

	// Call Tick Component
	void SetHUDCrosshairs(float DeltaTime);

	UFUNCTION(Server, Reliable)
		void ServerReload();

	// ������ Client ��� ��ο��� �߻��ϴ� ���� Dealing �ϱ� 
	void HandleReload();

	// ���� ������ �ִ� m_CarriedAmmo �� �̿��ؼ� Ammo ä��� (źâ ä���)
	int32 AmountToReload();
private :
	UPROPERTY()
	class ABlasterCharacter* m_BlasterCharacter;
	UPROPERTY()
	class ABlasterPlayerController* m_Controller;
	UPROPERTY()
	class ABlasterHUD* m_HUD;

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

	/*
	HUD and Crosshairs
	*/
	float m_CrosshairVelocityFactor;
	float m_CrosshairInAirFactor;
	float m_CrosshairShootingFactor;
	float m_CrosshairAimFactor;

	// BlasterAnimInstance ���� ���� �ѱ���, ���� ������ �������� Rotate ��Ű�µ� ���ǰ� �ִ�.
	FVector m_HitTarget;

	FHUDPackage m_HUDPackage;

	/*
	* Aiming and FOV
	*/
	// FOV When Not Aiming; Set to the camera's base fov in BeginPlay
	float m_DefaultFOV;
	float m_CurrentFOV;

	UPROPERTY(EditAnywhere, Category = Combat)
	float m_ZoomedFOV = 30.f;

	UPROPERTY(EditAnywhere, Category = Combat)
	float m_ZoomInterpSpeed = 20.f;

	void InterpFOV(float DeltaTime);

	/*
	* Automatic Fire
	*/
	FTimerHandle m_FireTimer;

	bool m_bCanFire = true;
	
	void StartFireTimer();
	void FireTimerFinished();
	void Fire();

	bool CanFire();
	
	// Carried Ammo for currently-equipped weapon
	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
	int32 m_CarriedAmmo;

	UFUNCTION()
	void OnRep_CarriedAmmo();
public :
};
