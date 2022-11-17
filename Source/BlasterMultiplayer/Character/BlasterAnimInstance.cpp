// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterAnimInstance.h"
#include "BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "../Weapon/Weapon.h"

void UBlasterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

}

void UBlasterAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (m_BlasterCharacter == nullptr)
	{
		m_BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
	}

	if (m_BlasterCharacter == nullptr)
		return;

	// Speed
	FVector Velocity = m_BlasterCharacter->GetVelocity();
	Velocity.Z = 0.f; // Not Moving Up
	Speed = Velocity.Size();

	// Is In Air
	bIsInAir = m_BlasterCharacter->GetCharacterMovement()->IsFalling();

	IsAccelerating = m_BlasterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;

	bWeaponEquipped = m_BlasterCharacter->IsWeaponEquipped();

	EquippedWeapon = m_BlasterCharacter->GetEquippedWeapon();

	bIsCrouched = m_BlasterCharacter->bIsCrouched;

	bIsAiming = m_BlasterCharacter->IsAiming();

	m_TurningInPlace = m_BlasterCharacter->GetTurningInPlace();

	// �Ʒ� ���� �������� �̹� Replicate �ǰ� �ֱ� ������, ������ Replicate ó���� ���� �ʿ䰡 ����.
	// 1) Offset Yaw For Strafing
	// YawOffset �� ���ϱ� ���ؼ��� � �������� �����̴���, � �������� Aiming �ϴ����� ������� �Ѵ�. (PlayerController �� Pointing �ϴ� ��)
	// ���콺�� ���������� ������ Yaw ���� +, �������� ������ -
	// -180 ~ 180 ������ ���� ������ �ȴ�.
	FRotator AimRotation = m_BlasterCharacter->GetBaseAimRotation();
	// UE_LOG(LogTemp, Warning, TEXT("MovementRotation Yaw %f : "), AimRotation.Yaw);

	// Movement�� Rotation �� �˾ƾ� �Ѵ�. => Direction Vector �� ���ڷ� �ʿ�� �ϰ� , Velocity �� ���ڷ� �Ѱ��� ���̴�.
	// ��, ���� Character �� �ٶ󺸴� World ���⿡ ���� Rotation ������ ����ִ�.
	// AimRotation : ���콺�� �ش��ϴ� �� -> PlayerController �� ������ �� ����. World �󿡼��� ...?
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(m_BlasterCharacter->GetVelocity());
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 6.f);
	YawOffset = DeltaRotation.Yaw;

	// 2) Lean
	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = m_BlasterCharacter->GetActorRotation(); // Get Rotation Of Root Component

	// Diff VAl
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaTime;
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 6.f);
	Lean = FMath::Clamp(Interp, -90.f, 90.f);

	AO_Yaw = m_BlasterCharacter->GetAO_Yaw();
	AO_Pitch = m_BlasterCharacter->GetAO_Pitch();

	// Place Left Hand To Weapon
	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && m_BlasterCharacter->GetMesh())
	{
		// Get Socket "World Space" Tranform From Custom Socket On Weapon
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), 
			ERelativeTransformSpace::RTS_World);

		// Transform To Bone Space Of Our Skeletal In Mesh
		// LeftHand �� LeftHandSocket ��ġ�� �̵���Ű�� ���� ���̱� �����̴�.
		// ��. �츮�� Mesh �� �⺻������ ���� ������ hand �� ��� �ְ� �����ߴ�.
		// �׷��ٸ� ������ Bone �� ���� LeftHandSocket �� ������� Transform ������ �˾Ƴ��� ���̴�.
		// �� �������� �Ʒ��� OutPosition, OutRotation �� ���� �� ���̴�.
		FVector OutPosition;
		FRotator OutRotation;
		m_BlasterCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), 
			FRotator::ZeroRotator, OutPosition, OutRotation);

		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));

		// ��, ���� ���ϸ� 1) LeftHandSocket �� World Space �� ����
		// 2) ������ Bone �� ���� LeftHandSocket �� Bone Space ������ LeftTransform �� ������ ���̴�.
		// �̸� Animation Blueprint ���� ����� ���̴�.
	}
}
