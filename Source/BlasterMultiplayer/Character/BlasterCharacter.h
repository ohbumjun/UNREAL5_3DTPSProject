// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "../BlasterTypes/TurningInPlace.h"
#include "BlasterCharacter.generated.h"


UCLASS()
class BLASTERMULTIPLAYER_API ABlasterCharacter : public ACharacter
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
	virtual void Jump() override; // Character Ŭ���� kooverride
	void FireButtonPressed();
	void FireButtonReleased();
	
private:	
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* m_CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
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
	// Reliable RPC : Guaranteed To be executed -> client wll receive confirmation that server received rpc , ���� confirmation �ȹ����� �ٽ� ����
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
	UPROPERTY(EditAnywhere, Category = Combate)
	class UAnimMontage* m_FireWeaponMontage;

public :
	void SetOverlappingWeapon(AWeapon* Weapon);
	bool IsWeaponEquipped();
	bool IsAiming();

	FORCEINLINE float GetAO_Yaw() const { return m_AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return m_AO_Pitch; }
	class AWeapon* GetEquippedWeapon();

	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return m_TurningInPlace; }
};
