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

	// Don't Want Character Rotate Along With Our Controller Rotation (마우스 돌아간다고, 캐릭터도 돌아가지 않게)
	// Stand Still Independent Of Controlller Rotation 
	// 캐릭터가 이동하는 방향대로 회전하게 끔
	// ex) 내가 왼쪽 이동 버튼을 눌렀다면 왼쪽으로 회전B
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	// Widget 생성
	m_OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	m_OverheadWidget->SetupAttachment(RootComponent);

	// 컴포넌트가 Replicated 되도록 세팅한다.
	// 스태틱 컴포넌트로서 동작하므로 GetLifetimeReplicatedProps 에서 별도로 등록해줄 필요가 없다.
	m_CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	m_CombatComponent->SetIsReplicated(true);

	// CanCrouch 변수를 true 로 세팅할 것이다
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

	// 마우스 아래로 하면 카메라 줌인 되는 효과 제거
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	// CombatComponent 측에서 LineTrace 를 통해 총을 쏠 위치 정보를 구하고 있다.
	// 그리고 다른 Character 를 쏠 경우, Crosshairs UI 를 빨간색으로 만들어야 한다.
	// LineTrace 의 channel 이 visibility 이다. LineTrace 를 통해 다른 Character 가 충돌되도록 인식하게 하기 위해서는
	// Mesh 가 Visibility Collision Channel 을 Block 으로 세팅해줄 것이다. 
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	// ProjectTile.cpp (생성자 참고) => 총이 맞게 하기 위한 Custom Collision Channel
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
		
	// Player Character 가 움직이는 방향으로 얼마나 빨리 돌것인가 (Z축 -> 언리얼에서는 위쪽을 향한다.)
	GetCharacterMovement()->RotationRate = FRotator(0.f, 0.f, 720.f);

	// 처음에는 not turning 으로 상태 세팅
	m_TurningInPlace = ETurningInPlace::ETIP_NotTurning;

	// Net Update Frequency 설정 -> Replication 을 이빈도만큼 한다는 것이 아니라, Replicte 여부를 해당 빈도로 체크한다는 것
	// 즉, Replication 으로 등록한 액터 및 변수들에 대해 서버측에서는 Replicat 여부를 항상 확인하는 비용이 발생한다.
	// 얼마나 자추 해당 액터가 변했는지 안변했는지를 확인할 것인가 => 변화가 발생한다면 그때는 Replicate
	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	// AimWalk Speed 를 따로 세팅한다.

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

	// 처음 시작시에 Health Value 세팅하기 
	UpdateHUDHealth();
	
	// Damage 에 대한 Event Callback 은 Server 에만 bind 할 것이다
	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ThisClass::ReceiveDamage);
	}
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Only Rotate Root Bone For Player Actually Controlling The Character
	//  IsLocallyControlled() 가 없으면, 다른 기계에서 Play 되는 캐릭터 -> 해당 캐릭터의 서버 버전도 AimOffset 을 호출하게 된다.
	// IsLocallyControlled() 를 해주면, Character 를 Play 하는 기계에서만 동작하게 할 것이다.
	if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled())
	{
		AimOffset(DeltaTime);
	}
	// Tick Function 에서가 아니라 SimProxiesTurn 에서 처리해줄 것이다.
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
		// Pitch(Y), Yaw, Roll(X) => Z 축 회전
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);

		// Creating Rotation Matrix From Rotator
		// Direction : Getting Forward Direction Of Controller (X 축 : 전방)	
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));

		// 해당 방향으로 이동시키기 
		AddMovementInput(Direction, Value);
	}
}

