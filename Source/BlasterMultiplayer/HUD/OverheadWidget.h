// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OverheadWidget.generated.h"

/**
 * 
 */
UCLASS()
class BLASTERMULTIPLAYER_API UOverheadWidget : public UUserWidget
{
	GENERATED_BODY()
public :
	UPROPERTY(meta = (BindWidget)) // Associate C++ Variable With Text Block In Widget Blueprint
	class UTextBlock* DisplayText;

	void SetDisplayText(FString TextToDisplay);


	// Called From EventBeginPlay() From BP_BlasterCharacter
	// Figure Out Network Role
	UFUNCTION(BlueprintCallable)
	void ShowPlayerNetRole(APawn* InPawn);

protected :
	// Called When Transition To Another Level Or Leave Current Level
	virtual void OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld) override;
};
