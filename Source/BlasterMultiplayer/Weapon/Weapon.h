// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WeaponTypes.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"


// Blueprint ���� ����� �� �ְ� ���ֱ�
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

	// CombatComponent -> ���⸦ Equip �ϴ� ���� Weapon Actor �� ���õ� ������ �Ϻε� Replicate �ǵ��� �ϰ� �ʹ�.
	// GetLifetimeReplicatedProps() ���� Replicate �ǵ��� �ϴ� ������ ������ ���̴�.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Owner �� ���� Rep Notify �Լ�
	virtual void OnRep_Owner() override;

	void SetHUDAmmo();

	void ShowPickupWidget(bool bShowWidget);

	virtual void Fire(const FVector& HitTarget);

	// Drop Weapon When Character Dies 
	void Dropped();

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
	- ���⸶�� �Ʒ��� ���� �ٸ��� ���ֱ� ���ؼ� Blueprint �󿡼� Edit �� �� �ְ� �Ѵ�.
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

	// Component Begin Overlap �̶�� Delegate �� Bind �� ���̴�. ���� Delegate �� ���� ���̴�. 
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

	// EWeaponState �� Replicate �ɶ� ȣ���� �Լ�
	UFUNCTION()
	void OnRep_WeaponState();

	// WBP_PickupWidget
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
		class UWidgetComponent* m_PickupWidget;

	// ex. Fire �� �ش��ϴ� Asset ���� �ִ� Animation �� �ڵ� �󿡼� ����ϰ� �ʹ�
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	class UAnimationAsset* m_FireAnimation;

	// ����� Bullet Class ����
	UPROPERTY(EditAnywhere)
	TSubclassOf<class ACasing> m_CasingClass;

	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Ammo)
	int32 m_Ammo = 30;

	UFUNCTION()
	void OnRep_Ammo();

	void SpendRound();

	UPROPERTY(EditAnywhere)
	int32 m_MagCapacity = 30;

	// �˾Ƽ� nullptr �� �������ֱ� ���ؼ� UPROPERTY() �� �ٿ��� ���̴�.
	UPROPERTY()
	class ABlasterCharacter* m_BlasterOwnerCharacter;
	UPROPERTY()
	class ABlasterPlayerController* m_BlasterOwnerController;

	UPROPERTY(EditAnywhere)
	EWeaponType m_WeaponType;
public :
	// FORCEINLINE == inline Ű����
	void SetWeaponState(EWeaponState State);
	FORCEINLINE USphereComponent* GetAreaSphere() const { return m_AreaSphere; }
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return m_WeaponMesh; }
	FORCEINLINE float GetZoomedFOV() const { return m_ZoomedFOV; }
	FORCEINLINE float GetZoomedInterpSpeed() const { return m_ZoomInterpSpeed; }
	bool IsEmpty();
	FORCEINLINE EWeaponType GetWeaponType() const { return m_WeaponType; }
};
