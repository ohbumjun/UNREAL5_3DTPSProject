// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"
#include "Components/SphereComponent.h"
#include "Camera/CameraComponent.h"
#include "../Weapon/Weapon.h"
#include "../Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Net/UnrealNetwork.h" // Replicate Variable
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "../PlayerController/BlasterPlayerController.h"
#include "TimerManager.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true; // For Debuggin Weapon Hit Point

	m_BaseWalkSpeed = 600.f;
	m_AimWalkSpeed = 450.f;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, m_EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, m_bAiming);
}

// Called when the game starts
void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	// SetComponentTickEnabled(true);
	
	if (m_BlasterCharacter)
	{
		// ���� : Crouch �� ���� CharacterMovementComponent �󿡼� MaxWalkSpeed �� �ƴ϶� Crouch Walk Speed ������ ���
		// ���� �ش� ���� Crouch �Ҷ��� ������� �ʴ´�.
		m_BlasterCharacter->GetCharacterMovement()->MaxWalkSpeed = m_BaseWalkSpeed;

		if (m_BlasterCharacter->GetFollowCamera())
		{
			m_DefaultFOV = m_BlasterCharacter->GetFollowCamera()->FieldOfView;
			m_CurrentFOV = m_DefaultFOV;
		}
	}


}

// Called every frame
void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (m_BlasterCharacter && m_BlasterCharacter->IsLocallyControlled())
	{
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		m_HitTarget = HitResult.ImpactPoint;

		SetHUDCrosshairs(DeltaTime);

		InterpFOV(DeltaTime);
	}
}

// Call From Blaster Charcter (Can be Called From Client And Server All)
void UCombatComponent::FireButtonPressed(bool bPressed)
{
	m_FireButtonPressed = bPressed;

	// �̸� ���� ���� ������ ServerFire_Implementation �� ������ ���̴�
	// Ŭ���̾�Ʈ���� ȣ���ص� �������� ����
	// �������� ȣ���ص� �������� ����
	// ���⼭ �߿��� ���� �ִ�. �̷��� ServerRPC �� ȣ���ϸ�, ������ Server ������ �ȴ�.
	// ��, ���� Client ������ ���������� Fire ��ư�� ������ Server �� �ش��ϴ� Machine ������ Fire ����� ���δ�.
	// ����, m_FireButtonPressed �� replicate �ϰ�, Rap_Notify �� �����ϸ� �ذ��� �� �ִ�.

	// ������, �̰��� �ϰ� ������ �ʴ�.
	// �츮�� ����ؼ� Fire Button �� ������ ���� �� �ִ� (Left Mouse Button)
	// �׷��� m_FireButtonPressed �� ��� true �� �������� ���̴�. ������ �ʴ´ٴ� ���̴�.
	// �׷��� Replication �� �ش� ������ ���� ���� �����Ѵ�.
	// �׷��� Server �������� ��� Fire �� �ϰ� �־, �ش� ���� ������ REplicate ���� �ʾƼ�
	// �ٸ� Client �鿡�� ���������� ������ ���޵��� �ʴ´ٴ� ���̴�.

	// ���� Client �鿡�� ���� �Ͼ�� ������, �ٸ� ������� ��������� �Ѵ�.
	// �̴� Multicast RPC �� ���� �� �� �ִ�.
	// �������� Multicast RPC  �� ȣ���ϸ�, ������ Ŭ���̾�Ʈ ��ο��� �����ϰ� �ȴٴ� ���̴�.
	// Ŭ���̾�Ʈ���� ȣ���ϸ� Invoking Client ������ ����ȴ�. ���� ��� Ŭ���̾�Ʈ�� BroadCast ���ַ���
	// ��ǻ� �������� Multicast RPC �� ȣ�����־�� �Ѵ�.
	if (m_FireButtonPressed && m_EquippedWeapon)
	{
		Fire();
	}
}

void UCombatComponent::StartFireTimer()
{
	UE_LOG(LogTemp, Warning, TEXT("StartFireTimer"))

		if (m_EquippedWeapon == nullptr || m_BlasterCharacter == nullptr)
			return;

	m_BlasterCharacter->GetWorldTimerManager().SetTimer(
		m_FireTimer,
		this,
		&UCombatComponent::FireTimerFinished,
		m_EquippedWeapon->m_FireDelay
	);
}

void UCombatComponent::FireTimerFinished()
{
	UE_LOG(LogTemp, Warning, TEXT("FireTimerFinished"))

		if (m_EquippedWeapon == nullptr)
			return;

	m_bCanFire = true;

	if (m_FireButtonPressed && m_EquippedWeapon->m_bAutomatic)
	{
		Fire();
	}
}

