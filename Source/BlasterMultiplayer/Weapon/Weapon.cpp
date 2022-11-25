// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h" // Replicate Variable
#include "BlasterMultiplayer/Character/BlasterCharacter.h"
#include "Animation/AnimationAsset.h"
#include "Casing.h"
#include "Engine/SkeletalMeshSocket.h"
#include "../PlayerController/BlasterPlayerController.h"

AWeapon::AWeapon()
{
	// Tick False
	PrimaryActorTick.bCanEverTick = false;

	// 서버가 모든 Weapon Object 들에 대한 정보를 Replicate 할 수 있게 해줄 것이다.
	bReplicates = true;

	m_WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(m_WeaponMesh);

	// 땅에 떨어뜨리면 , 그 위치에 딱 고정되어 놓여야 하니까
	m_WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);

	// 단 Pawn 은 그것을 step through 하거나 pick up 할 수 있게 하기
	m_WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);

	// 처음에는 Collision Disable 되게 해둘 것이다
	// 무기를 들고 난 이후, Enable 시킬 것이다
	m_WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// m_AreaSphere : Detect Overlap With Character -> Character Pick Up Weapon
	// 하지만, 이러한 기능을 Server 에서만 동작하게 할 것이다.
	// BeginPlay() 에서 해줄 것이다.
	m_AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	m_AreaSphere->SetupAttachment(RootComponent);
	m_AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	m_AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	m_PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	m_PickupWidget->SetupAttachment(RootComponent);
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();
	
	// m_AreaSphere : Detect Overlap With Character -> Character Pick Up Weapon
	// 하지만, 이러한 기능을 Server 에서만 동작하게 할 것이다.
	// if (GetLocalRole() == ENetRole::ROLE_Authority)
	if (HasAuthority()) // 바로 위 코드와 같은 기능
	{
		m_AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

		m_AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	
		// Bind Over Lap Function (Begin)
		m_AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereOverlap);

		// Bind Over Lap Function (End)
		m_AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnSphereEndOverlap);
	}

	if (m_PickupWidget)
	{
		// 처음에는 안보이게 한다.
		m_PickupWidget->SetVisibility(false);
	}
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 모든 클라이언트들에게 Replicate
	DOREPLIFETIME(AWeapon, m_WeaponState);
	DOREPLIFETIME(AWeapon, m_Ammo);
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Blaster Character 와 충돌했을 때만 Widget 을 보여주고 싶다

	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);

	if (BlasterCharacter)
	{
		// 이제 ABlasterCharacter 의 m_OverlappingWeapon 이 nullptr 이 아니라, 다른 값으로 세팅될 것이다.
		// This Functino Only Happen On Server => Replicate Variable To Clients
		BlasterCharacter->SetOverlappingWeapon(this);
	}
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);

	if (BlasterCharacter && m_PickupWidget)
	{
		// 더이상 픽업 위젯을 보이지 않게 한다.
		BlasterCharacter->SetOverlappingWeapon(nullptr);
	}
}

// Client 측으로 Replicate 되는 함수 (서버에서는 호출 X)
void AWeapon::OnRep_WeaponState()
{
	switch (m_WeaponState)
	{
		// 장착되는 순간
		// 1) Widget 숨기고
		// 2) Collision 을 Disable 시킨다.
	case EWeaponState::EWS_Equipped :
		ShowPickupWidget(false);
		m_AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		// SetSimulatePhysics(false) 를 해야만 Equip 을 해서 hand Socket 에 장착할 수 있다.
		m_WeaponMesh->SetSimulatePhysics(false);
		m_WeaponMesh->SetEnableGravity(false);
		m_WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;

	case EWeaponState::EWS_Dropped:
		// m_AreaSphere 에 부딪혀서 Equip 할 수 있게 하는 기능은 서버에서만 가능하도록 할 것이다.
		// if (HasAuthority())
		// {
		// 	// AreaSphere 의 Collision Component 를 Disable 시킨다.
		// 	m_AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		// }

		// 땅에 떨어지면 멈출 수 있게 한다.
		m_WeaponMesh->SetSimulatePhysics(true);
		m_WeaponMesh->SetEnableGravity(true);
		m_WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

		// Widget 숨기기
		ShowPickupWidget(true);
		break;
	}
}

