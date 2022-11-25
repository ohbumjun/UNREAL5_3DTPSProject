// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterAnimInstance.h"
#include "BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "../BlasterTypes/CombatState.h"
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

	bRotateRootBone = m_BlasterCharacter->ShouldRotateRootBone();

	bElimmed = m_BlasterCharacter->IsElimmed();


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

		// 총구 방향으로 Hit Target 쪽으로 회전하는 로직을 위해서는 HitTarget 을 실시간으로 Replicate 해야 하지만
		// BandWidth가 커진다. 그냥 안한다. Locally Control 일 때만 적용할 것이다.
		if (m_BlasterCharacter->IsLocallyControlled())
		{
			bLocallyControlled = true;

			// Weapon 이 정확하게 Aim 하는 방향을 바라보게 할 것이다
			// 먼저 RightHand 에 해당하는 Bone을 가져올 것이다. (왜 ? Mesh 상에서 총을 들고 있는 Hand 가 해당 Bone 이므로)
			// FTransform RightHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(
			FTransform RightHandTransform = m_BlasterCharacter->GetMesh()->GetSocketTransform(
				FName("hand_r"),
				ERelativeTransformSpace::RTS_World);

			// hand_r 를 보면 X 축이 몸쪽으로 향하고 있다. 즉, Gun 이 향하는 방향과 반대이다. 이를 반영한다.
			
			FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(
				RightHandTransform.GetLocation(),
				RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - m_BlasterCharacter->GetHitTarget()));
		
			// 예를 들어, 먼 곳을 조준하고 있다가, 갑자기 가까운 물체를 조준하게 되면, 총구가 향하는 방향이
			// 확 바뀔 수 있다. 이를 방지하기 위해서 Interpolation 을 수행하는 것이다.
			RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaTime, 20.f);
		}

		// Reloading 중이라면 Left Hand 에 적용 중인 Left Hand IK 를 적용하지 않을 것이다
		bUseFabrick = m_BlasterCharacter->GetCombatState() != ECombatState::ECS_Reloading;

		// Reload Animation 이 진행중이라면, Aim Animation 으로 전환되지 않게 할 것이다.
		bUseAimOffset = m_BlasterCharacter->GetCombatState() != ECombatState::ECS_Reloading;

		// ex. Reload 중에는 Right Hand 를 Rotate X (Right Hand 를 Rotate 하게 되면, 총구가 향하는 방향으로 총을 돌리게 된다)
		bTransformRightHand = m_BlasterCharacter->GetCombatState() != ECombatState::ECS_Reloading;


		/* <디버그 용도>
		// 특정 Socket 의 World Transform 위치를 가져온다. (총구에 해당하는 Socket)
		FTransform MuzzleTipTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("MuzzleFlash"),
			ERelativeTransformSpace::RTS_World);
		
		// 총구가 바라보는 방향을 구한다. -> 현재 사용하는 Weapon Mesh 의 MuzzleFlash Socket 을 보면
		// X 축을 바라보고 있는 것을 확인할 수 있다.
		// 따라서 해당 Socket 이 바라보는 X 축 방향을 가져올 것이다. 
		// 위에서 World Transform 정보를 가져왔으므로, 월드 Space X 축 기준으로의 정보를 가져오게 된다.
		FVector MuzzleX(FRotationMatrix(MuzzleTipTransform.GetRotation().Rotator()).GetUnitAxis(EAxis::X));

		// MuzzleFlash Socket 총구가 바라보는 방향으로 선 그리기
		DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), 
			MuzzleTipTransform.GetLocation() + MuzzleX * 1000.f, FColor::Red);

		// Draw Line From MuzzleFlash -> Hit Point
		DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), m_BlasterCharacter->GetHitTarget(),
			FColor::Orange);
		*/
	}
}
