// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "../BlasterTypes/TurningInPlace.h"
#include "../Interfaces/InteractWithCrosshairsInterface.h"
#include "Components/TimelineComponent.h"
#include "BlasterCharacter.generated.h"


UCLASS()
class BLASTERMULTIPLAYER_API ABlasterCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ABlasterCharacter();

	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// �ش� �Լ� �ȿ��� Ư�� ������ replicated �ǵ��� ����ϴ� ������ �����Ѵ�.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Access to Combat Component
	virtual void PostInitializeComponents() override;

	// Called By Combat Component
	void PlayFireMontage(bool bAiming);

	void PlayElimMontage();

	// Not Very Imporant -> Set As "Unreliable"
	// => RPC �� ����ϴ� ��� ReceiveDamage ��� �Լ� �ȿ��� Replicate �� ���� ó�� (Replication �� �� ��� ����)
	// UFUNCTION(NetMulticast, Unreliable)
	// 	void MultiCastHit();

	virtual void OnRep_ReplicatedMovement() override;

	// Called When Player Gets Elimmed
	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim();

	// Calling Only On Server
	void Elim();

	virtual void Destroyed() override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void MoveForward(float Value);
	void MoveRight(float Value);

	void Turn(float Value);
	void LookUp(float Value); 

	void EquipButtonPressed();
	void CrouchButtonPressed();
	void AimButtonPressed();
	void AimButtonReleased();
	void AimOffset(float DeltaTime);
	void SimProxiesTurn();
	virtual void Jump() override; // Character Ŭ���� kooverride
	void FireButtonPressed();
	void FireButtonReleased();

	// Called By AProjectile When Hit
	void PlayHitReactMontage();
	// Callback To Damage Event (Damage Triggerd By ex. ProjectBullet Class)
	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
		class AController* InstigatorController, AActor* DamageCursor);

	void UpdateHUDHealth();

	// Poll for any relevant classes and initizlie out HUD
	void PollInit();
