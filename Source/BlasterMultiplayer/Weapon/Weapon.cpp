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

	DOREPLIFETIME(AWeapon, m_WeaponState);
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

// Client ������ Replicate �Ǵ� �Լ� 
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
		break;
	}
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

	// Bullet ����
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
}

