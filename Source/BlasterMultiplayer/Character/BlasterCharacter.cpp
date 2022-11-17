// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h" // Replicate Variable
#include "../Weapon/Weapon.h"
#include "../BlasterComponents/CombatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "BlasterAnimInstance.h"

ABlasterCharacter::ABlasterCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	m_CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	m_CameraBoom->SetupAttachment(GetMesh());
	m_CameraBoom->TargetArmLength = 600.f;
	// We can rotate CameraBoom Along With our Controller When we have mouse Input
	m_CameraBoom->bUsePawnControlRotation = true; 

	m_FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	m_FollowCamera->SetupAttachment(m_CameraBoom, USpringArmComponent::SocketName);
	// FAlse. since it is attached to CameraBoom
	m_FollowCamera->bUsePawnControlRotation = false;

	// Don't Want Character Rotate Along With Our Controller Rotation (���콺 ���ư��ٰ�, ĳ���͵� ���ư��� �ʰ�)
	// Stand Still Independent Of Controlller Rotation 
	// ĳ���Ͱ� �̵��ϴ� ������ ȸ���ϰ� ��
	// ex) ���� ���� �̵� ��ư�� �����ٸ� �������� ȸ��B
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	// Widget ����
	m_OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	m_OverheadWidget->SetupAttachment(RootComponent);

	// ������Ʈ�� Replicated �ǵ��� �����Ѵ�.
	// ����ƽ ������Ʈ�μ� �����ϹǷ� GetLifetimeReplicatedProps ���� ������ ������� �ʿ䰡 ����.
	m_CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	m_CombatComponent->SetIsReplicated(true);

	// CanCrouch ������ true �� ������ ���̴�
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

	// ���콺 �Ʒ��� �ϸ� ī�޶� ���� �Ǵ� ȿ�� ����
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	// Player Character �� �����̴� �������� �󸶳� ���� �����ΰ� (Z�� -> �𸮾󿡼��� ������ ���Ѵ�.)
	GetCharacterMovement()->RotationRate = FRotator(0.f, 0.f, 720.f);

	// ó������ not turning ���� ���� ����
	m_TurningInPlace = ETurningInPlace::ETIP_NotTurning;

	// Net Update Frequency ���� -> Replication �� �̺󵵸�ŭ �Ѵٴ� ���� �ƴ϶�, Replicte ���θ� �ش� �󵵷� üũ�Ѵٴ� ��
	// ��, Replication ���� ����� ���� �� �����鿡 ���� ������������ Replicat ���θ� �׻� Ȯ���ϴ� ����� �߻��Ѵ�.
	// �󸶳� ���� �ش� ���Ͱ� ���ߴ��� �Ⱥ��ߴ����� Ȯ���� ���ΰ� => ��ȭ�� �߻��Ѵٸ� �׶��� Replicate
	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	// AimWalk Speed �� ���� �����Ѵ�.

}

// Called to bind functionality to input
void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ThisClass::Jump);
	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &ThisClass::EquipButtonPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ThisClass::CrouchButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &ThisClass::AimButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &ThisClass::AimButtonReleased);
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ThisClass::FireButtonPressed);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ThisClass::FireButtonReleased);

	PlayerInputComponent->BindAxis("MoveForward", this, &ThisClass::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ThisClass::MoveRight);

	PlayerInputComponent->BindAxis("Turn", this, &ThisClass::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &ThisClass::LookUp);
}


// Called when the game starts or when spawned
void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AimOffset(DeltaTime);
}

void ABlasterCharacter::MoveForward(float Value)
{
	// Controller : Inherited Variable 
	if (Controller != nullptr && Value != 0.f)
	{
		// Pitch(Y), Yaw, Roll(X) => Z �� ȸ��
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);

		// Creating Rotation Matrix From Rotator
		// Direction : Getting Forward Direction Of Controller (X �� : ����)	
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));

		// �ش� �������� �̵���Ű�� 
		AddMovementInput(Direction, Value);
	}
}

void ABlasterCharacter::MoveRight(float Value)
{
	if (Controller != nullptr && Value != 0.f)
	{
		// Pitch(Y), Yaw, Roll(X) => Z �� ȸ��
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		
		// Creating Rotation Matrix From Rotator
		// Direction : Getting Forward Direction Of Controller (Y �� : ����)
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));
		
		AddMovementInput(Direction, Value);
	}
}