private:	
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	class USpringArmComponent* m_CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	class UCameraComponent* m_FollowCamera;

	// private  �� ������ BlueprintReadOnly �ϰ� �Ϸ��� �ش� meta ������ �ʿ��ϴ�
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* m_OverheadWidget;

	// Replicate �ǵ��� �ϰ� �ʹ�
	// ��, Server ���� ��ȭ�� �����, ��� Client �������� ��ȭ�� ����� �ϰ� �ʹ�
	// �Ӹ� �ƴ϶� Ư�� function (GetLifetimeReplicatedProps) �� override �Ͽ� replicate ������ ����ؾ� �Ѵ�.
	// (���� : Replication �� ��ȭ�� ���� ���� ����ȴ�)
	// UPROPERTY(Replicated)
	// �ش� ������ Ư�� Client ���� Overlap �� ��, �ش� Client ���� OnRep_OverlappingWeapon �Լ��� ȣ��ǰ� �� ���̴�.
	// �������� : �� ���, Server ���� Control �ϴ� Pawn ���Դ� ������ �ʴ´�. �ֳ��ϸ� �ش� �Լ��� �������� ȣ����� �ʱ� �����̴�
	// ���ø����̼��� ���� -> Ŭ�󸮾�Ʈ �������θ� ����Ǵ� ���̱� �����̴�.
	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon) 
	class AWeapon* m_OverlappingWeapon;

	// ������ Replicate �� ���� ȣ��Ǵ� �Լ� => ���� ? AWeapon* LastWeapon : Replicate �Ǳ� ������ ���� ���°� �Ѿ ���̴�
	// ex) �ش� ������ nullptr �� ���� -> ���ø����̼� -> OnRep_OverlappingWeapon(���� ���� ���� --> nullptr X)
	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	UPROPERTY(VisibleAnywhere)
	class UCombatComponent* m_CombatComponent;

	// RPC 
	// Reliable RPC : Guaranteed To be executed -> client wll receive confirmation that server received rpc , 
	// ���� confirmation �ȹ����� �ٽ� ����
	// - ������ ��������� ������ ������ ���� x => ex. Tick Function ����� �ǵ��� x => ������ Equip Weapon ����� okay
	// Unreliable RPC : Can Be Dropped  => �⺻������ RPC �� Unreliable
	// As We Send Information From Client To Server, Info Be Sent In Packet
	// PAcket is unit of info sent across network, and can be dropped
	UFUNCTION(Server, Reliable)
	void ServerEquippedButtonPressed();

	float m_AO_Yaw;
	float m_InterpAO_Yaw; // Turn �ϰ� ���� �ٽ� ������ 0���� �����ֱ� ����
	float m_AO_Pitch;
	FRotator m_StartingAimRotation;

	ETurningInPlace m_TurningInPlace;
	void TurnInPlace(float DeltaTime);
	
	// Blueprint ���� �����ϰ� �� �� -> Fire �� ���� Animation Montage
	UPROPERTY(EditAnywhere, Category = "Combat")
	class UAnimMontage* m_FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	class UAnimMontage* m_HitReactMontage;

	UPROPERTY(EditAnywhere, Category = "Combat") 
	class UAnimMontage* m_ElimMontage;

	void HideCameraIfCharacterClose();

	UPROPERTY(EditAnywhere)
	float m_CameraThreadhold = 200.f;

	bool m_bRotateRootBone;

	float m_TurnThreshold = 8.f;

	FRotator m_ProxyRotationLastFrame;
	FRotator m_ProxyRotation;
	float m_ProxyYaw;
	float m_TimeSinceLastMovementReplication;

	void CalculateAO_Pitch();

	float CalculateSpeed();

	/*
	* Player Health
	*/
	UPROPERTY(EditAnywhere, Category = "PlayerStats")
	float m_MaxHealth = 100.f;

	// Replicate this variable
	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "PlayerStats")
	float m_Health = 100.f;

	UFUNCTION()
	void OnRep_Health();

	UPROPERTY()
	class ABlasterPlayerController* m_BlasterPlayerController;

	bool m_bElimmed = false;

	FTimerHandle m_ElimTimer;

	// ĳ���͸��� ���� �ٸ� ElimDelay �� �������� �ȵȴ�.
	UPROPERTY(EditDefaultsOnly)
	float m_ElimDelay = 3.f;

	void ElimTimerFinished();

	/*
	* Dissolve Effect
	*/
	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* m_DissolveTimeline;

	FOnTimelineFloat m_DissolveTrack;

	// Blueprint �󿡼� ��������� �Ѵ�.
	UPROPERTY(EditAnywhere)
	UCurveFloat* m_DissolveCurve;

	// Called Every Frame As We Are Updating Timeline
	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);

	void StartDissolve();

	// Save Dynamic Instance We create / change at run time 
	// ��, �Ʒ��� m_DissolveMaterialInstance �� �̿��ؼ� m_DyanamicDissolveMaterialInstance �� ��Ÿ�Ӷ� ������ ���̴�.
	UPROPERTY(VisibleAnywhere, Category = Elim)
	UMaterialInstanceDynamic* m_DyanamicDissolveMaterialInstance;

	// Actual material instance set on blueprint, used with dynamic material instance
	UPROPERTY(EditAnywhere, Category = Elim)
		UMaterialInstance* m_DissolveMaterialInstance;
	
	UPROPERTY()
	class ABlasterPlayerState* m_BlasterPlayerState;
public :
	FORCEINLINE float GetAO_Yaw() const { return m_AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return m_AO_Pitch; }
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return m_TurningInPlace; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return m_FollowCamera; }
	FORCEINLINE bool ShouldRotateRootBone() const { return m_bRotateRootBone; }
	FORCEINLINE bool IsElimmed() const { return m_bElimmed; }
	FORCEINLINE float GetHealth() const { return m_Health; }
	FORCEINLINE float GetMaxHealth() const { return m_MaxHealth; }

	void SetOverlappingWeapon(AWeapon* Weapon);
	bool IsWeaponEquipped();
	bool IsAiming();
	class AWeapon* GetEquippedWeapon();
	FVector GetHitTarget() const;
};
