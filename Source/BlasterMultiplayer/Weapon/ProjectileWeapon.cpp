// Fill out your copyright notice in the Description page of Project Settings.

#include "ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Projectile.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	APawn* ProjectTileInstigator = Cast<APawn>(GetOwner());

	// 우리가 사용하는 Gun Skeletal Mesh 에 MuzzleFlash 라는 Socket 을 달아야 한다.
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));

	if (MuzzleFlashSocket)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());

		// Projectile 의 방향 세팅 (From MuzzleFlash Socket to HitTarget From TraceUnderCrosshairs)
		FVector ToTarget = HitTarget - SocketTransform.GetLocation();

		FRotator TargetRotation = ToTarget.Rotation();

		// Spawn Actor Based On Projectile Class
		if (m_ProjectTileClass && ProjectTileInstigator)
		{
			FActorSpawnParameters SpawnParams;

			// 아래에서 새롭게 만든 Actor 의 Owner 가 해당 값으로 세팅된다.
			// 현재 우리는 Weapon 을 Equip 할대 Owner 를 세팅해주고 있다.
			// ex) UCombatComponent::EquipWeapon
			// 따라서 현재 Fire 하고 있는 Character 를 Projectile 의 Owner 로 세팅해줄 것이다.
			SpawnParams.Owner = GetOwner(); // Get Owner Of Weapon
			SpawnParams.Instigator = ProjectTileInstigator;

			UWorld* World = GetWorld();

			if (World)
			{
				World->SpawnActor<AProjectile>(
					m_ProjectTileClass,
					// Spawn At "MuzzleFlash" Socket
					SocketTransform.GetLocation(),
					TargetRotation,
					SpawnParams
					);
			}
		}
	}

}
