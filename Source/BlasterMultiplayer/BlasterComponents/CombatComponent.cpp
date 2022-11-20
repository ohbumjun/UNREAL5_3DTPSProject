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
		// 참고 : Crouch 할 때는 CharacterMovementComponent 상에서 MaxWalkSpeed 가 아니라 Crouch Walk Speed 변수를 사용
		// 따라서 해당 값은 Crouch 할때는 적용되지 않는다.
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
	// 여기서 바로 값을 변경해주는 이유
	// 일반적으로 RPC는 Delay 가 생길 수 있다. 즉, 내가 마우스를 누르는 시점과, RPC 를 거쳐서 이를 통해 실제 값이 변경되는 데 까지는
	// 시간 차이가 존재할 수도 있다.
	// 코드상으로 말을 하면 아래의 코드가 ServerSetAiming_Implementation() 라는 RPC 를 실행하기도 전에
	// 실행될 수 있다는 것이다.
	// 이것이 우리가 정확히 원하는 것이다. 우리는 네트워크 속도와 관계없이 우선은, 해당 버튼을 누른
	// 클라이언트던 서버던 local 에서는 바로 변화가 있기를 원하기 때문이다.
	// 만약 해당 줄을 넣지 않으면 클라이언트 입장에서는 SetAiming 함수를 호출하고도, RPC 호출 -> replicate 될 때까지 기다리게 될 것이다.
	m_bAiming = bIsAiming;

	/* 1st Version
	
	// 서버가 아니라면, RPC 호출
	if (m_BlasterCharacter->HasAuthority() == false)
	{
		ServerSetAiming(bIsAiming);
	}
	// Server 에서는 별다른 조치를 취하지 않는다. 
	// if 문 위의 코드에서 m_bAiming = bIsAiming; 라고 바로 세팅해뒀고
	// 해당 변수를 replicated 변수로 등록했기 때문에 괜찮다는 것이다.

	*/
	
	// 2nd Version
	// Unreal Document 를 참고하면, Server 측에서, Server RPC 를 호출하면 자동으로 Server 에서 해당 코드가 실행된다.
	// 마찬가지로 클라이언트 측에서 , Server RPC 를 호출해도 Server 에서 해당 코드가 실행된다.
	// 그러면, 사실상 if 문을 통해서 분기문을 작성할 필요가 없다는 것이다.
	// m_bAiming 는 replicate 해 두었기 때문에, 어떤 상황에서든 모든 클라이언트 측으로 변경사항이 반영될 것이다
	ServerSetAiming(bIsAiming);

	// 이동 속도 줄이기 (해당 함수는 서버, 클라이언트에서 모두 호출되는 함수이다)
	if (m_BlasterCharacter)
	{
		m_BlasterCharacter->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? m_AimWalkSpeed : m_BaseWalkSpeed;
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	m_bAiming = bIsAiming;

	// GetCharacterMovement()->MaxWalkSpeed 의 경우, GetCharacterMovement() 에 해당하는 컴포넌트가 
	// 계속 기존의 MaxWalkSpeed로 유지하려는 경향이 있다.
	// 하지만 이렇게 Server RPC 를 통해서 호출해주면, 유지안해주고, 우리가 원하는 값으로 Update 시켜준다.
	if (m_BlasterCharacter)
	{
		m_BlasterCharacter->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? m_AimWalkSpeed : m_BaseWalkSpeed;
	}
}

