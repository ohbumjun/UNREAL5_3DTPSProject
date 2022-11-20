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
#include "../HUD/BlasterHUD.h"

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

// Rap Notify �̴�.
// �ش� �Լ��� ȣ��Ǿ��ٴ� ���� m_EquippedWeapon �� ��ȭ�� �߻��ߴٴ� ��
// ��, ���⸦ �����߰ų� ���ȴٴ� ��
void UCombatComponent::OnRep_EquippedWeapon()
{
	if (m_EquippedWeapon && m_BlasterCharacter)
	{
		m_BlasterCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;

		m_BlasterCharacter->bUseControllerRotationYaw = true;
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
	if (m_FireButtonPressed)
	{
		FHitResult HitResult;

		TraceUnderCrosshairs(HitResult);

		ServerFire(HitResult.ImpactPoint);
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

		// Move Foward By Enough Distances
		FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH;

		// TraceHit : Filled In By Below Function
		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			Start,
			End,
			ECollisionChannel::ECC_Visibility
		);

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
			FHUDPackage HUDPackage;

			if (m_EquippedWeapon)
			{
				// Create FHUD Package With CrossHair Textures & Set It To BlasterCharacter
				HUDPackage.CrosshairsCenter = m_EquippedWeapon->m_CrosshairCenter;
				HUDPackage.CrosshairsLeft = m_EquippedWeapon->m_CrosshairLeft;
				HUDPackage.CrosshairsRight = m_EquippedWeapon->m_CrosshairRight;
				HUDPackage.CrosshairsTop = m_EquippedWeapon->m_CrosshairTop;
				HUDPackage.CrosshairsBottom = m_EquippedWeapon->m_CrosshairBottom;
			}
			else
			{
				// If No Weapon Equipped, We will have no crosshairs
				HUDPackage.CrosshairsCenter = nullptr;
				HUDPackage.CrosshairsLeft = nullptr;
				HUDPackage.CrosshairsRight = nullptr;
				HUDPackage.CrosshairsTop = nullptr;
				HUDPackage.CrosshairsBottom = nullptr;
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

			HUDPackage.CrosshairSpread = m_CrosshairVelocityFactor + m_CrosshairInAirFactor;

			m_HUD->SetHUDPackage(HUDPackage);

			
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
	// - m_WeaponState ������ Replicate �ǵ��� �Ѵ�.
	// - �̰��� ������ ������ Weapon �� Replicate �� ������ Actor �� �����ص� �����̱� �����̴�
	// - ������ : bReplicates = true; 
	m_EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

	const USkeletalMeshSocket* HandSocket = m_BlasterCharacter->GetMesh()->GetSocketByName(FName("RightHandSocket"));

	if (HandSocket)
	{
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