void UCombatComponent::Fire()
{
	UE_LOG(LogTemp, Warning, TEXT("Fire Function"));

	if (m_bCanFire)
	{
		UE_LOG(LogTemp, Warning, TEXT("Fire Function Yes ! m_bCanFire = true"));

		ServerFire(m_HitTarget);

		// Affect Cross Hair Shooting Factor
		if (m_EquippedWeapon)
		{
			m_CrosshairShootingFactor = 0.6f;

			// FireTimer �� Finish �� �������� false �� �����ϰ� �� ���̴�.
			m_bCanFire = false;
		}

		// Automatic fire �� true �� ���
		// ����ؼ� Fire Button �� ������ �ִٸ�, ���������� �ٽ� Fire �� �� �� �ְ� �ϱ� ����
		StartFireTimer();
	}
}


void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if (m_EquippedWeapon == nullptr)
		return;

	// Called From Client, -> Execute By Server
	// Play Fire Montage (1. Not Aiming, 2. Aiming)
	// if (m_BlasterCharacter && m_FireButtonPressed) : m_FireButtonPressed ������ �ʿ����.
	// �ֳ��ϸ� m_FireButtonPressed ��� ������ Client ������ �����ϴ� ������. ������ ���� �Լ��� �������� ����

	if (m_BlasterCharacter)
	{
		// m_bAiming : GetLifetimeReplicatedProps() ���� Replicated Variable �� ����ص� ����
		// ���� ���������� Ŭ���̾�Ʈ ������ m_bAiming �� ���� ������ ���޵� ���̴�.
		m_BlasterCharacter->PlayFireMontage(m_bAiming);

		// Fire Weapon -> Play Animation For Weapon Itself In It
		// m_EquippedWeapon->Fire(m_HitTarget);

		// TraceHitTarget : HitTarget That Is Being BroadCasted 
		// ex) Client Calculate TraceHitTarget -> RPC -> Pass In TraceHitTarget -> Execute All On Server, Client 
		m_EquippedWeapon->Fire(TraceHitTarget); 
	}
}

// ��. �ش� �Լ��� Server ������"��" ȣ��ȴ�.
// ���� Multicast RPC �� �������� "ȣ��" �ϸ� ������ Ŭ���̾�Ʈ ��ο��� ����ȴ�.
// ��, ���������� Server ���� ������ Broad Cast �ȴٴ� ���̴�.

// ���� : Client -> Server RPC �� ServerFire ȣ�� -> ServerFire_Implementation �� �������� ���� ->
//       -> Multicast RPC �� Server ���� ȣ�� -> MulticastFire_Implementation �� ��� Client, �������� ����
void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	MulticastFire(TraceHitTarget);
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (m_BlasterCharacter == nullptr || WeaponToEquip == nullptr)
		return;

	// Attach Weapon to Socket On Skeleton 
	// (Bluepint �� ���ؼ� Mesh �� ã�� ���ϴ� ��ġ�� Socket �߰��ϱ� (46��)
	m_EquippedWeapon = WeaponToEquip;

	// ��. �츮�� CombatComponent �� Replicate �ǵ��� �����ߴ� (BlasterCharacter ������ ����)
	// �������� ���� ���� �������� ���Ӱ� �����ϱ� ������, �ش� �������� Client ������ Replicate �� ���̴�.
	// ������, m_EquippedWeapon ���� ���� �ȵȴ�.
	// ���� m_EquippedWeapon ������ Ư�� Property �鵵 Replicate �ǵ��� �����ؾ� �Ѵ�.
	// ���� �̸� ���ؼ� Variable Replication �� ������ ���̴�.
	// - m_EquippedWeapon ������ Replicate �ǵ��� �Ѵ�.
	// - �̰��� ������ ������ Weapon �� Replicate �� ������ Actor �� �����ص� �����̱� �����̴�
	// - ������ : bReplicates = true; 
	m_EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

	const USkeletalMeshSocket* HandSocket = m_BlasterCharacter->GetMesh()->GetSocketByName(FName("RightHandSocket"));

	if (HandSocket)
	{
		// Actor �� Attach �ϴ� ������ �ڵ����� ���������� replicate �ǵ��� �����Ǿ� �ִ�.
		// ���⼭ �߿��� ����, 1) m_EquippedWeapon �� 2) Attaching Actor �ϴ� �������� ���� Replicate �ȴٴ�
		// ������ ����. Network ��Ȳ� ���� �ٸ��� ����� ���� �ֱ� �����̴�.
		// m_EquippedWeapon �� m_WeaponState �� ���� �����ؾ� �ϴ� ������ Collision �����̴�.
		// ��ü������ SetPhysics() �� ���õ� �Լ� �����ε�
		// ������ EWS_Equipped �� m_EquippedWeapon ���� m_WeaponState �� �����ؾ߸�
		// SetPhysics(false) �� ���õǰ�, �׶���μ� Attaching Actor ������ ���������� �����̴�.
		HandSocket->AttachActor(m_EquippedWeapon, m_BlasterCharacter->GetMesh());
	}

	// Set Owner
	// SetOwner() ��� ������ Replicated �ǵ��� �̹� �����Ǿ� �ִ�. 
	// ���� Client ������ Replicate �� ���̴�.
	m_EquippedWeapon->SetOwner(m_BlasterCharacter);

	// MovementComponent �� World���� Rotation �� �ƴ϶�
	// PlayerController �� World ���� Rotation ������ ����� ���̴�. ���콺...? (���� 51. 52) 
	// �ֳ��ϸ� �÷��̻�, ���콺�� �ٶ󺸴� �������� ���� ��� ���� ���콺 ����� �÷��̾ �ٶ󺸴� ������ ��ġ���Ѿ� �ϱ� ����
	// �翬�� �Ʒ��� ������ ���ؼ��� Replication �� ��������� �Ѵ�. OnRep_EquippedWeapon �� ���� ������ ���̴�.
	m_BlasterCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;

	m_BlasterCharacter->bUseControllerRotationYaw = true;
}

