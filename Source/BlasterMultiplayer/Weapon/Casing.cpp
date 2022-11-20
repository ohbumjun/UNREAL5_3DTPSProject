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

	// SetSimulatePhysics(true) �� true �� ������ ��Ȳ���� ������ ���� �ִ�.
	// Physics �� Enable �� ���¿����� Hit Event �� ���۽�ų �� �ְ� �ؾ� �Ѵ�.
	// ��, ���� �ε����� Physics Simulation �� HitEvent �� �߻���ų �� �ְ� �ؾ� �Ѵٴ� ���̴�.
	m_CasingMesh->SetNotifyRigidBodyCollision(true);

	m_ShellEjectionImpulse = 10.f;
}

void ACasing::BeginPlay()
{
	Super::BeginPlay();

	m_CasingMesh->OnComponentHit.AddDynamic(this, &ACasing::OnHit);

	// Bullet �� ���� �ε�����, ƨ�ܳ����� ���� Physics �� �����ϰ� �� �� �ִ�.
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

