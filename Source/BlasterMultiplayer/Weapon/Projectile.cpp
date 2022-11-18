// Fill out your copyright notice in the Description page of Project Settings.

#include "Projectile.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Gameframework/ProjectileMovementComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"

// Sets default values
AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

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
	// ex) 중력에 의해 떨어지면 Root Component 의 Rotation 또한 변하게 될 것이다.
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
}

// Called every frame
void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