void ABlasterCharacter::Turn(float Value)
{
	// Move Mouse Left, Right => AddControllerYawInput() Will Add "Yaw(Z)" To Controller Rotation
	AddControllerYawInput(Value);
}

void ABlasterCharacter::LookUp(float Value)
{
	// Move Mouse Up, Down => AddControllerYawInput() Will Add "Pitch(Y)" To Controller Rotation
	AddControllerPitchInput(Value);
}


void ABlasterCharacter::EquipButtonPressed()
{
	// Server ���� Equip ��Ű�� Replicate �ؾ� �Ѵ�.
	// Client ���� �ܼ��� Equip ��Ű�� �ȵȴ�.
	// if (m_CombatComponent && HasAuthority())
	// {
	// 		m_CombatComponent->EquipWeapon(m_OverlappingWeapon);
	// }

	// �� if ���� ���, Server �� �ش��ϴ� Player ���� ���⸦ ������ �� �ִ� �����̴�
	// Client �� ���ϴ� Player ���� ���⸦ Equip �� �� �� ����.
	// �̸� �����ϱ� ���ؼ� Client �� Server ������, �� Equip �ϰ� �ʹٶ�� ���� �����ϰ�
	// Server �� �̷��� ����� �������ִ� ������� �����ϰ� �ؾ� �Ѵ�.
	// �̸� ���� Remote Procedure Call �� Ȱ���� ���̴� (RPC)
	// RPC �� �ϳ��� ��迡�� Call �ؼ�, �ٸ� ��迡�� Execute �ϰ� �ϴ� ��
	// ex) Client ���� Call �ϰ�, Server ���� Execute �ϱ�
	// - �� ��� BlasterCharacter �� Ư�� �Լ��� Client ������ Call �ϰ�, Server ���� Execute ��Ű��

	// if (m_CombatComponent && HasAuthority()) => HasAuthority() �� �ʿ����. �ֳ��ϸ� �ش� �Լ��� �������� ȣ��Ǵ� ���� ����Ǳ� �����̴�.
	if (m_CombatComponent)
	{
		// �������� ȣ��Ǵ� ��
		if (HasAuthority())
		{
			m_CombatComponent->EquipWeapon(m_OverlappingWeapon);
		}
		else
		{
			// ȣ���� Client ��, ������ ������ -> ���� ������ ��������� m_CombatComponent->EquipWeapon(m_OverlappingWeapon); �� �����ϸ�
			// Replicate �Ǽ� �ٽ� Client �� ���� ������ �Ѿ ���̴�.
			
			// RPC �� �����Ѵ�. => ServerEquippedButtonPressed_Implementation �� ȣ�� => Ŭ���̾�Ʈ ������ ��ȭ ������ ���޵ȴ�.
			ServerEquippedButtonPressed();
		}
	}
}


// RPC �� ȣ��Ǿ��� ��, �������� �� ���� �������ִ� ��
// �ش� �Լ��� ���� ���������� ȣ��� ���̴�. �׸��� Replicated �ǵ��� �ؾ� �Ѵ�. 
void ABlasterCharacter::ServerEquippedButtonPressed_Implementation()
{
	if (m_CombatComponent)
	{
		m_CombatComponent->EquipWeapon(m_OverlappingWeapon);
	}
}


