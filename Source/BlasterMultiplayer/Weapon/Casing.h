// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Casing.generated.h"

// Bullet Shell (ÃÑ¾Ë)
UCLASS()
class BLASTERMULTIPLAYER_API ACasing : public AActor
{
	GENERATED_BODY()
	
public:	
	ACasing();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
		virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
			FVector NormalImpulse, const FHitResult& Hit);

private:
	UPROPERTY(VisibleAnywhere)
		UStaticMeshComponent* m_CasingMesh;

	UPROPERTY(EditAnywhere)
		float m_ShellEjectionImpulse;

	UPROPERTY(EditAnywhere)
	class USoundCue* m_ShellSound;

public:	
	virtual void Tick(float DeltaTime) override;

};