void ABlasterCharacter::MoveRight(float Value)
{
	if (Controller != nullptr && Value != 0.f)
	{
		// Pitch(Y), Yaw, Roll(X) => Z 축 회전
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		
		// Creating Rotation Matrix From Rotator
		// Direction : Getting Forward Direction Of Controller (Y 축 : 우측)
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
	// Server 에서 Equip 시키고 Replicate 해야 한다.
	// Client 에서 단순히 Equip 시키면 안된다.
	// if (m_CombatComponent && HasAuthority())
	// {
	// 		m_CombatComponent->EquipWeapon(m_OverlappingWeapon);
	// }

	// 위 if 문의 경우, Server 에 해당하는 Player 만이 무기를 장착할 수 있는 형태이다
	// Client 에 속하는 Player 들은 무기를 Equip 할 수 가 없다.
	// 이를 구현하기 위해서 Client 가 Server 측에게, 나 Equip 하고 싶다라는 것을 전달하고
	// Server 가 이러한 기능을 구현해주는 방식으로 동작하게 해야 한다.
	// 이를 위해 Remote Procedure Call 을 활용할 것이다 (RPC)
	// RPC 는 하나의 기계에서 Call 해서, 다른 기계에서 Execute 하게 하는 것
	// ex) Client 에서 Call 하고, Server 에서 Execute 하기
	// - 이 경우 BlasterCharacter 의 특정 함수를 Client 측에서 Call 하고, Server 에서 Execute 시키기

	// if (m_CombatComponent && HasAuthority()) => HasAuthority() 는 필요없다. 왜냐하면 해당 함수가 서버에서 호출되는 것이 보장되기 때문이다.
	if (m_CombatComponent)
	{
		// 서버에서 호출되는 것
		if (HasAuthority())
		{
			m_CombatComponent->EquipWeapon(m_OverlappingWeapon);
		}
		else
		{
			// 호출은 Client 가, 실행은 서버가 -> 서버 측에서 결과적으로 m_CombatComponent->EquipWeapon(m_OverlappingWeapon); 를 실행하면
			// Replicate 되서 다시 Client 로 변동 사항이 넘어갈 것이다.
			
			// RPC 를 전달한다. => ServerEquippedButtonPressed_Implementation 를 호출 => 클라이언트 측으로 변화 사항이 전달된다.
			ServerEquippedButtonPressed();
		}
	}
}


// RPC 가 호출되었을 때, 서버에서 할 일을 지정해주는 것
// 해당 함수는 오직 서버에서만 호출될 것이다. 그리고 Replicated 되도록 해야 한다. 
void ABlasterCharacter::ServerEquippedButtonPressed_Implementation()
{
	if (m_CombatComponent)
	{
		m_CombatComponent->EquipWeapon(m_OverlappingWeapon);
	}
}


void ABlasterCharacter::CrouchButtonPressed()
{
	// Character Class 는 기본적으로 Crouch() 함수를 상속받는다.
	// Crouch() 함수의 중요한 점은, Character 가 실제 Crouch 하게 해주는 것이 아니라
	// Crouch 를 Start 할 수 있게끔 요청만 하는 것이다.
	// Character 클래스 안의 bIsCrouched 변수를 1로 변경해준다. 뿐만 아니라 해당 변수는 Replicated 되도록 설정되어 있고
	// OnRep_IsCrouched() 함수를 통해 Replicated 될 때 해당 함수를 호출할 수 있게끔 되어 있다. 
	// 0) Blueprint 혹은 생성자를 통해 을 통해 CharacterMoveComponent()가 CanCrouch 될 수 있도록 세팅한다.
	// 1) OnRep_IsCrouched() 를 override 해서 우리가 원하는 행동을 정의할 수도 있다.
	// 2) 애니메이션 변환은 BlasterAnimInstance 의 bIsCrouched 변수를 통해 세팅할 것이다 + Animation Blueprint

	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

// 아래 2개 함수는, 서버던 , 클라이언트든 모두에서 호출하는 함수이다.
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

// 57 강
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

		// Diff In Rotation (순서가 중요하다) => 순서를 잘못 세팅하면, 마우스는 오른쪽을 바라보는데, 캐릭터는 왼쪽을 바라볼 수 있다.
		// FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(m_StartingAimRotation, CurrentAimRotation);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, m_StartingAimRotation);

		// 서있는 상태에서 얼마나 회전했는지에 대한 정보를 사용할 것이다.
		m_AO_Yaw = DeltaAimRotation.Yaw;

		// 지금 현재 Turn 하고 있지 않은 시점이라면, 기존의 AO_Yaw 값 저장해두기
		if (m_TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			m_InterpAO_Yaw = m_AO_Yaw;
		}

		// 이때 만큼은 Character 방향이 Player Controller 방향을 따라가게 한다.
		// 즉, Equip 이 없을 때는, 따라가지 않게 하다가, Running, Jumping, Equip + Stop 라는 3가지 경우에서는 방향을 일치시킨다는 것이다.
		// 단, Equip + Stop 의 경우, Blueprint Anim Graph 상에서 m_AO_Yaw * -1 각도만큼, Root Bone 을 회전시키게 해뒀다.
		// 다시 말해서, bUseControllerRotationYaw = true 를 통해 오른쪽으로 50도 회전했더라도
		// 그 반대 각도만큼 Root Bone을 회전시키니, 실제 회전은 안되는 것이다.
		// 다만, 아래 Turn In Place 함수를 통해 특정 각도 이상의 방향으로 바라보면 회전을 진행할 것이다.
		bUseControllerRotationYaw = true;

		// 현재 m_AO_Yaw 값에 따라, 제자리에서 상하좌우 바라보는 애니메이션이 바뀌는 중이다
		// 그런데 우리는 특정 각도 이내에서만 제자리 상하좌우 바라보는 애니메이션을 하고 싶다 (ex. -45 ~ 45)
		// 만약 좌우 회전량이 크게 넘어가면 ex) 왼쪽으로 90도 보기 -> 왼쪽으로 Turn 하는 애니메이션을 진행하고자 한다.
		// 단 해당 기능은, 제자리에 멈춰있을 때만 적용하고, Running 혹은 Jumping 상태에서는 적용하지 않을 것이다
		// m_AO_Yaw 값에 근거해서 제자리에서 보는 방향에 따라서 Turn 하는 애니메이션을 할지 말지를 결정한다.
		TurnInPlace(DeltaTime);
	}

	if (Speed > 0.f || bIsInAir)
	{
		// 뛰거나 IsInAir 일 때는 다시 false 로 만들어서, Rotating Root Bone 을 안하게 한다.
		m_bRotateRootBone = false;

		// Aim  방향을 현재 Aim 하고 있는 방향으로 세팅한다.
		m_StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);

		m_AO_Yaw = 0.f;

		// 이때는 다시 Character 방향이 Player Controller 방향을 따라가게 한다.
		bUseControllerRotationYaw = true;

		// Running 혹은 Jumping 일 때는 좌우로 Turn 하지 않는다.
		m_TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	
	CalculateAO_Pitch();
}


