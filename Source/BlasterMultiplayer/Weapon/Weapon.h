// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"


// Blueprint 에서 사용할 수 있게 해주기
UENUM(BlueprintType) 
enum class EWeaponState : uint8
{
	EWS_Initial UMETA(DisplayName = "Initial State"), // DisplayName In Blueprint
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),
	EWS_MAX UMETA(DisplayName = "DefaultMax")
};

UCLASS()
class BLASTERMULTIPLAYER_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeapon();

public:	
	virtual void Tick(float DeltaTime) override;

	// CombatComponent -> 무기를 Equip 하는 순간 Weapon Actor 와 관련된 변수들 일부도 Replicate 되도록 하고 싶다.
	// GetLifetimeReplicatedProps() 에서 Replicate 되도록 하는 변수를 세팅할 것이다.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void ShowPickupWidget(bool bShowWidget);

	virtual void Fire(const FVector& HitTarget);

	/*
	* Textures for weapon crosshairs
	*/
	UPROPERTY(EditAnywhere, Category = Crosshairs)
		class UTexture2D* m_CrosshairCenter;
	UPROPERTY(EditAnywhere, Category = Crosshairs)
		class UTexture2D* m_CrosshairLeft;
	UPROPERTY(EditAnywhere, Category = Crosshairs)
		class UTexture2D* m_CrosshairRight;
	UPROPERTY(EditAnywhere, Category = Crosshairs)
		class UTexture2D* m_CrosshairTop;
	UPROPERTY(EditAnywhere, Category = Crosshairs)
		class UTexture2D* m_CrosshairBottom;

	/*
	Zoomed FOV While Aiming => For Zoom While Aiming
	- 무기마다 아래의 값을 다르게 해주기 위해서 Blueprint 상에서 Edit 할 수 있게 한다.
	*/
	UPROPERTY(EditAnywhere)
		float m_ZoomedFOV = 30.f;
	UPROPERTY(EditAnywhere)
		float m_ZoomInterpSpeed = 20.f;
	UPROPERTY(EditAnywhere, Category = Combat)
		float m_FireDelay = 0.15f;
	UPROPERTY(EditAnywhere, Category = Combat)
		bool m_bAutomatic = true;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Component Begin Overlap 이라는 Delegate 에 Bind 할 것이다. 따라서 Delegate 를 붙일 것이다. 
	// OverLapEvent -> Show Pick Up Widget If Collide With Character
	UFUNCTION()
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	virtual void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);
private :
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class USkeletalMeshComponent* m_WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
		class USphereComponent* m_AreaSphere;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere)
		EWeaponState m_WeaponState;

	// EWeaponState 가 Replicate 될때 호출할 함수
	UFUNCTION()
	void OnRep_WeaponState();

	// WBP_PickupWidget
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
		class UWidgetComponent* m_PickupWidget;

	// ex. Fire 에 해당하는 Asset 내에 있는 Animation 을 코드 상에서 재생하고 싶다
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	class UAnimationAsset* m_FireAnimation;


	// 사용할 Bullet Class 장착
	UPROPERTY(EditAnywhere)
	TSubclassOf<class ACasing> m_CasingClass;


public :
	// FORCEINLINE == inline 키워드
	void SetWeaponState(EWeaponState State);
	FORCEINLINE USphereComponent* GetAreaSphere() const { return m_AreaSphere; }
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return m_WeaponMesh; }
	FORCEINLINE float GetZoomedFOV() const { return m_ZoomedFOV; }
	FORCEINLINE float GetZoomedInterpSpeed() const { return m_ZoomInterpSpeed; }
};