// 서버 측에서만 호출되는 함수 (CombatComponent 에서 진행)
void AWeapon::SetWeaponState(EWeaponState State)
{
	m_WeaponState = State;

	switch (State)
	{
	case EWeaponState::EWS_Equipped:
		// Widget 숨기기
		ShowPickupWidget(false);
		// AreaSphere 의 Collision Component 를 Disable 시킨다.
		m_AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		// SetSimulatePhysics(false) 를 해야만 Equip 을 해서 hand Socket 에 장착할 수 있다.
		m_WeaponMesh->SetSimulatePhysics(false);
		m_WeaponMesh->SetEnableGravity(false);
		m_WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	case EWeaponState::EWS_Dropped:
		// 다시 무기를 주울 수 있게 Collision 을 다시 활성화 한다.
		if (HasAuthority())
		{
			// AreaSphere 의 Collision Component 를 Disable 시킨다.
			m_AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}

		// 땅에 떨어지면 멈출 수 있게 한다.
		m_WeaponMesh->SetSimulatePhysics(true);
		m_WeaponMesh->SetEnableGravity(true);
		m_WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

		// Widget 숨기기
		ShowPickupWidget(true);
		break;
	}
}

bool AWeapon::IsEmpty()
{
	return m_Ammo <= 0;
}

// Replicate 과정을 통해서 서버 측에서 호출하던, 클라이언트 측에서 호출하던 변동 사항이 적용되도록 세팅했다.
void AWeapon::ShowPickupWidget(bool bShowWidget)
{
	if (m_PickupWidget)
	{
		m_PickupWidget->SetVisibility(bShowWidget);
	}
}

void AWeapon::Fire(const FVector& HitTarget)
{
	if (m_FireAnimation)
	{
		// Play Animation Using Skeletal Mesh 
		m_WeaponMesh->PlayAnimation(m_FireAnimation, false); // false : No Loop
	}

	// Casing 생성
	// ACasing Not Replicated
	// Allow Spawning ACasing Actor Locally
	if (m_CasingClass)
	{
		// 우리가 사용하는 Gun Skeletal Mesh 에 MuzzleFlash 라는 Socket 을 달아야 한다.
		const USkeletalMeshSocket* AmmoEjectSocket = m_WeaponMesh->GetSocketByName(FName("AmmoEject"));

		if (AmmoEjectSocket)
		{
			FTransform SocketTransform = AmmoEjectSocket->GetSocketTransform(m_WeaponMesh);

			UWorld* World = GetWorld();

			// Simply Spawning Actor
			if (World)
			{
				World->SpawnActor<ACasing>(
					m_CasingClass,
					// Spawn At "MuzzleFlash" Socket
					SocketTransform.GetLocation(),
					SocketTransform.GetRotation().Rotator()
				);
			}
		}
	}

	SpendRound();
}

// Called Only From Server
void AWeapon::Dropped()
{
	// m_WeaponState 는 Replicate 하게 된다. 
	SetWeaponState(EWeaponState::EWS_Dropped);

	// Detach Weapon
	// - Detach 하는 순간 World Transform 을 유지할 것이다. 그런데 Gravity, Physics 를 적용한 상황이므로, 
	// - 아래로 바로 떨어질 것이다.
	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	
	m_WeaponMesh->DetachFromComponent(DetachRules);

	// Weapons' Owner Set To nullptr (Replicated Internally)
	SetOwner(nullptr);

	m_BlasterOwnerCharacter = nullptr;
	m_BlasterOwnerController = nullptr;
}

void AWeapon::AddAmmo(int32 AmmoToAdd)
{
	m_Ammo = FMath::Clamp(m_Ammo - AmmoToAdd, 0, m_MagCapacity);

	SetHUDAmmo();
}

void AWeapon::SetHUDAmmo()
{
	m_BlasterOwnerCharacter = m_BlasterOwnerCharacter == nullptr ?
		Cast<ABlasterCharacter>(GetOwner()) : m_BlasterOwnerCharacter;

	if (m_BlasterOwnerCharacter)
	{
		m_BlasterOwnerController = m_BlasterOwnerController == nullptr ?
			Cast<ABlasterPlayerController>(m_BlasterOwnerCharacter->Controller) :
			m_BlasterOwnerController;

		if (m_BlasterOwnerController)
		{
			m_BlasterOwnerController->SetHUDWeaponAmmo(m_Ammo);
		}
	}
}

// Automatic Fire 를 진행중이므로, 매우 빈번하게 해당 함수가 호출될 것이다.
// Only Called On Server
void AWeapon::SpendRound()
{
	// 1. Subtract Ammo
	m_Ammo = FMath::Clamp(m_Ammo - 1, 0, m_MagCapacity);

	// 2. Update HUD Of Weapon Owner
	SetHUDAmmo();
}

// SpendRound 는 Server 측에서만 호출된다.
// 따라서 m_Ammo 변수가 바뀐 것에 대한 동일한 효과가 클라이언트들 측에서도
// 호출되게끔 하고 싶다.
void AWeapon::OnRep_Ammo()
{
	SetHUDAmmo();
}

void AWeapon::OnRep_Owner()
{
	Super::OnRep_Owner();

	if (Owner == nullptr)
	{
		m_BlasterOwnerCharacter = nullptr;
		m_BlasterOwnerController = nullptr;
	}
	else
	{
		SetHUDAmmo();
	}
}