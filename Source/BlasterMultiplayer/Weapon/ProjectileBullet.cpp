// Fill out your copyright notice in the Description page of Project Settings.

#include "ProjectileBullet.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	// ProjectTileWeapon 에서 Projectile 을 만들어낼 때, SpawnParams.Investigator 항목을 세팅해둔다.
	// 그것을 활용할 것이다.
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());

	if (OwnerCharacter)
	{
		AController* OwnerController = OwnerCharacter->Controller;

		if (OwnerController)
		{
			// Trigger Damage Event
			// - Nothing Will happen unless we bind callback to damage event
			// - BlasterCharacter::ReceiveDamage Class 를 bind 시킴 (but Server Only)
			UGameplayStatics::ApplyDamage(OtherActor, m_Damage, OwnerController, this, UDamageType::StaticClass());
		}
	}


	// 부모 함수에서는 Projectile 을 Destroy 시킨다. 따라서 AProjectileBullet 에서의 고유한 작업은, 위에서 먼저 처리
	// 즉, 아래 함수를 가장 마지막에 실행
	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}