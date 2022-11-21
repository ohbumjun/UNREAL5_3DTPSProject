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
#include "../PlayerController/BlasterPlayerController.h"
#include "../BlasterMultiplayer.h"
#include "../GameMode/BlasterGameMode.h"

ABlasterCharacter::ABlasterCharacter()
{
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
	// CombatComponent ������ LineTrace �� ���� ���� �� ��ġ ������ ���ϰ� �ִ�.
	// �׸��� �ٸ� Character �� �� ���, Crosshairs UI �� ���������� ������ �Ѵ�.
	// LineTrace �� channel �� visibility �̴�. LineTrace �� ���� �ٸ� Character �� �浹�ǵ��� �ν��ϰ� �ϱ� ���ؼ���
	// Mesh �� Visibility Collision Channel �� Block ���� �������� ���̴�. 
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	// ProjectTile.cpp (������ ����) => ���� �°� �ϱ� ���� Custom Collision Channel
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
		
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

	// ó�� ���۽ÿ� Health Value �����ϱ� 
	UpdateHUDHealth();
	
	// Damage �� ���� Event Callback �� Server ���� bind �� ���̴�
	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ThisClass::ReceiveDamage);
	}
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Only Rotate Root Bone For Player Actually Controlling The Character
	//  IsLocallyControlled() �� ������, �ٸ� ��迡�� Play �Ǵ� ĳ���� -> �ش� ĳ������ ���� ������ AimOffset �� ȣ���ϰ� �ȴ�.
	// IsLocallyControlled() �� ���ָ�, Character �� Play �ϴ� ��迡���� �����ϰ� �� ���̴�.
	if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled())
	{
		AimOffset(DeltaTime);
	}
	// Tick Function ������ �ƴ϶� SimProxiesTurn ���� ó������ ���̴�.
	// else
	// {
	// 	SimProxiesTurn();
	// }
	else
	{
		m_TimeSinceLastMovementReplication += DeltaTime;

		if (m_TimeSinceLastMovementReplication > 0.05f)
		{
			OnRep_ReplicatedMovement();
		}
		
		CalculateAO_Pitch();
	}

	HideCameraIfCharacterClose();
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
// - Calculate AO_Yaw Variable
void ABlasterCharacter::AimOffset(float DeltaTime)
{
	// No Weapon ? return
	if (m_CombatComponent && m_CombatComponent->m_EquippedWeapon == nullptr)
		return;

	// Not Executed When Running Or In Air (Jumping)
	float Speed =CalculateSpeed();

	bool bIsInAir = GetCharacterMovement()->IsFalling();

	if (Speed == 0.f && !bIsInAir)
	{
		m_bRotateRootBone = true;

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
		// �ٰų� IsInAir �� ���� �ٽ� false �� ����, Rotating Root Bone �� ���ϰ� �Ѵ�.
		m_bRotateRootBone = false;

		// Aim  ������ ���� Aim �ϰ� �ִ� �������� �����Ѵ�.
		m_StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);

		m_AO_Yaw = 0.f;

		// �̶��� �ٽ� Character ������ Player Controller ������ ���󰡰� �Ѵ�.
		bUseControllerRotationYaw = true;

		// Running Ȥ�� Jumping �� ���� �¿�� Turn ���� �ʴ´�.
		m_TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	
	CalculateAO_Pitch();
}


void ABlasterCharacter::CalculateAO_Pitch()
{
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

	m_AO_Pitch = GetBaseAimRotation().Pitch;

	if (m_AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// map pitch from [270 , 360) to [-90,0)
		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90.f, 0.f);

		m_AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, m_AO_Pitch);
	}
}

float ABlasterCharacter::CalculateSpeed()
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	// float Speed = Velocity.Size();
	return Velocity.Size();
}

// Whenever Actor's Movement Changes, It is replicated, and below function is called
// -> Call SimProxiesTurn Instead From Tick Function
// -> Call This Function For The Server That is not Locally Controller
void ABlasterCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();

	SimProxiesTurn();

	// �߿��� ����, �������� �ʴ´ٸ� �ش� �Լ��� ȣ�������� �ʰ� �ȴ�.
	// �̸� ���ؼ�, ���������� SimProxiesTurn() �� ȣ���� ������ �����ؼ�
	// �ش� �Լ��� ȣ���� �� �����Ǿ��ٸ�, �ֱ������� �ٽ� ȣ�����ִ� ����� ������ ���̴�.
	// ���⼭�� SimProxiesTurn() �� ȣ��� ���̹Ƿ�, 0���� reset 
	// ��, �ֱ������� �ð��� ���� ȣ���ϴ� ���� Tick �Լ����� �������� ���̴�
	m_TimeSinceLastMovementReplication = 0.f;
}

