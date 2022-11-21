// Fill out your copyright notice in the Description page of Project Settings.

#include "ProjectileBullet.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	// ProjectTileWeapon ���� Projectile �� ���� ��, SpawnParams.Investigator �׸��� �����صд�.
	// �װ��� Ȱ���� ���̴�.
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());

	if (OwnerCharacter)
	{
		AController* OwnerController = OwnerCharacter->Controller;

		if (OwnerController)
		{
			// Trigger Damage Event
			// - Nothing Will happen unless we bind callback to damage event
			// - BlasterCharacter::ReceiveDamage Class �� bind ��Ŵ (but Server Only)
			UGameplayStatics::ApplyDamage(OtherActor, m_Damage, OwnerController, this, UDamageType::StaticClass());
		}
	}


	// �θ� �Լ������� Projectile �� Destroy ��Ų��. ���� AProjectileBullet ������ ������ �۾���, ������ ���� ó��
	// ��, �Ʒ� �Լ��� ���� �������� ����
	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}