void ABlasterCharacter::CrouchButtonPressed()
{
	// Character Class �� �⺻������ Crouch() �Լ��� ��ӹ޴´�.
	// Crouch() �Լ��� �߿��� ����, Character �� ���� Crouch �ϰ� ���ִ� ���� �ƴ϶�
	// Crouch �� Start �� �� �ְԲ� ��û�� �ϴ� ���̴�.
	// Character Ŭ���� ���� bIsCrouched ������ 1�� �������ش�. �Ӹ� �ƴ϶� �ش� ������ Replicated �ǵ��� �����Ǿ� �ְ�
	// OnRep_IsCrouched() �Լ��� ���� Replicated �� �� �ش� �Լ��� ȣ���� �� �ְԲ� �Ǿ� �ִ�. 
	// 0) Blueprint Ȥ�� �����ڸ� ���� �� ���� CharacterMoveComponent()�� CanCrouch �� �� �ֵ��� �����Ѵ�.
	// 1) OnRep_IsCrouched() �� override �ؼ� �츮�� ���ϴ� �ൿ�� ������ ���� �ִ�.
	// 2) �ִϸ��̼� ��ȯ�� BlasterAnimInstance �� bIsCrouched ������ ���� ������ ���̴� + Animation Blueprint

	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

// �Ʒ� 2�� �Լ���, ������ , Ŭ���̾�Ʈ�� ��ο��� ȣ���ϴ� �Լ��̴�.
void ABlasterCharacter::AimButtonPressed()
{
	if (m_CombatComponent)
	{
		m_CombatComponent->SetAiming(true);
	}
}

void ABlasterCharacter::AimButtonReleased()
{
	if (m_CombatComponent)
	{
		m_CombatComponent->SetAiming(false);
	}
}

// 57 ��
void ABlasterCharacter::AimOffset(float DeltaTime)
{
	// No Weapon ? return
	if (m_CombatComponent && m_CombatComponent->m_EquippedWeapon == nullptr)
		return;

	// Not Executed When Running Or In Air (Jumping)
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	float Speed = Velocity.Size();

	bool bIsInAir = GetCharacterMovement()->IsFalling();

	if (Speed == 0.f && !bIsInAir)
	{
		// Standing Still !
		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);

		// Diff In Rotation (������ �߿��ϴ�) => ������ �߸� �����ϸ�, ���콺�� �������� �ٶ󺸴µ�, ĳ���ʹ� ������ �ٶ� �� �ִ�.
		// FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(m_StartingAimRotation, CurrentAimRotation);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, m_StartingAimRotation);

		// ���ִ� ���¿��� �󸶳� ȸ���ߴ����� ���� ������ ����� ���̴�.
		m_AO_Yaw = DeltaAimRotation.Yaw;

		// ���� ���� Turn �ϰ� ���� ���� �����̶��, ������ AO_Yaw �� �����صα�
		if (m_TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			m_InterpAO_Yaw = m_AO_Yaw;
		}

		// �̶� ��ŭ�� Character ������ Player Controller ������ ���󰡰� �Ѵ�.
		// ��, Equip �� ���� ����, ������ �ʰ� �ϴٰ�, Running, Jumping, Equip + Stop ��� 3���� ��쿡���� ������ ��ġ��Ų�ٴ� ���̴�.
		// ��, Equip + Stop �� ���, Blueprint Anim Graph �󿡼� m_AO_Yaw * -1 ������ŭ, Root Bone �� ȸ����Ű�� �ص״�.
		// �ٽ� ���ؼ�, bUseControllerRotationYaw = true �� ���� ���������� 50�� ȸ���ߴ���
		// �� �ݴ� ������ŭ Root Bone�� ȸ����Ű��, ���� ȸ���� �ȵǴ� ���̴�.
		// �ٸ�, �Ʒ� Turn In Place �Լ��� ���� Ư�� ���� �̻��� �������� �ٶ󺸸� ȸ���� ������ ���̴�.
		bUseControllerRotationYaw = true;

		// ���� m_AO_Yaw ���� ����, ���ڸ����� �����¿� �ٶ󺸴� �ִϸ��̼��� �ٲ�� ���̴�
		// �׷��� �츮�� Ư�� ���� �̳������� ���ڸ� �����¿� �ٶ󺸴� �ִϸ��̼��� �ϰ� �ʹ� (ex. -45 ~ 45)
		// ���� �¿� ȸ������ ũ�� �Ѿ�� ex) �������� 90�� ���� -> �������� Turn �ϴ� �ִϸ��̼��� �����ϰ��� �Ѵ�.
		// �� �ش� �����, ���ڸ��� �������� ���� �����ϰ�, Running Ȥ�� Jumping ���¿����� �������� ���� ���̴�
		// m_AO_Yaw ���� �ٰ��ؼ� ���ڸ����� ���� ���⿡ ���� Turn �ϴ� �ִϸ��̼��� ���� ������ �����Ѵ�.
		TurnInPlace(DeltaTime);
	}

	if (Speed > 0.f || bIsInAir)
	{
		// Aim  ������ ���� Aim �ϰ� �ִ� �������� �����Ѵ�.
		m_StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);

		m_AO_Yaw = 0.f;

		// �̶��� �ٽ� Character ������ Player Controller ������ ���󰡰� �Ѵ�.
		bUseControllerRotationYaw = true;

		// Running Ȥ�� Jumping �� ���� �¿�� Turn ���� �ʴ´�.
		m_TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	m_AO_Pitch = GetBaseAimRotation().Pitch;
	// Setting Pitch
	/*
	// 58 �� ����
	m_AO_Pitch = GetBaseAimRotation().Pitch;

	�켱 .. ���⼭ �ñ��� ���� �츮�� ������ m_AO_Pitch ���� ������ RPC �ϰ� ������ ������
	��� ������ �ش� Ŭ�����Ʈ�� m_AO_Pitch ���� ���� ������ �˰� �ִ� ���ϱ�
	��, ��� Ŭ���̾�Ʈ �� Character �� Rotation ������ �˰� �ִ� ���ϱ� ? (Client -> Server)

	�ֳ��ϸ� CharacterMovementComponent �󿡼� �˾Ƽ� Server ������ �����ϴ� ������ ���������� ������ �ֱ� �����̴�. 

	----
	
	���� Ŭ���̾�Ʈ �󿡼� �ܼ��� �� ���� �����Ѵٰ� �غ���.
	Ŭ���̾�Ʈ �󿡼��� ������ �ٶ󺸸� 0 ~ 90, �Ʒ����� �ٶ󺸸� 0 ~ -90 �� ���� ������ �ȴ�.

	�׷��� ���� ������, Ȥ�� �ٸ� Ŭ���̾�Ʈ ������ Ŭ���̾�Ʈ�� �� ���� �ٶ󺸰� �Ǹ� 0 ~ 360 �� ���� ���ϰ� �ȴ�.
	��, ������ ���� 0 ~ 90
	�Ʒ����� ���� �ش� Ŭ���̾�Ʈ�� Pitch �� 360 ~ 270 ������ ���� �ٶ󺸰� �ȴٴ� ���̴�.

	�̸� �����ϱ� ���ؼ��� Unreal Engine �� ��Ʈ��ũ �󿡼� Rotation �� ��� �����ϴ����� ������� �Ѵ�.

	CharacterMovement ������ Rotation ������ 5 byte ũ��� Compress �Ͽ� ���� ������ �����ϱ� �����̴�.
	���� ���ϸ� rotation ������ unsigned value �� compress �Ѵٴ� ���̴�
	���� �츮�� ������ -90 ~ -1 ������ ����� ���޵��� �ʴ´ٴ� ���̴�.
	ex) CharacterMovementComponent::PackYawAndPitchTo32() ����
	*/

	if (m_AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// map pitch from [270 , 360) to [-90,0)
		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90.f, 0.f);

		m_AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, m_AO_Pitch);
	}
}

