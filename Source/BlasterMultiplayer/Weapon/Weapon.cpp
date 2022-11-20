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

	DOREPLIFETIME(AWeapon, m_WeaponState);
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

// Client 측으로 Replicate 되는 함수 
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
		break;
	}
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

	// Bullet 생성
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
}

