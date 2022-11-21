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
	// ���ӵ� X => Rather Pressed Key To Move
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
	// Equipped Weapon �� ������ �ް� �ش� ������ �̿��Ͽ� Left Hand �� ��ġ��ų ���̴�.
	// ex) ���� ����ϰ��� �ϴ� Weapon �� "LeftHandSocket" �̶�� ������ �����Ͽ� ��� -> � Weapon �̴� ���� �̸��� ������ ���
	UPROPERTY(BlueprintReadOnly, Category = Character, meta = (AllowPrivateAccess = "true"))
	FTransform LeftHandTransform;

	UPROPERTY(BlueprintReadOnly, Category = Character, meta = (AllowPrivateAccess = "true"))
	ETurningInPlace m_TurningInPlace;

	// �ѱ� ������ HitTarget �� ���� �� �ְ� �ϱ� ���� Rotator
	UPROPERTY(BlueprintReadOnly, Category = Character, meta = (AllowPrivateAccess = "true"))
	FRotator RightHandRotation;

	UPROPERTY(BlueprintReadOnly, Category = Character, meta = (AllowPrivateAccess = "true"))
	bool bLocallyControlled = false;
	
	UPROPERTY(BlueprintReadOnly, Category = Character, meta = (AllowPrivateAccess = "true"))
	bool bRotateRootBone = false;
};