void ABlasterCharacter::Jump()
{
	// ���� : �⺻������ Crouched ���¿����� Jump �� �ȵǵ��� Character Ŭ������ �����صξ���.
	// �̷��� ������ �ٲٱ� ���� Jump() �Լ��� override �� ���̴�.
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Super::Jump();
	}
}

void ABlasterCharacter::FireButtonPressed()
{
	if (m_CombatComponent)
	{
		m_CombatComponent->FireButtonPressed(true);
	}
}

void ABlasterCharacter::FireButtonReleased()
{
	if (m_CombatComponent)
	{
		m_CombatComponent->FireButtonPressed(false);
	}
}

void ABlasterCharacter::TurnInPlace(float DeltaTime)
{
	// Character �� Turn Left, Right �Ҷ��� Yaw ���� �̿��� ���̴�
	if (m_AO_Yaw > 90.f)
	{
		m_TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (m_AO_Yaw < -90.f)
	{
		m_TurningInPlace = ETurningInPlace::ETIP_Left;
	}

	// ���Ӱ� Right Ȥ�� Left �� Turn �ϴ� ���̶�� ȸ�� �������� ��������� �Ѵ�.
	// �� ������ �ش� �������� ȸ���ϴ� ���̴�. -90 ~~ 0 Ȥ�� 90 ~~ 0 ������ ������ �����ϸ鼭 ȸ�� ������ ó������ ���̴�.(���� 0����)
	if (m_TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		// 5.f : ȸ�� �ӵ��� �����ϰ� �� ����
		m_InterpAO_Yaw = FMath::FInterpTo(m_InterpAO_Yaw, 0.f, DeltaTime, 6.f);
	
		// �ش� �ڵ带 ����, �������� ȸ���� �����ϴ� ���̴�.
		m_AO_Yaw = m_InterpAO_Yaw;
	
		// ����� ȸ���ߴ��� Ȯ���ϱ�
		if (FMath::Abs(m_AO_Yaw) < 15.f)
		{
			// �ٽ� Not Turning ���� �ٲ��ֱ� => Animation ������ �̿� ���� ���� Idle �� �ٲ� ���̴�.
			m_TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	
			// Reset Starting Aim
			m_StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}

}

// Function Only Called From Server
void ABlasterCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	// Server ������ ������ ���ִ� Widget �� �����
	if (m_OverlappingWeapon)
	{
		m_OverlappingWeapon->ShowPickupWidget(false);
	}

	m_OverlappingWeapon = Weapon;

	// Only Show Widget That Is Actually Controlling The Pawn
	// IsLocallyControlled() => true ��� => Being Called On Character That is Actually Being Controlled, 
	//   By The Player Who Is Hosting Game On Server
	// (���� ���� Local Role �� Authority �� ��쿡 ȣ���ϴ� �� ����. )
	if (IsLocallyControlled())
	{
		// �ش� �Լ��� ȣ�����ִ� ������ ���������� OnRep_OverlappingWeapon() ���ø����̼� �Լ��� ȣ����� �����Ƿ�
		// ���� ����� �ϴ� �Լ��� ���������� �ٸ� ������� ȣ������� �ϱ� �����̴�.
		if (m_OverlappingWeapon)
		{
			m_OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}


// m_OverlappingWeapon �� ��ȭ�� ���� �� ȣ��Ǵ� Replicated �Լ�
// - Ŭ���̾�Ʈ �������� ȣ��ǰ�, ���� �������� ȣ����� ���� ���̴�.
void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (m_OverlappingWeapon)
	{
		m_OverlappingWeapon->ShowPickupWidget(true);
	}

	// ���� ������Ʈ������, ������ nullptr �� �ƴϾ��ٰ� nullptr �� �����ϴ� ���� (Widget �ݱ�)
	// �̰� nullptr �̶�� ����, ������ nullptr �̾��ٰ� ���ο� ������ �����ϴ� ���� (Widget ����)
	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Register Variable To be Replicated 
	// - Weapon Class ���� m_OverlappingWeapon �� �������� ���� �Ҵ����� ���̴�.
	// - ABlasterCharacter �� SetOverlappingWeapon �Լ��� ȣ������ ���̴�.
	// (�߿� : �Ʒ� �Լ��� ȣ���ϸ�, m_OverlappingWeapon �� ��ȭ�� ���� �� ���� �� ��� Ŭ���̾�Ʈ �鿡�� Replicate �Ǵ� ���̴�
	// �׷��� Ư�� ���� ��ó�� ���� Pick Up Widget �� �ߴ� ��츦 ����� ��, �ٲ� ������ ������ �ִ� Ŭ���̾�Ʈ���Ը� �������ָ� �ȴ�.
	// ���� ����� �������ִ� �ڵ� Ŭ���̾�Ʈ�鿡�� ������ �ʿ䰡 ���ٴ� ���̴�.
	// DOREPLIFETIME(ABlasterCharacter, m_OverlappingWeapon);

	// Ư�� ������ ����� �� �ִ�. (�� ���, Owner, ��, �ش� Pawn �� ��Ʈ���ϰ� �ִ� ��ü���Ը� Replicated �ǵ��� �Ѵ�.)
	DOREPLIFETIME_CONDITION(ABlasterCharacter, m_OverlappingWeapon, COND_OwnerOnly)
	
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (m_CombatComponent)
	{
		m_CombatComponent->m_BlasterCharacter = this;
	}
}

void ABlasterCharacter::PlayFireMontage(bool bAiming)
{
	if (m_CombatComponent == nullptr || m_CombatComponent->m_EquippedWeapon == nullptr)
		return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	// Play Fire Montage
	if (AnimInstance && m_FireWeaponMontage)
	{
		AnimInstance->Montage_Play(m_FireWeaponMontage);

		// Choose Section Name
		FName SectionName;
		SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

bool ABlasterCharacter::IsWeaponEquipped()
{
	// m_CombatComponent->m_EquippedWeapon �� replicated �ǵ��� �ؾ� �Ѵ�.
	// (CombatComponent ����)
	return (m_CombatComponent && m_CombatComponent->m_EquippedWeapon);
}

bool ABlasterCharacter::IsAiming()
{
	return (m_CombatComponent && m_CombatComponent->m_bAiming);
}

AWeapon* ABlasterCharacter::GetEquippedWeapon()
{
	if (m_CombatComponent == nullptr)
		return nullptr;

	return m_CombatComponent->m_EquippedWeapon;
}
