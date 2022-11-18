// Fill out your copyright notice in the Description page of Project Settings.

#include "Projectile.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Gameframework/ProjectileMovementComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"

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

	m_ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(
		TEXT("ProjectileMovmentComponent"));
	// Bullet Keep Rotation Along With Velocity
	// ex) �߷¿� ���� �������� Root Component �� Rotation ���� ���ϰ� �� ���̴�.
	m_ProjectileMovementComponent->bRotationFollowsVelocity = true;
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

