// Fill out your copyright notice in the Description page of Project Settings.

#include "ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Projectile.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	// 오로지 서버에 존재하는 Weapon 으로부터만 ProjectTile 이 만들어지게 한다.
	if (HasAuthority() == false)
		return;

	// 현재 Weapon 의 생성자를 보면 bReplicates = true 로 세팅되어 있다.
	// 만약 이 변수가 false 라면, 즉, replicate 되지 않는 것이라면, 모든 기계에서
	// 해당 Actor 에 대한 Authority 를 가지고 있다는 의미가 된다.
	// 즉, 모든 기계에서 각각 서버와 독립적으로 Spawn되고 존재한다는 것이다.
	// 하지만, Replicate 된다는 것은, Server 에서 Spawn 된다는 것을 의미한다.
	// 그리고 이러한 Spawning Action 이 Client 들에게 Propagate 되고
	// Server 만이 Authority 를 가지게 된다는 것이다. 
	// 마찬가지로 파생클래스인 AProjectileWeapon 또한 Server 에서만 Authority 를 가지고 있게 된다.
	// 따라서 마찬가지로 해당 Projectile 이 Server 에서만 Spawn 될 수 있게 하고 싶은 것이다.
	// 즉, 이 과정까지만 진행하면, Server 에서 총을 쏠 때만 아래의 총알 Projectile 이 보이게 되는 것
	// Client 에서 쏘면 생성이 안된다는 것이다.
	// 또한 Server 에서 쏘더라도, Server 에서는 해당 ProjectTile 이 보이겠지만
	// 클라이언트 측에서는 안보이게 될 것이다.
	// Replicate 해야 한다.
	// 이를 위해서는 ProjectTile 의 생성자에서 bReplicates = true 를 세팅해줘야 한다.
	// 그러면 Server 에서 ProjectTile 을 생성하면, 해당 생성 Action 이 Replicate 되어
	// 다른 Client  들에서도 생성되게 될 것이다.

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