// Rap Notify �̴�.
// �ش� �Լ��� ȣ��Ǿ��ٴ� ���� m_EquippedWeapon �� ��ȭ�� �߻��ߴٴ� ��
// ��, ���⸦ �����߰ų� ���ȴٴ� ��
void UCombatComponent::OnRep_EquippedWeapon()
{
	if (m_EquippedWeapon && m_BlasterCharacter)
	{
		// ��. Replicate �Լ� ���� ������, Client ���� �ش� �Լ��� ȣ�����ִ� ���̰�
		// ������ Network ��Ȳ�� �������, ���������� �ڵ� ������ ����� ���̴�.
		// ��, SetWeaponState(EWeaponState::EWS_Equipped); �� ���ؼ� Physics ������ �и��� �ٲ�ٴ� ���̴�.
		m_EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

		const USkeletalMeshSocket* HandSocket = m_BlasterCharacter->GetMesh()->GetSocketByName(FName("RightHandSocket"));

		if (HandSocket)
		{
			HandSocket->AttachActor(m_EquippedWeapon, m_BlasterCharacter->GetMesh());
		}

		m_BlasterCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;

		m_BlasterCharacter->bUseControllerRotationYaw = true;
	}
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	// trace From Center Of Screen -> Need ViewPort Size
	FVector2D ViewPortSize;

	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewPortSize);
	}

	// Crosshair Location = Center Of ViewPort
	FVector2D CrosshairLocation(ViewPortSize.X / 2.f, ViewPortSize.Y / 2.f);

	// Upper Result : Screen Space -> Need To Convert To World Space
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;

	// �� Machine ���� Player0 => Player Who Is Controlling The Pawn (��, �ڱ� �ڽ�)
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection // Unit Vector
	);

	// Successful Got World Pos Of Screen Center, Direction
	if (bScreenToWorld)
	{
		// Start Pos Line Trace
		FVector Start = CrosshairWorldPosition;

		// ���� LineTrace ��ġ��, ĳ���� ��ġ ���ʿ��� �����ؾ� �Ѵ�.
		// �׷��� �⺻������ Camera ��, ���� ĳ���ͺ��� ���ʿ��� �ٶ󺸰� �ִ�.
		// ���� ���� ��ġ�� Character �������� �������־�� �Ѵ�. (100.f �������� �������� ���̴�)
		if (m_BlasterCharacter)
		{
			float DistanceToCharacter = (m_BlasterCharacter->GetActorLocation() - Start).Size();

			Start += CrosshairWorldDirection * (DistanceToCharacter + 100.f);

			// DrawDebugSphere(GetWorld(), Start, 16.f, 12, FColor::Red, false);
		}

		// Move Foward By Enough Distances
		FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH;

		// TraceHit : Filled In By Below Function
		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			Start,
			End,
			ECollisionChannel::ECC_Visibility
		);

		if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>())
		{
			m_HUDPackage.CrosshairsColor = FLinearColor::Red;
		}
		else
		{
			m_HUDPackage.CrosshairsColor = FLinearColor::White;
		}

		// No Blocking Hit
		if (TraceHitResult.bBlockingHit == false)
		{
			TraceHitResult.ImpactPoint = End;

			// m_HitTarget = End;
		}
		else
		{
			// Working
			// Draw Sphere
			// DrawDebugSphere(
			// 	GetWorld(),
			// 	TraceHitResult.ImpactPoint,
			// 	12.f,
			// 	12.f,
			// 	FColor::Red,
			// 	false, // Draw Debug Sphere Every Frame
			// 	-1.f   // Draw Debug Sphere Every Frame
			// );

			// HitTarget ������ ����
			// m_HitTarget = TraceHitResult.ImpactPoint;
		}
	}
}

