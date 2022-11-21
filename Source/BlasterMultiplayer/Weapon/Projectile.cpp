// Fill out your copyright notice in the Description page of Project Settings.

#include "Projectile.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Gameframework/ProjectileMovementComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"
#include "../Character/BlasterCharacter.h"
#include "../BlasterMultiplayer.h"

// Sets default values
AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	// Server ���� Spawn �Ǹ�, �׷��� Spawn Action �� Ŭ���̾�Ʈ�鿡�� Replicate �Ǵ� �� (��, Ŭ���̾�Ʈ �������� ����)
	bReplicates = true;

	m_CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(m_CollisionBox);
	m_CollisionBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	m_CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	m_CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	m_CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, 
		ECollisionResponse::ECR_Block);
	m_CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, 
		ECollisionResponse::ECR_Block);
	
	// 1_1) Projectile �� Character �ʹ� �浹�ؾ� �Ѵ�. ��Ȯ�ϰԴ� Character �� Mesh Component �� �浹�ؾ� �ϴµ�
	// Mesh Component �� ObjectType �� Pawn �̹Ƿ�, Pawn �� �浹�ϰ� �ؾ� �Ѵ�.
	// m_CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);

	// 1_2) ������, Character �� ���, Root Component �� Capsule Component ���� Object Type �� Pawn �̴�
	// �츮�� ��Ȯ�ϰ� Mesh �� ������ ���� �����Ű�� �ʹ�.
	// ���� Custom Collision Channel �� ���� Character �� Mesh �� �����Ű��
	// �ش� Custom Channel �� ���ؼ� Block �� �ǵ��� ������ ���̴�.
	m_CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECollisionResponse::ECR_Block);

	m_ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(
		TEXT("ProjectileMovmentComponent"));
	// Bullet Keep Rotation Along With Velocity
	// ex) �߷¿� ���� �������� Root Component �� Rotation ���� ���ϰ� �� ���̴�.
	m_ProjectileMovementComponent->bRotationFollowsVelocity = true;

	// �̰� ���û��� -> ProjectileMovement Speed �����ϱ�
	m_ProjectileMovementComponent->InitialSpeed = 8000.f;
	m_ProjectileMovementComponent->MaxSpeed = 8000.f;

}

// Called when the game starts or when spawned
void AProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	if (m_Tracer)
	{
		// Attach To Root Component Or Collision Box
		m_TracerComponent = UGameplayStatics::SpawnEmitterAttached(
			m_Tracer, // Particle System
			m_CollisionBox, // Attach to Root
			FName(), // If Want To Attach To Certain Bone, Set Name Of Bone
			GetActorLocation(),
			GetActorRotation(),
			// Spawn Tracer Component At The Position Of Root Component, And Follow
			EAttachLocation::KeepWorldPosition 
		);
	}

	// Bind Hit Event To Delegate
	// - Only At Server -> Only Be Generated On Server, Not Client -> We Won't Get Hit Events On Client
	// Why ? Because Server Need Authority Over It
	if (HasAuthority())
	{
		m_CollisionBox->OnComponentHit.AddDynamic(this, &ThisClass::OnHit);
	}
}

// Spawn Partcle And Sound
// Only Called From Server Side
void AProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	/*
	�̷��� �ϸ� Server �������� �����ȴ�.
	��, ���� Client������ ���� ����, �浹 �ÿ� Particle �� Sound �� Server ������ ���̰� �ȴ�
	�ֳ��ϸ� ���� RPC, FNetVector_Quantize ���� ���ؼ�, Client������ Projecttile �� �����ϸ�
	����, Ŭ���̾�Ʈ ��ο��� ������ �� �ְ� �ߴ�.

	��, BeginPlay() ���� Server �� ���� �ش� Overlap Event �Լ��� ȣ��ǰ� �����Ƿ�
	������������ ���̰� �Ǵ� ���̴�.

	if (m_ImpactParticle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), m_ImpactParticle, GetActorTransform());
	}

	if (m_ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, m_ImpactSound, GetActorLocation());
	}
	*/

	
	// AProjectile Actor �� Destroy �� ���̴�.
	// ���� : Server ����"��" Destroy�ص� �ش� ������ Propagate �Ǿ �ٸ� Ŭ���̾�Ʈ������ ������� �ȴ�.
	// OnHit ��� �Լ��� ���������� ����ǰ� �صξ��� (���� �ּ��� ����)
	Destroy();
}

// Called every frame
void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called When Actor Destroyed During GamePlay, Or In the Editor
// - If Called From Server -> Replicated To Client -> Also Actor Destoryed From Client
void AProjectile::Destroyed()
{
	Super::Destroyed();

	// ���� �Լ��� ���� �ڵ�� ��� ��迡�� ����ȴ�.
	if (m_ImpactParticle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), m_ImpactParticle, GetActorTransform());
	}

	if (m_ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, m_ImpactSound, GetActorLocation());
	}

}

