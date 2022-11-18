// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"
#include "Components/SphereComponent.h"
#include "../Weapon/Weapon.h"
#include "../Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Net/UnrealNetwork.h" // Replicate Variable
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

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

	SetComponentTickEnabled(true);
	
	// ���� : Crouch �� ���� CharacterMovementComponent �󿡼� MaxWalkSpeed �� �ƴ϶� Crouch Walk Speed ������ ���
	// ���� �ش� ���� Crouch �Ҷ��� ������� �ʴ´�.
	m_BlasterCharacter->GetCharacterMovement()->MaxWalkSpeed = m_BaseWalkSpeed;
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

// Call From Blaster Charcter
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
		ServerFire();
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

			m_HitTarget = End;
		}
		else
		{
			// Working
			// Draw Sphere
			DrawDebugSphere(
				GetWorld(),
				TraceHitResult.ImpactPoint,
				12.f,
				12.f,
				FColor::Red,
				false, // Draw Debug Sphere Every Frame
				-1.f   // Draw Debug Sphere Every Frame
			);

			// HitTarget ������ ����
			m_HitTarget = TraceHitResult.ImpactPoint;
		}
	}
}

void UCombatComponent::MulticastFire_Implementation()
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
		m_EquippedWeapon->Fire(m_HitTarget);
	}
}

// ��. �ش� �Լ��� Server ������"��" ȣ��ȴ�.
// ���� Multicast RPC �� �������� "ȣ��" �ϸ� ������ Ŭ���̾�Ʈ ��ο��� ����ȴ�.
// ��, ���������� Server ���� ������ Broad Cast �ȴٴ� ���̴�.

// ���� : Client -> Server RPC �� ServerFire ȣ�� -> ServerFire_Implementation �� �������� ���� ->
//       -> Multicast RPC �� Server ���� ȣ�� -> MulticastFire_Implementation �� ��� Client, �������� ����
void UCombatComponent::ServerFire_Implementation()
{
	MulticastFire();
}

// Called every frame
void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	FHitResult HitResult;

	TraceUnderCrosshairs(HitResult);

	// UE_LOG(LogTemp, Warning, TEXT("Tick Component"));
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

