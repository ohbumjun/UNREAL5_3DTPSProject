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
	// 바로 위 부모 클래스에서 UFUNCTION 매크로를 붙여줬기 때문에 여기에는 안붙여도 된다.
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		FVector NormalImpulse, const FHitResult& Hit);
};
