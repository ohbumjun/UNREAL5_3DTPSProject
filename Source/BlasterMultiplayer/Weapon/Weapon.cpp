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

	// ������ ��� Weapon Object �鿡 ���� ������ Replicate �� �� �ְ� ���� ���̴�.
	bReplicates = true;

	m_WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(m_WeaponMesh);

	// ���� ����߸��� , �� ��ġ�� �� �����Ǿ� ������ �ϴϱ�
	m_WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);

	// �� Pawn �� �װ��� step through �ϰų� pick up �� �� �ְ� �ϱ�
	m_WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);

	// ó������ Collision Disable �ǰ� �ص� ���̴�
	// ���⸦ ��� �� ����, Enable ��ų ���̴�
	m_WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// m_AreaSphere : Detect Overlap With Character -> Character Pick Up Weapon
	// ������, �̷��� ����� Server ������ �����ϰ� �� ���̴�.
	// BeginPlay() ���� ���� ���̴�.
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
	// ������, �̷��� ����� Server ������ �����ϰ� �� ���̴�.
	// if (GetLocalRole() == ENetRole::ROLE_Authority)
	if (HasAuthority()) // �ٷ� �� �ڵ�� ���� ���
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
		// ó������ �Ⱥ��̰� �Ѵ�.
		m_PickupWidget->SetVisibility(false);
	}
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// ��� Ŭ���̾�Ʈ�鿡�� Replicate
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
	// Blaster Character �� �浹���� ���� Widget �� �����ְ� �ʹ�

	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);

	if (BlasterCharacter)
	{
		// ���� ABlasterCharacter �� m_OverlappingWeapon �� nullptr �� �ƴ϶�, �ٸ� ������ ���õ� ���̴�.
		// This Functino Only Happen On Server => Replicate Variable To Clients
		BlasterCharacter->SetOverlappingWeapon(this);
	}
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);

	if (BlasterCharacter && m_PickupWidget)
	{
		// ���̻� �Ⱦ� ������ ������ �ʰ� �Ѵ�.
		BlasterCharacter->SetOverlappingWeapon(nullptr);
	}
}

// Client ������ Replicate �Ǵ� �Լ� (���������� ȣ�� X)
void AWeapon::OnRep_WeaponState()
{
	switch (m_WeaponState)
	{
		// �����Ǵ� ����
		// 1) Widget �����
		// 2) Collision �� Disable ��Ų��.
	case EWeaponState::EWS_Equipped :
		ShowPickupWidget(false);
		m_AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		// SetSimulatePhysics(false) �� �ؾ߸� Equip �� �ؼ� hand Socket �� ������ �� �ִ�.
		m_WeaponMesh->SetSimulatePhysics(false);
		m_WeaponMesh->SetEnableGravity(false);
		m_WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;

	case EWeaponState::EWS_Dropped:
		// m_AreaSphere �� �ε����� Equip �� �� �ְ� �ϴ� ����� ���������� �����ϵ��� �� ���̴�.
		// if (HasAuthority())
		// {
		// 	// AreaSphere �� Collision Component �� Disable ��Ų��.
		// 	m_AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		// }

		// ���� �������� ���� �� �ְ� �Ѵ�.
		m_WeaponMesh->SetSimulatePhysics(true);
		m_WeaponMesh->SetEnableGravity(true);
		m_WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

		// Widget �����
		ShowPickupWidget(true);
		break;
	}
}

// ���� �������� ȣ��Ǵ� �Լ� (CombatComponent ���� ����)
void AWeapon::SetWeaponState(EWeaponState State)
{
	m_WeaponState = State;

	switch (State)
	{
	case EWeaponState::EWS_Equipped:
		// Widget �����
		ShowPickupWidget(false);
		// AreaSphere �� Collision Component �� Disable ��Ų��.
		m_AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		// SetSimulatePhysics(false) �� �ؾ߸� Equip �� �ؼ� hand Socket �� ������ �� �ִ�.
		m_WeaponMesh->SetSimulatePhysics(false);
		m_WeaponMesh->SetEnableGravity(false);
		m_WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	case EWeaponState::EWS_Dropped:
		// �ٽ� ���⸦ �ֿ� �� �ְ� Collision �� �ٽ� Ȱ��ȭ �Ѵ�.
		if (HasAuthority())
		{
			// AreaSphere �� Collision Component �� Disable ��Ų��.
			m_AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}

		// ���� �������� ���� �� �ְ� �Ѵ�.
		m_WeaponMesh->SetSimulatePhysics(true);
		m_WeaponMesh->SetEnableGravity(true);
		m_WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

		// Widget �����
		ShowPickupWidget(true);
		break;
	}
}

bool AWeapon::IsEmpty()
{
	return m_Ammo <= 0;
}

// Replicate ������ ���ؼ� ���� ������ ȣ���ϴ�, Ŭ���̾�Ʈ ������ ȣ���ϴ� ���� ������ ����ǵ��� �����ߴ�.
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

	// Casing ����
	// ACasing Not Replicated
	// Allow Spawning ACasing Actor Locally
	if (m_CasingClass)
	{
		// �츮�� ����ϴ� Gun Skeletal Mesh �� MuzzleFlash ��� Socket �� �޾ƾ� �Ѵ�.
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
	// m_WeaponState �� Replicate �ϰ� �ȴ�. 
	SetWeaponState(EWeaponState::EWS_Dropped);

	// Detach Weapon
	// - Detach �ϴ� ���� World Transform �� ������ ���̴�. �׷��� Gravity, Physics �� ������ ��Ȳ�̹Ƿ�, 
	// - �Ʒ��� �ٷ� ������ ���̴�.
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

// Automatic Fire �� �������̹Ƿ�, �ſ� ����ϰ� �ش� �Լ��� ȣ��� ���̴�.
// Only Called On Server
void AWeapon::SpendRound()
{
	// 1. Subtract Ammo
	m_Ammo = FMath::Clamp(m_Ammo - 1, 0, m_MagCapacity);

	// 2. Update HUD Of Weapon Owner
	SetHUDAmmo();
}

// SpendRound �� Server �������� ȣ��ȴ�.
// ���� m_Ammo ������ �ٲ� �Ϳ� ���� ������ ȿ���� Ŭ���̾�Ʈ�� ��������
// ȣ��ǰԲ� �ϰ� �ʹ�.
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