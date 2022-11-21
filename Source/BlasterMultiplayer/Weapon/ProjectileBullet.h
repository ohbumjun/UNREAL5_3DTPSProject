// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "ProjectileBullet.generated.h"

/**
 * 
 */
UCLASS()
class BLASTERMULTIPLAYER_API AProjectileBullet : public AProjectile
{
	GENERATED_BODY()
	
protected :
	// �ٷ� �� �θ� Ŭ�������� UFUNCTION ��ũ�θ� �ٿ���� ������ ���⿡�� �Ⱥٿ��� �ȴ�.
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		FVector NormalImpulse, const FHitResult& Hit);
};
