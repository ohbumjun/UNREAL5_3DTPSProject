// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

UCLASS()
class BLASTERMULTIPLAYER_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	AProjectile();

protected:
	virtual void BeginPlay() override;

	// Generated When Components Hit Sth
	// Callbacks Bound To Hit Or Overlap Events Have to have UFUNCTION Macro
	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		FVector NormalImpulse, const FHitResult& Hit);

public:	
	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;

private  :
	UPROPERTY(EditAnywhere)
	class UBoxComponent* m_CollisionBox;

	UPROPERTY(VisibleAnywhere)
		class UProjectileMovementComponent* m_ProjectileMovementComponent;

	// Spawn Particle System For Bullet
	// Blueprint 상에서 Setting 해도 된다.
	UPROPERTY(EditAnywhere)
	class UParticleSystem* m_Tracer;

	// Tracer 를 생성하면 해당 멤버변수에 UParticleSystemComponent 세팅
	class UParticleSystemComponent* m_TracerComponent;

	// Particle When Hit
	UPROPERTY(EditAnywhere)
	class UParticleSystem* m_ImpactParticle;

	UPROPERTY(EditAnywhere)
		class USoundCue* m_ImpactSound;
};
