
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "ProjectileWeapon.generated.h"

/**
 * 
 */
UCLASS()
class BLASTERMULTIPLAYER_API AProjectileWeapon : public AWeapon
{
	GENERATED_BODY()

public :
	virtual void Fire(const FVector& HitTarget) override;

	
private :
	// Spawn Projection
	UPROPERTY(EditAnywhere)
	TSubclassOf<class AProjectile> m_ProjectTileClass;
};
