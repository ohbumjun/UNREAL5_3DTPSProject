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

	// 아래 관련 변수들은 이미 Replicate 되고 있기 때문에, 별도로 Replicate 처리를 해줄 필요가 없다.
	// 1) Offset Yaw For Strafing
	// YawOffset 을 구하기 위해서는 어떤 방향으로 움직이는지, 어떤 방향으로 Aiming 하는지를 살펴봐야 한다. (PlayerController 가 Pointing 하는 곳)
	// 마우스를 오른쪽으로 돌리면 Yaw 값은 +, 왼쪽으로 돌리면 -
	// -180 ~ 180 사이의 값을 가지게 된다.
	FRotator AimRotation = m_BlasterCharacter->GetBaseAimRotation();
	// UE_LOG(LogTemp, Warning, TEXT("MovementRotation Yaw %f : "), AimRotation.Yaw);

	// Movement의 Rotation 도 알아야 한다. => Direction Vector 를 인자로 필요로 하고 , Velocity 를 인자로 넘겨줄 것이다.
	// 즉, 실제 Character 가 바라보는 World 방향에 대한 Rotation 정보가 들어있다.
	// AimRotation : 마우스에 해당하는 것 -> PlayerController 의 방향인 것 같다. World 상에서의 ...?
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
		// LeftHand 를 LeftHandSocket 위치로 이동시키고 싶은 것이기 때문이다.
		// 자. 우리는 Mesh 가 기본적으로 총을 오른쪽 hand 에 들고 있게 세팅했다.
		// 그렇다면 오른손 Bone 에 대한 LeftHandSocket 의 상대적인 Transform 정보를 알아내올 것이다.
		// 그 정보값이 아래의 OutPosition, OutRotation 에 들어가게 될 것이다.
		FVector OutPosition;
		FRotator OutRotation;
		m_BlasterCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), 
			FRotator::ZeroRotator, OutPosition, OutRotation);

		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));

		// 즉, 쉽게 말하면 1) LeftHandSocket 의 World Space 에 접근
		// 2) 오른손 Bone 에 대한 LeftHandSocket 의 Bone Space 정보를 LeftTransform 에 저장한 것이다.
		// 이를 Animation Blueprint 에서 사용할 것이다.
	}
}
