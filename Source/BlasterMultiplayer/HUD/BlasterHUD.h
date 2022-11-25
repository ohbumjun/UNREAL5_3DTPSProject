// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BlasterHUD.generated.h"

// Blueprint �󿡼� Type ���� ������ �� �ִ�.
USTRUCT(BlueprintType)
struct FHUDPackage
{
	// Replextion �� �����Ѵ�.
	GENERATED_BODY()
public :
	class UTexture2D* CrosshairsCenter;
	class UTexture2D* CrosshairsLeft;
	class UTexture2D* CrosshairsRight;
	class UTexture2D* CrosshairsTop;
	class UTexture2D* CrosshairsBottom;

	// How much we should spread crosshairs (x,y �������� ���� ������ spread, shrink)
	float CrosshairSpread;

	FLinearColor CrosshairsColor;
};

/**
���� Map ������ �������Ʈ�� �̿��Ͽ� ������ ���̴�.
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