// Rap Notify 이다.
// 해당 함수가 호출되었다는 것은 m_EquippedWeapon 에 변화가 발생했다는 것
// 즉, 무기를 장착했거나 버렸다는 것
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

	// 이를 통해 서버 측에서 ServerFire_Implementation 를 실행할 것이다
	// 클라이언트에서 호출해도 서버에서 실행
	// 서버에서 호출해도 서버에서 실행
	// 여기서 중요한 것이 있다. 이렇게 ServerRPC 만 호출하면, 실행이 Server 에서만 된다.
	// 즉, 실제 Client 에서든 서버에서든 Fire 버튼을 누르면 Server 에 해당하는 Machine 에서만 Fire 모션이 보인다.
	// 물론, m_FireButtonPressed 를 replicate 하고, Rap_Notify 를 실행하면 해결할 수 있다.
	
	// 하지만, 이것을 하고 싶지는 않다.
	// 우리는 계속해서 Fire Button 을 누르고 있을 수 있다 (Left Mouse Button)
	// 그러면 m_FireButtonPressed 는 계속 true 로 남아있을 것이다. 변하지 않는다는 것이다.
	// 그런데 Replication 은 해당 변수가 변할 때만 동작한다.
	// 그러면 Server 측에서는 계속 Fire 를 하고 있어도, 해당 변수 정보가 REplicate 되지 않아서
	// 다른 Client 들에게 정상적으로 정보가 전달되지 않는다는 것이다.

	// 따라서 Client 들에게 무언가 일어나고 있음을, 다른 방식으로 전달해줘야 한다.
	// 이는 Multicast RPC 를 통해 할 수 있다.
	// 서버에서 Multicast RPC  를 호출하면, 서버와 클라이언트 모두에서 실행하게 된다는 것이다.
	// 클라이언트에서 호출하면 Invoking Client 에서만 실행된다. 따라서 모든 클라이언트에 BroadCast 해주려면
	// 사실상 서버에서 Multicast RPC 를 호출해주어야 한다.
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

	// 각 Machine 에서 Player0 => Player Who Is Controlling The Pawn (즉, 자기 자신)
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

			// HitTarget 변수로 세팅
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
			// - Charcter 의 Speed 에 따라 크기가 조금씩 달라지게 할 것이다
			// [0, 600] -> [0, 1] (최대 속도가 600이라고 한다면, 비율에 따라 0과 1 범위로 mapping 시킬 것이다)
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
	// if (m_BlasterCharacter && m_FireButtonPressed) : m_FireButtonPressed 정보는 필요없다.
	// 왜냐하면 m_FireButtonPressed 라는 정보는 Client 측에만 존재하는 데이터. 하지만 현재 함수는 서버에서 실행

	if (m_BlasterCharacter)
	{
		// m_bAiming : GetLifetimeReplicatedProps() 에서 Replicated Variable 로 등록해둔 상태
		// 따라서 정상적으로 클라이언트 측으로 m_bAiming 에 대한 정보가 전달될 것이다.
		m_BlasterCharacter->PlayFireMontage(m_bAiming);

		// Fire Weapon -> Play Animation For Weapon Itself In It
		// m_EquippedWeapon->Fire(m_HitTarget);

		// TraceHitTarget : HitTarget That Is Being BroadCasted 
		// ex) Client Calculate TraceHitTarget -> RPC -> Pass In TraceHitTarget -> Execute All On Server, Client 
		m_EquippedWeapon->Fire(TraceHitTarget); 
	}
}

// 자. 해당 함수는 Server 측에서"만" 호출된다.
// 만약 Multicast RPC 를 서버에서 "호출" 하면 서버와 클라이언트 모두에서 실행된다.
// 즉, 정상적으로 Server 측의 정보가 Broad Cast 된다는 것이다.

// 정리 : Client -> Server RPC 인 ServerFire 호출 -> ServerFire_Implementation 가 서버에서 실행 ->
//       -> Multicast RPC 를 Server 에서 호출 -> MulticastFire_Implementation 가 모든 Client, 서버에서 실행
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
	// (Bluepint 를 통해서 Mesh 를 찾고 원하는 위치에 Socket 추가하기 (46강)
	m_EquippedWeapon = WeaponToEquip;

	// 자. 우리는 CombatComponent 가 Replicate 되도록 세팅했다 (BlasterCharacter 생성자 참고)
	// 서버에서 위와 같이 정보들을 새롭게 세팅하기 때문에, 해당 정보들이 Client 측으로 Replicate 될 것이다.
	// 하지만, m_EquippedWeapon 같은 경우는 안된다.
	// 따라서 m_EquippedWeapon 에서의 특정 Property 들도 Replicate 되도록 세팅해야 한다.
	// 따라서 이를 위해서 Variable Replication 을 진행할 것이다.
	// - m_WeaponState 변수가 Replicate 되도록 한다.
	// - 이것이 가능한 이유는 Weapon 이 Replicate 가 가능한 Actor 로 세팅해둔 상태이기 때문이다
	// - 생성자 : bReplicates = true; 
	m_EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

	const USkeletalMeshSocket* HandSocket = m_BlasterCharacter->GetMesh()->GetSocketByName(FName("RightHandSocket"));

	if (HandSocket)
	{
		HandSocket->AttachActor(m_EquippedWeapon, m_BlasterCharacter->GetMesh());
	}

	// Set Owner
	// SetOwner() 라는 변수가 Replicated 되도록 이미 설정되어 있다. 
	// 따라서 Client 측에도 Replicate 될 것이다.
	m_EquippedWeapon->SetOwner(m_BlasterCharacter);

	// MovementComponent 의 World상의 Rotation 이 아니라
	// PlayerController 의 World 상의 Rotation 정보를 사용할 것이다. 마우스...? (강좌 51. 52) 
	// 왜냐하면 플레이상, 마우스가 바라보는 방향으로 총을 쏘기 위해 마우스 방향과 플레이어가 바라보는 방향을 일치시켜야 하기 때문
	// 당연히 아래의 변수에 대해서도 Replication 을 진행해줘야 한다. OnRep_EquippedWeapon 를 통해 진행할 것이다.
	m_BlasterCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;

	m_BlasterCharacter->bUseControllerRotationYaw = true;
}

