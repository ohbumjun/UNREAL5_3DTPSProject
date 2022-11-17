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

	// 해당 함수 안에서 특정 변수가 replicated 되도록 등록하는 역할을 수행한다.
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
	virtual void Jump() override; // Character 클래스 kooverride
	void FireButtonPressed();
	void FireButtonReleased();
	
private:	
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* m_CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* m_FollowCamera;

	// private  한 변수를 BlueprintReadOnly 하게 하려면 해당 meta 정보가 필요하다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* m_OverheadWidget;

	// Replicate 되도록 하고 싶다
	// 즉, Server 에서 변화가 생기면, 모든 Client 측에서도 변화가 생기게 하고 싶다
	// 뿐만 아니라 특정 function (GetLifetimeReplicatedProps) 을 override 하여 replicate 변수로 등록해야 한다.
	// (참고 : Replication 은 변화가 생길 때만 진행된다)
	// UPROPERTY(Replicated)
	// 해당 변수가 특정 Client 에게 Overlap 될 때, 해당 Client 에서 OnRep_OverlappingWeapon 함수가 호출되게 할 것이다.
	// 주의할점 : 이 경우, Server 에서 Control 하는 Pawn 에게는 보이지 않는다. 왜냐하면 해당 함수는 서버에서 호출되지 않기 때문이다
	// 리플리케이션은 서버 -> 클라리언트 방향으로만 진행되는 것이기 때문이다.
	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon) 
	class AWeapon* m_OverlappingWeapon;

	// 변수가 Replicate 될 때만 호출되는 함수 => 인자 ? AWeapon* LastWeapon : Replicate 되기 이전의 변수 상태가 넘어갈 것이다
	// ex) 해당 변수를 nullptr 로 세팅 -> 리플리케이션 -> OnRep_OverlappingWeapon(이전 변수 상태 --> nullptr X)
	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	UPROPERTY(VisibleAnywhere)
	class UCombatComponent* m_CombatComponent;

	// RPC 
	// Reliable RPC : Guaranteed To be executed -> client wll receive confirmation that server received rpc , 만약 confirmation 안받으면 다시 전달
	// - 하지만 상대적으로 느리기 때문에 자주 x => ex. Tick Function 등에서는 되도록 x => 하지만 Equip Weapon 기능은 okay
	// Unreliable RPC : Can Be Dropped  => 기본적으로 RPC 는 Unreliable
	// As We Send Information From Client To Server, Info Be Sent In Packet
	// PAcket is unit of info sent across network, and can be dropped
	UFUNCTION(Server, Reliable)
	void ServerEquippedButtonPressed();

	float m_AO_Yaw;
	float m_InterpAO_Yaw; // Turn 하고 나서 다시 각도를 0으로 맞춰주기 위함
	float m_AO_Pitch;
	FRotator m_StartingAimRotation;

	ETurningInPlace m_TurningInPlace;
	void TurnInPlace(float DeltaTime);
	
	// Blueprint 에서 세팅하게 할 것 -> Fire 할 때의 Animation Montage
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
