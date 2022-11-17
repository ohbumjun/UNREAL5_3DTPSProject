// Fill out your copyright notice in the Description page of Project Settings.

#include "OverheadWidget.h"
#include "Components/TextBlock.h"

void UOverheadWidget::SetDisplayText(FString TextToDisplay)
{
	if (DisplayText)
	{
		DisplayText->SetText(FText::FromString(TextToDisplay));
	}
}

// Called From EventBeginPlay() From BP_BlasterCharacter
void UOverheadWidget::ShowPlayerNetRole(APawn* InPawn)
{
	// Get Local Network Role Of Pawn
	ENetRole LocalRole = InPawn->GetLocalRole();

	FString Role;
	switch (LocalRole)
	{
		// Server
	case ENetRole::ROLE_Authority :
		Role = FString("Authority");
		break;
		// Your Machine
	case ENetRole::ROLE_AutonomousProxy :
		Role = FString("AutonomousProxy");
		break;
		// Other Machine
	case ENetRole::ROLE_SimulatedProxy :
		Role = FString("SimulatedProxy");
		break;
	case ENetRole::ROLE_None :
		Role = FString("None");
		break;
	}

	FString LocalRoleString = FString::Printf(TEXT("Local Role : %s"), *Role);

	SetDisplayText(LocalRoleString);
}

void UOverheadWidget::OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld)
{
	// Remove Widget From ViewPort
	Super::OnLevelRemovedFromWorld(InLevel, InWorld);
}