// Turning For Simulated Proxy (Rotating Root Bone In Anim Blueprint Only Applied To Server, Local Player)
// Call This Function When Simulated Proxy Gets Updated
void ABlasterCharacter::SimProxiesTurn()
{
	if (m_CombatComponent == nullptr || m_CombatComponent->m_EquippedWeapon == nullptr)
		return;

	float Speed = CalculateSpeed();

	// ���ٰ� �ٽ� �Ȱų� �ϴ� �� Speed �� 0 �̻��� �Ǹ�, Turn �ϴ� ���¸� �ǵ�����.
	if (Speed > 0.f)
	{
		m_TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	// �ش� ������ Anim Blueprint �󿡼� Animation Blend �� ���ȴ�.
	m_bRotateRootBone = false;

	// Ư�� Threadhold �� ���صΰ�, �ش� ���� Yaw �� �Ѿ�� Turn In Animation �� ������ ���̴�.
	m_ProxyRotationLastFrame = m_ProxyRotation;
	m_ProxyRotation = GetActorRotation();

	// Calculate Rotation From Last Frame
	m_ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(m_ProxyRotation, m_ProxyRotationLastFrame).Yaw;

	if (FMath::Abs(m_ProxyYaw) > m_TurnThreshold)
	{
		if (m_ProxyYaw > m_TurnThreshold)
		{
			m_TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else if (m_ProxyYaw < -1 * m_TurnThreshold)
		{
			m_TurningInPlace = ETurningInPlace::ETIP_Left;
		}
		else
		{
			m_TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}

		return;
	}


	// Default Behavior
	m_TurningInPlace = ETurningInPlace::ETIP_NotTurning;
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

// Tick ���� �� Frame ȣ��
void ABlasterCharacter::HideCameraIfCharacterClose()
{
	if (IsLocallyControlled() == false)
		return;

	// ���� ��Ʈ���ϴ� Character �� �ʹ� ������ Hide ��Ų��.
	// Locally Controlled ���� ����ǹǷ�, �ٸ� Player ���� ���� Character Mesh�� �� �� �ִ�.
	if ((m_FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < m_CameraThreadhold)
	{
		GetMesh()->SetVisibility(false);

		if (m_CombatComponent && m_CombatComponent->m_EquippedWeapon &&
			m_CombatComponent->m_EquippedWeapon->GetWeaponMesh())
		{
			// ���⵵ �����.
			m_CombatComponent->m_EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);

		if (m_CombatComponent && m_CombatComponent->m_EquippedWeapon &&
			m_CombatComponent->m_EquippedWeapon->GetWeaponMesh())
		{
			m_CombatComponent->m_EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
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

	DOREPLIFETIME(ABlasterCharacter, m_Health);
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

void ABlasterCharacter::PlayElimMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	// Play Fire Montage
	if (AnimInstance && m_ElimMontage)
	{
		AnimInstance->Montage_Play(m_ElimMontage);
	}
}

void ABlasterCharacter::PlayHitReactMontage()
{
	// m_Equipped Weapon �� ������ �־�� �Ѵ�. �츮�� ����ϴ� Hit Animation �� ���� ��� �ִ� �����̱� �����̴�.
	if (m_CombatComponent == nullptr || m_CombatComponent->m_EquippedWeapon == nullptr)
		return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	// Play Fire Montage
	if (AnimInstance && m_HitReactMontage)
	{
		AnimInstance->Montage_Play(m_HitReactMontage);

		// Choose Section Name
		FName SectionName(TEXT("FromFront"));
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

FVector ABlasterCharacter::GetHitTarget() const
{
	if (m_CombatComponent == nullptr)
		return FVector();

	return m_CombatComponent->m_HitTarget;
}


void ABlasterCharacter::OnRep_Health()
{
	UpdateHUDHealth();
	PlayHitReactMontage();
}


void ABlasterCharacter::UpdateHUDHealth()
{
	m_BlasterPlayerController = m_BlasterPlayerController == nullptr ?
		Cast<ABlasterPlayerController>(Controller) :
		m_BlasterPlayerController;

	if (m_BlasterPlayerController)
	{
		m_BlasterPlayerController->SetHUDHealth(m_Health, m_MaxHealth);
	}
}

void ABlasterCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	class AController* InstigatorController, AActor* DamageCursor)
{
	// ���� m_Health �� Replicate �ǰ� �ȴ�. �� ���� �������� Health ������ �ٲ�� �صξ���, Replication �Լ��� �����
	// OnRep_Health() �� �ٸ� ���鿡�� ȣ���ϰ� �Ǵ� ���̴�. (��Ȯ�ϰԴ� Replication �� �ٸ� Ŭ���̾�Ʈ�鿡�� ȣ��,
	// ���������� ȣ�� X)
	// ���� ���������� ��ȭ ������ �����ϰ� �ݿ��ϱ� ���� , ReceiveDamage ���� ������ ������ �ڵ带 �ۼ� 
	// (�ش� �Լ��� ���������� ȣ��ǹǷ�)
	m_Health = FMath::Clamp(m_Health - Damage, 0.f, m_MaxHealth);

	PlayHitReactMontage();

	UpdateHUDHealth();

	if (m_Health <= 0.f)
	{
		ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();

		if (BlasterGameMode)
		{
			m_BlasterPlayerController = m_BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : m_BlasterPlayerController;
			ABlasterPlayerController* AttackerController = Cast<ABlasterPlayerController>(InstigatorController);
			BlasterGameMode->PlayerEliminated(this, m_BlasterPlayerController, AttackerController);
		}
	}
}

void ABlasterCharacter::Elim_Implementation()
{
	m_bElimmed = true;

	PlayElimMontage();
}