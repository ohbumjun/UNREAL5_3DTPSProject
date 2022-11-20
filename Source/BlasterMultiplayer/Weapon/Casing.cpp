// Fill out your copyright notice in the Description page of Project Settings.

#include "Casing.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

// ACasing Not Replicated
// Allow Spawning ACasing Actor Locally
ACasing::ACasing()
{
	PrimaryActorTick.bCanEverTick = false;

	m_CasingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasingMesh"));
	SetRootComponent(m_CasingMesh);

	m_CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera,
		ECollisionResponse::ECR_Ignore);

	// Enable Physics
	m_CasingMesh->SetSimulatePhysics(true);

	m_CasingMesh->SetEnableGravity(true);

	// SetSimulatePhysics(true) 를 true 로 세팅한 상황에서 주의할 점이 있다.
	// Physics 가 Enable 된 상태에서도 Hit Event 를 동작시킬 수 있게 해야 한다.
	// 즉, 무언가 부딪히면 Physics Simulation 이 HitEvent 를 발생시킬 수 있게 해야 한다는 것이다.
	m_CasingMesh->SetNotifyRigidBodyCollision(true);

	m_ShellEjectionImpulse = 10.f;
}

void ACasing::BeginPlay()
{
	Super::BeginPlay();

	m_CasingMesh->OnComponentHit.AddDynamic(this, &ACasing::OnHit);

	// Bullet 이 벽에 부딪히면, 튕겨나가는 등의 Physics 를 적용하게 될 수 있다.
	m_CasingMesh->AddImpulse(GetActorForwardVector() * m_ShellEjectionImpulse);

}

void ACasing::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Sound That Bullet Hit The Ground
	if (m_ShellSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, m_ShellSound, GetActorLocation());
	}

	Destroy();
}

// Called every frame
void ACasing::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