// Called Every Frame
void UCombatComponent::SetHUDCrosshairs(float DeltaTime)
{
	// Access HUD by PlayerController
	if (m_BlasterCharacter == nullptr)
		return;

	m_Controller = m_Controller == nullptr ? Cast<ABlasterPlayerController>(m_BlasterCharacter->Controller) : m_Controller;

	if (m_Controller)
	{
		// Set HUD
		m_HUD = m_HUD == nullptr ? Cast<ABlasterHUD>(m_Controller->GetHUD()) : m_HUD;

		if (m_HUD)
		{

			if (m_EquippedWeapon)
			{
				// Create FHUD Package With CrossHair Textures & Set It To BlasterCharacter
				m_HUDPackage.CrosshairsCenter = m_EquippedWeapon->m_CrosshairCenter;
				m_HUDPackage.CrosshairsLeft = m_EquippedWeapon->m_CrosshairLeft;
				m_HUDPackage.CrosshairsRight = m_EquippedWeapon->m_CrosshairRight;
				m_HUDPackage.CrosshairsTop = m_EquippedWeapon->m_CrosshairTop;
				m_HUDPackage.CrosshairsBottom = m_EquippedWeapon->m_CrosshairBottom;
			}
			else
			{
				// If No Weapon Equipped, We will have no crosshairs
				m_HUDPackage.CrosshairsCenter = nullptr;
				m_HUDPackage.CrosshairsLeft = nullptr;
				m_HUDPackage.CrosshairsRight = nullptr;
				m_HUDPackage.CrosshairsTop = nullptr;
				m_HUDPackage.CrosshairsBottom = nullptr;
			}

			// Calculate Crosshairs Spread
			// - Charcter �� Speed �� ���� ũ�Ⱑ ���ݾ� �޶����� �� ���̴�
			// [0, 600] -> [0, 1] (�ִ� �ӵ��� 600�̶�� �Ѵٸ�, ������ ���� 0�� 1 ������ mapping ��ų ���̴�)
			FVector2D WalkSpeedRange(0.f, m_BlasterCharacter->GetCharacterMovement()->MaxWalkSpeed);
			FVector2D VelocityMultiplierRange(0, 1.f);

			FVector Velocity = m_BlasterCharacter->GetVelocity();
			Velocity.Z = 0.f;

			m_CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());

			// In Air ->Spread More Slowly -> Interpolate Using DeltaTime
			if (m_BlasterCharacter->GetCharacterMovement()->IsFalling())
			{
				m_CrosshairInAirFactor = FMath::FInterpTo(m_CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
			}
			else
			{
				// when hit ground -> interpolate to 0 with much faster 
				m_CrosshairInAirFactor = FMath::FInterpTo(m_CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
			}

			if (m_bAiming)
			{
				// Shrink or Spread Crosshair HUD When Aiming
				m_CrosshairAimFactor = FMath::FInterpTo(m_CrosshairAimFactor, 0.58f, DeltaTime, 30.f);
			}
			else
			{
				m_CrosshairAimFactor = FMath::FInterpTo(m_CrosshairAimFactor, 0.f, DeltaTime, 30.f);
			}

			// Shooting �� �Ҷ�, FireButtonPressed ���� m_CrosshairShootingFactor �� 0.6f �� ���õǰ� �ִ�.
			// �׷��� �ٽ� �׻� 0���� ���ƿ��� �� ���̴�. ��, ���� �� ���� Ȯ�� �Ǿ��ٰ�, �ݹ� �ٽ� ���ƿ��� �ϱ�
			m_CrosshairShootingFactor = FMath::FInterpTo(m_CrosshairShootingFactor, 0.f, DeltaTime, 40.f);

			// 0.3f : BaseLine Spread
			m_HUDPackage.CrosshairSpread = 0.3f +
				m_CrosshairVelocityFactor +
				m_CrosshairInAirFactor +
				-1.f * m_CrosshairAimFactor +
				m_CrosshairShootingFactor;

			m_HUD->SetHUDPackage(m_HUDPackage);


		}
	}
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (m_EquippedWeapon == nullptr)
		return;

	if (m_bAiming)
	{
		m_CurrentFOV = FMath::FInterpTo(m_CurrentFOV, m_EquippedWeapon->GetZoomedFOV(),
			DeltaTime, m_EquippedWeapon->GetZoomedInterpSpeed());
	}
	else
	{
		// Interp Back To Default FOV , at Default Speed
		m_CurrentFOV = FMath::FInterpTo(m_CurrentFOV, m_DefaultFOV, DeltaTime,
			m_ZoomInterpSpeed);
	}

	if (m_BlasterCharacter && m_BlasterCharacter->GetFollowCamera())
	{
		m_BlasterCharacter->GetFollowCamera()->SetFieldOfView(m_CurrentFOV);
	}
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	// ���⼭ �ٷ� ���� �������ִ� ����
	// �Ϲ������� RPC�� Delay �� ���� �� �ִ�. ��, ���� ���콺�� ������ ������, RPC �� ���ļ� �̸� ���� ���� ���� ����Ǵ� �� ������
	// �ð� ���̰� ������ ���� �ִ�.
	// �ڵ������ ���� �ϸ� �Ʒ��� �ڵ尡 ServerSetAiming_Implementation() ��� RPC �� �����ϱ⵵ ����
	// ����� �� �ִٴ� ���̴�.
	// �̰��� �츮�� ��Ȯ�� ���ϴ� ���̴�. �츮�� ��Ʈ��ũ �ӵ��� ������� �켱��, �ش� ��ư�� ����
	// Ŭ���̾�Ʈ�� ������ local ������ �ٷ� ��ȭ�� �ֱ⸦ ���ϱ� �����̴�.
	// ���� �ش� ���� ���� ������ Ŭ���̾�Ʈ ���忡���� SetAiming �Լ��� ȣ���ϰ�, RPC ȣ�� -> replicate �� ������ ��ٸ��� �� ���̴�.
	m_bAiming = bIsAiming;

	/* 1st Version

	// ������ �ƴ϶��, RPC ȣ��
	if (m_BlasterCharacter->HasAuthority() == false)
	{
		ServerSetAiming(bIsAiming);
	}
	// Server ������ ���ٸ� ��ġ�� ������ �ʴ´�.
	// if �� ���� �ڵ忡�� m_bAiming = bIsAiming; ��� �ٷ� �����صװ�
	// �ش� ������ replicated ������ ����߱� ������ �����ٴ� ���̴�.

	*/

	// 2nd Version
	// Unreal Document �� �����ϸ�, Server ������, Server RPC �� ȣ���ϸ� �ڵ����� Server ���� �ش� �ڵ尡 ����ȴ�.
	// ���������� Ŭ���̾�Ʈ ������ , Server RPC �� ȣ���ص� Server ���� �ش� �ڵ尡 ����ȴ�.
	// �׷���, ��ǻ� if ���� ���ؼ� �б⹮�� �ۼ��� �ʿ䰡 ���ٴ� ���̴�.
	// m_bAiming �� replicate �� �ξ��� ������, � ��Ȳ������ ��� Ŭ���̾�Ʈ ������ ��������� �ݿ��� ���̴�
	ServerSetAiming(bIsAiming);

	// �̵� �ӵ� ���̱� (�ش� �Լ��� ����, Ŭ���̾�Ʈ���� ��� ȣ��Ǵ� �Լ��̴�)
	if (m_BlasterCharacter)
	{
		m_BlasterCharacter->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? m_AimWalkSpeed : m_BaseWalkSpeed;
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	m_bAiming = bIsAiming;

	// GetCharacterMovement()->MaxWalkSpeed �� ���, GetCharacterMovement() �� �ش��ϴ� ������Ʈ�� 
	// ��� ������ MaxWalkSpeed�� �����Ϸ��� ������ �ִ�.
	// ������ �̷��� Server RPC �� ���ؼ� ȣ�����ָ�, ���������ְ�, �츮�� ���ϴ� ������ Update �����ش�.
	if (m_BlasterCharacter)
	{
		m_BlasterCharacter->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? m_AimWalkSpeed : m_BaseWalkSpeed;
	}
}