void ABlasterCharacter::CalculateAO_Pitch()
{
	// Setting Pitch
	/*
	// 58 강 참고
	m_AO_Pitch = GetBaseAimRotation().Pitch;

	우선 .. 여기서 궁금한 것은 우리는 별도로 m_AO_Pitch 등의 변수를 RPC 하고 있지도 않은데
	어떻게 서버는 해당 클리어언트의 m_AO_Pitch 등의 변수 정보를 알고 있는 것일까
	즉, 어떻게 클라이언트 측 Character 의 Rotation 정보를 알고 있는 것일까 ? (Client -> Server)

	왜냐하면 CharacterMovementComponent 상에서 알아서 Server 측으로 전달하는 로직을 내부적으로 가지고 있기 때문이다.

	----

	만약 클라이언트 상에서 단순히 이 값을 세팅한다고 해보자.
	클라이언트 상에서는 위쪽을 바라보면 0 ~ 90, 아래쪽을 바라보면 0 ~ -90 의 값을 가지게 된다.

	그런데 서버 측에서, 혹은 다른 클라이언트 측에서 클라이언트의 이 값을 바라보게 되면 0 ~ 360 의 값을 지니게 된다.
	즉, 위쪽을 보면 0 ~ 90
	아래쪽을 보면 해당 클라이언트의 Pitch 가 360 ~ 270 사이의 값을 바라보게 된다는 것이다.

	이를 이해하기 위해서는 Unreal Engine 이 네트워크 상에서 Rotation 을 어떻게 전달하는지를 살펴봐야 한다.

	CharacterMovement 에서는 Rotation 정보를 5 byte 크기로 Compress 하여 서버 측으로 전달하기 때문이다.
	쉽게 말하면 rotation 정보를 unsigned value 로 compress 한다는 것이다
	따라서 우리가 보내는 -90 ~ -1 정보가 제대로 전달되지 않는다는 것이다.
	ex) CharacterMovementComponent::PackYawAndPitchTo32() 참고
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

	// 중요한 것은, 움직이지 않는다면 해당 함수를 호출해주지 않게 된다.
	// 이를 위해서, 마지막으로 SimProxiesTurn() 를 호출한 시점을 추적해서
	// 해당 함수를 호출한 지 오래되었다면, 주기적으로 다시 호출해주는 기능을 구현할 것이다.
	// 여기서는 SimProxiesTurn() 가 호출된 것이므로, 0으로 reset 
	// 단, 주기적으로 시간에 따라 호출하는 것은 Tick 함수에서 진행해줄 것이다
	m_TimeSinceLastMovementReplication = 0.f;
}

// Turning For Simulated Proxy (Rotating Root Bone In Anim Blueprint Only Applied To Server, Local Player)
// Call This Function When Simulated Proxy Gets Updated
void ABlasterCharacter::SimProxiesTurn()
{
	if (m_CombatComponent == nullptr || m_CombatComponent->m_EquippedWeapon == nullptr)
		return;

	float Speed = CalculateSpeed();

	// 돌다가 다시 걷거나 하는 등 Speed 가 0 이상이 되면, Turn 하는 상태를 되돌린다.
	if (Speed > 0.f)
	{
		m_TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	// 해당 변수는 Anim Blueprint 상에서 Animation Blend 로 사용된다.
	m_bRotateRootBone = false;

	// 특정 Threadhold 를 정해두고, 해당 값을 Yaw 가 넘어가면 Turn In Animation 을 실행할 것이다.
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
	// 참고 : 기본적으로 Crouched 상태에서는 Jump 가 안되도록 Character 클래스가 설정해두었다.
	// 이러한 로직을 바꾸기 위해 Jump() 함수를 override 한 것이다.
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
	// Character 가 Turn Left, Right 할때의 Yaw 값을 이용할 것이다
	if (m_AO_Yaw > 90.f)
	{
		m_TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (m_AO_Yaw < -90.f)
	{
		m_TurningInPlace = ETurningInPlace::ETIP_Left;
	}

	// 새롭게 Right 혹은 Left 로 Turn 하는 것이라면 회전 각도값을 수정해줘야 한다.
	// 내 몸통이 해당 방향으로 회전하는 것이니. -90 ~~ 0 혹은 90 ~~ 0 사이의 값으로 보간하면서 회전 각도를 처리해줄 것이다.(점점 0으로)
	if (m_TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		// 5.f : 회전 속도를 조절하게 된 변수
		m_InterpAO_Yaw = FMath::FInterpTo(m_InterpAO_Yaw, 0.f, DeltaTime, 6.f);
	
		// 해당 코드를 통해, 실질적인 회전을 수행하는 것이다.
		m_AO_Yaw = m_InterpAO_Yaw;
	
		// 충분히 회전했는지 확인하기
		if (FMath::Abs(m_AO_Yaw) < 15.f)
		{
			// 다시 Not Turning 으로 바꿔주기 => Animation 에서도 이에 따라 원래 Idle 로 바뀔 것이다.
			m_TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	
			// Reset Starting Aim
			m_StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}

}

// Tick 에서 매 Frame 호출
void ABlasterCharacter::HideCameraIfCharacterClose()
{
	if (IsLocallyControlled() == false)
		return;

	// 현재 컨트롤하는 Character 가 너무 가까우면 Hide 시킨다.
	// Locally Controlled 때만 적용되므로, 다른 Player 들은 나의 Character Mesh를 볼 수 있다.
	if ((m_FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < m_CameraThreadhold)
	{
		GetMesh()->SetVisibility(false);

		if (m_CombatComponent && m_CombatComponent->m_EquippedWeapon &&
			m_CombatComponent->m_EquippedWeapon->GetWeaponMesh())
		{
			// 무기도 숨긴다.
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
	// Server 에서도 기존에 떠있던 Widget 을 숨기기
	if (m_OverlappingWeapon)
	{
		m_OverlappingWeapon->ShowPickupWidget(false);
	}

	m_OverlappingWeapon = Weapon;

	// Only Show Widget That Is Actually Controlling The Pawn
	// IsLocallyControlled() => true 라면 => Being Called On Character That is Actually Being Controlled, 
	//   By The Player Who Is Hosting Game On Server
	// (쉽게 말해 Local Role 이 Authority 인 경우에 호출하는 것 같다. )
	if (IsLocallyControlled())
	{
		// 해당 함수를 호출해주는 이유는 서버에서는 OnRep_OverlappingWeapon() 리플리케이션 함수가 호출되지 않으므로
		// 같은 기능을 하는 함수를 서버에서도 다른 방식으로 호출해줘야 하기 때문이다.
		if (m_OverlappingWeapon)
		{
			m_OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}


// m_OverlappingWeapon 에 변화가 생길 때 호출되는 Replicated 함수
// - 클라이언트 측에서만 호출되고, 서버 측에서는 호출되지 않을 것이다.
void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (m_OverlappingWeapon)
	{
		m_OverlappingWeapon->ShowPickupWidget(true);
	}

	// 현재 프로젝트에서는, 기존에 nullptr 이 아니었다가 nullptr 로 세팅하는 상태 (Widget 닫기)
	// 이게 nullptr 이라는 것은, 기존에 nullptr 이었다가 새로운 값으로 세팅하는 상태 (Widget 열기)
	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Register Variable To be Replicated 
	// - Weapon Class 에서 m_OverlappingWeapon 에 실질적인 값을 할당해줄 것이다.
	// - ABlasterCharacter 의 SetOverlappingWeapon 함수를 호출해줄 것이다.
	// (중요 : 아래 함수를 호출하면, m_OverlappingWeapon 에 변화가 생길 때 게임 내 모든 클라이언트 들에게 Replicate 되는 것이다
	// 그런데 특정 무기 근처에 가면 Pick Up Widget 이 뜨는 경우를 고려할 때, 바뀐 정보는 가까이 있는 클라이언트에게만 전달해주면 된다.
	// 굳이 무기와 떨어져있는 코든 클라이언트들에게 전달할 필요가 없다는 것이다.
	// DOREPLIFETIME(ABlasterCharacter, m_OverlappingWeapon);

	// 특정 조건을 명시할 수 있다. (이 경우, Owner, 즉, 해당 Pawn 을 컨트롤하고 있는 주체에게만 Replicated 되도록 한다.)
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
	// m_Equipped Weapon 을 가지고 있어야 한다. 우리가 사용하는 Hit Animation 이 총을 들고 있는 형태이기 때문이다.
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
	// m_CombatComponent->m_EquippedWeapon 가 replicated 되도록 해야 한다.
	// (CombatComponent 참고)
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
	// 현재 m_Health 는 Replicate 되게 된다. 즉 서버 측에서만 Health 정보가 바뀌게 해두었고, Replication 함수로 등록한
	// OnRep_Health() 를 다른 기계들에서 호출하게 되는 것이다. (정확하게는 Replication 은 다른 클라이언트들에서 호출,
	// 서버에서는 호출 X)
	// 따라서 서버에서도 변화 사항을 적절하게 반영하기 위해 , ReceiveDamage 에서 서버에 적용할 코드를 작성 
	// (해당 함수는 서버에서만 호출되므로)
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