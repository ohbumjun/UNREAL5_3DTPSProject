// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "../BlasterTypes/TurningInPlace.h"
#include "BlasterAnimInstance.generated.h"

UCLASS()
class BLASTERMULTIPLAYER_API UBlasterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public :
	// Much Like Begin Play -> Called At The Beginning Of The Game ...
	virtual void NativeInitializeAnimation() override;
	// Called Every Time => Like Tick() 
	virtual void NativeUpdateAnimation(float DeltaTime) override;

private :
	UPROPERTY(BlueprintReadOnly, Category = Character, meta = (AllowPrivateAccess = "true"))
		class ABlasterCharacter* m_BlasterCharacter;
	UPROPERTY(BlueprintReadOnly, Category = Character, meta = (AllowPrivateAccess = "true"))
		float Speed;
	UPROPERTY(BlueprintReadOnly, Category = Character, meta = (AllowPrivateAccess = "true"))
		bool bIsInAir;
	// 가속도 X => Rather Pressed Key To Move
	UPROPERTY(BlueprintReadOnly, Category = Character, meta = (AllowPrivateAccess = "true"))
		bool IsAccelerating;
	UPROPERTY(BlueprintReadOnly, Category = Character, meta = (AllowPrivateAccess = "true"))
		bool bWeaponEquipped;

	class AWeapon* EquippedWeapon;

	UPROPERTY(BlueprintReadOnly, Category = Character, meta = (AllowPrivateAccess = "true"))
		bool bIsCrouched;
	UPROPERTY(BlueprintReadOnly, Category = Character, meta = (AllowPrivateAccess = "true"))
		bool bIsAiming;

	// Driving Running Blend Space ex) EquippedRun
	UPROPERTY(BlueprintReadOnly, Category = Character, meta = (AllowPrivateAccess = "true"))
	float YawOffset;

	UPROPERTY(BlueprintReadOnly, Category = Character, meta = (AllowPrivateAccess = "true"))
	float Lean;

	// Rotator For Last Frame
	FRotator CharacterRotationLastFrame;
	FRotator CharacterRotation;
	FRotator DeltaRotation;

	UPROPERTY(BlueprintReadOnly, Category = Character, meta = (AllowPrivateAccess = "true"))
		float AO_Yaw;
	UPROPERTY(BlueprintReadOnly, Category = Character, meta = (AllowPrivateAccess = "true"))
		float AO_Pitch;

	// Transform For Left Hand => Place On Gun
	// Equipped Weapon 에 소켓을 달고 해당 소켓을 이용하여 Left Hand 를 위치시킬 것이다.
	// ex) 내가 사용하고자 하는 Weapon 에 "LeftHandSocket" 이라는 소켓을 생성하여 사용 -> 어떤 Weapon 이던 같은 이름의 소켓을 사용
	UPROPERTY(BlueprintReadOnly, Category = Character, meta = (AllowPrivateAccess = "true"))
	FTransform LeftHandTransform;

	UPROPERTY(BlueprintReadOnly, Category = Character, meta = (AllowPrivateAccess = "true"))
	ETurningInPlace m_TurningInPlace;

	// 총구 방향이 HitTarget 을 향할 수 있게 하기 위한 Rotator
	UPROPERTY(BlueprintReadOnly, Category = Character, meta = (AllowPrivateAccess = "true"))
	FRotator RightHandRotation;

	UPROPERTY(BlueprintReadOnly, Category = Character, meta = (AllowPrivateAccess = "true"))
	bool bLocallyControlled = false;
	
	UPROPERTY(BlueprintReadOnly, Category = Character, meta = (AllowPrivateAccess = "true"))
	bool bRotateRootBone = false;
};
