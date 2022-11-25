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

	// 해당 함수 안에서 특정 변수가 replicated 되도록 등록하는 역할을 수행한다. 
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

	// TMap 은 Replicate 될 수 없다.
	// 왜냐하면 TMap 은 Hash 알고리즘을 쓰는데, 해시 함수가 서버와
	// 클라이언트에서 같은 결과값을 리턴해주지 않기 때문이다.
	// 특정 Weapon 에 대한 Ammo가 감소한다고 하여 전체 TMap 을 Replicate 하지 않을 것이다.
	// 현재 사용하는 Weapon 에 대한 m_CarriedAmmo 만을 Replicate 시켜줄 것이다.
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

	// 서버와 Client 기계 모두에서 발생하는 일을 Dealing 하기 
	void HandleReload();

	// 내가 가지고 있는 m_CarriedAmmo 를 이용해서 Ammo 채우기 (탄창 채우기)
	int32 AmountToReload();
private :
	UPROPERTY()
	class ABlasterCharacter* m_BlasterCharacter;
	UPROPERTY()
	class ABlasterPlayerController* m_Controller;
	UPROPERTY()
	class ABlasterHUD* m_HUD;

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

	/*
	HUD and Crosshairs
	*/
	float m_CrosshairVelocityFactor;
	float m_CrosshairInAirFactor;
	float m_CrosshairShootingFactor;
	float m_CrosshairAimFactor;

	// BlasterAnimInstance 에서 현재 총구를, 총을 쏴야할 방향으로 Rotate 시키는데 사용되고 있다.
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
