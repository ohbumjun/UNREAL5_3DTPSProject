// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BlasterHUD.generated.h"

// Blueprint 상에서 Type 으로 지정할 수 있다.
USTRUCT(BlueprintType)
struct FHUDPackage
{
	// Replextion 을 진행한다.
	GENERATED_BODY()
public :
	class UTexture2D* CrosshairsCenter;
	class UTexture2D* CrosshairsLeft;
	class UTexture2D* CrosshairsRight;
	class UTexture2D* CrosshairsTop;
	class UTexture2D* CrosshairsBottom;

	// How much we should spread crosshairs (x,y 방향으로 같은 정도로 spread, shrink)
	float CrosshairSpread;

	FLinearColor CrosshairsColor;
};

/**
실제 Map 에서는 블루프린트를 이용하여 세팅할 것이다.
 */
UCLASS()
class BLASTERMULTIPLAYER_API ABlasterHUD : public AHUD
{
	GENERATED_BODY()
	
public  :
	// Called Every Frame
	virtual void DrawHUD() override;

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	TSubclassOf<class UUserWidget> m_CharacterOverlayClass;

	UPROPERTY()
	class UCharacterOverlay* m_CharacterOverlay;

	void AddCharacterOverlay();
protected :
	virtual void BeginPlay() override;
private :
	// Texture For Drawing Cross Hair
	FHUDPackage m_HUDPackage;

	UPROPERTY(EditAnywhere)
	float m_CrosshairSpreadMax = 16.f;

	void DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread,
		FLinearColor CrosshairColor);
public :
	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { m_HUDPackage = Package; }
};
