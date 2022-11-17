// Fill out your copyright notice in the Description page of Project Settings.

#include "ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Projectile.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	APawn* ProjectTileInstigator = Cast<APawn>(GetOwner());

	// �츮�� ����ϴ� Gun Skeletal Mesh �� MuzzleFlash ��� Socket �� �޾ƾ� �Ѵ�.
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));

	if (MuzzleFlashSocket)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());

		// Projectile �� ���� ���� (From MuzzleFlash Socket to HitTarget From TraceUnderCrosshairs)
		FVector ToTarget = HitTarget - SocketTransform.GetLocation();

		FRotator TargetRotation = ToTarget.Rotation();

		// Spawn Actor Based On Projectile Class
		if (m_ProjectTileClass && ProjectTileInstigator)
		{
			FActorSpawnParameters SpawnParams;

			// �Ʒ����� ���Ӱ� ���� Actor �� Owner �� �ش� ������ ���õȴ�.
			// ���� �츮�� Weapon �� Equip �Ҵ� Owner �� �������ְ� �ִ�.
			// ex) UCombatComponent::EquipWeapon
			// ���� ���� Fire �ϰ� �ִ� Character �� Projectile �� Owner �� �������� ���̴�.
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
