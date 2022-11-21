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

	// Server 에서 Spawn 되면, 그러한 Spawn Action 이 클라이언트들에게 Replicate 되는 것 (즉, 클라이언트 측에서도 생성)
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
	
	// 1_1) Projectile 은 Character 와는 충돌해야 한다. 정확하게는 Character 의 Mesh Component 와 충돌해야 하는데
	// Mesh Component 의 ObjectType 이 Pawn 이므로, Pawn 과 충돌하게 해야 한다.
	// m_CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);

	// 1_2) 하지만, Character 의 경우, Root Component 이 Capsule Component 또한 Object Type 이 Pawn 이다
	// 우리는 정확하게 Mesh 를 때렸을 때만 적용시키고 싶다.
	// 따라서 Custom Collision Channel 을 만들어서 Character 의 Mesh 에 적용시키고
	// 해당 Custom Channel 에 대해서 Block 이 되도록 세팅할 것이다.
	m_CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECollisionResponse::ECR_Block);

	m_ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(
		TEXT("ProjectileMovmentComponent"));
	// Bullet Keep Rotation Along With Velocity
	// ex) 중력에 의해 떨어지면 Root Component 의 Rotation 또한 변하게 될 것이다.
	m_ProjectileMovementComponent->bRotationFollowsVelocity = true;

	// 이건 선택사항 -> ProjectileMovement Speed 설정하기
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
	이렇게 하면 Server 측에서만 생성된다.
	즉, 설령 Client측에서 총을 쏴도, 충돌 시에 Particle 및 Sound 는 Server 에서만 보이게 된다
	왜냐하면 현재 RPC, FNetVector_Quantize 등을 통해서, Client측에서 Projecttile 을 생성하면
	서버, 클라이언트 모두에서 생성될 수 있게 했다.

	단, BeginPlay() 에서 Server 일 때만 해당 Overlap Event 함수가 호출되게 했으므로
	서버측에서만 보이게 되는 것이다.

	if (m_ImpactParticle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), m_ImpactParticle, GetActorTransform());
	}

	if (m_ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, m_ImpactSound, GetActorLocation());
	}
	*/

	
	// AProjectile Actor 를 Destroy 할 것이다.
	// 참고 : Server 에서"만" Destroy해도 해당 정보가 Propagate 되어서 다른 클라이언트에서도 사라지게 된다.
	// OnHit 라는 함수는 서버에서만 실행되게 해두었다 (위에 주석글 참고)
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

	// 여기 함수에 넣은 코드는 모든 기계에서 실행된다.
	if (m_ImpactParticle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), m_ImpactParticle, GetActorTransform());
	}

	if (m_ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, m_ImpactSound, GetActorLocation());
	}

}

