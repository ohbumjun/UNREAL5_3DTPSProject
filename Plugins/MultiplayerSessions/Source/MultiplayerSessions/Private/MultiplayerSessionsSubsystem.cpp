// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"

// Callback Bound To Delegate
// CreateUObject : Construct New Delegate
UMultiplayerSessionsSubsystem::UMultiplayerSessionsSubsystem():
	CreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete)),
	FindSessionsCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionsComplete)),
	JoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete)),
	DestroySessionCompleteDelegate(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySessionComplete)),
	StartSessionCompleteDelegate(FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnStartSessionComplete))
{
	// Online Subsystem
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem)
	{
		// Access Session Interface
		SessionInterface = Subsystem->GetSessionInterface();
	}
}

void UMultiplayerSessionsSubsystem::CreateSession(int32 NumPublicConnections, FString MatchType)
{
	if (!SessionInterface.IsValid())
	{
		return;
	}

	// Check If Session Exist
	auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
	if (ExistingSession != nullptr)
	{
		// Destroy Exising Session
		// ���⼭ �߿��� ���� DestroySession �� ȣ���Ѵٰ� �ٷ� Session �� ������� ���̾ƴϴ�
		// Sever ���� Destroy Session ��û�� ������, ó���� �Ϸ�� ������ �ð��� �ɸ��µ�
		// �ٷ� �Ʒ����� �ٽ� CreateSession �� �ϰ� �Ǹ�, ���������� Session �� ��������� ���� ���� �ִ�.
		// ���� Delegate �� �̿��ؼ� Session �� ��� Destroy �� ���Ŀ� Session �� Create �� ���̴�
		bCreateSessionOnDestroy = true;
		LastNumPublicConnections = NumPublicConnections;
		LastMatchType = MatchType;

		DestroySession();
	}

	// Add Delegate To Session Interface Delegate List
	// Store the delegate in a FDelegateHandle so we can later remove it from the delegate list
	CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);

	LastSessionSettings = MakeShareable(new FOnlineSessionSettings());
	// m_LastSessionSettings->bIsLANMatch = false; // No LAN Match, But Going Go Connect Internet
	// IOnlineSubsystem::Get()->GetSubsystemName() : return "NULL" ?  it means LAN Connection ! -> Not Connected Steam �̶�� �ǹ�
	// 	But, Editor ������ null, -> In Packaged Project, It works
	LastSessionSettings->bIsLANMatch = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL" ? true : false;
	LastSessionSettings->NumPublicConnections = NumPublicConnections; // Max Total "NumPublicConnections" Players For Particular Session
																											// Different From DefaultGame.ini [/Script/Engine.GameSession] => Player For This Game Project
	LastSessionSettings->bAllowJoinInProgress = true;   // Player Can Join While Session Running
	LastSessionSettings->bAllowJoinViaPresence = true; // Find Session In Our Region ex) Asia
	LastSessionSettings->bShouldAdvertise = true;		 // Advertise Session So That Other Player Can Find
	LastSessionSettings->bUsesPresence = true;			 // Allow us to use prsesence when finding session
	// EOnlineDataAdvertisementType::ViaOnlineServiceAndPing : Our Session Will Be Advertised Online Service But Also Being Ping As Well
	LastSessionSettings->Set(FName("MatchType"), MatchType, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	LastSessionSettings->BuildUniqueId = 1;
	LastSessionSettings->bUseLobbiesIfAvailable = true;

	// Get Local Player Info From Player Controller
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();

	// Create Session Failure
	if (!SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *LastSessionSettings))
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);

		// Broadcast our own custom delegate
		MultiplayerOnCreateSessionComplete.Broadcast(false);
	}

	// Session Creation Successful => Will Call UMultiplayerSessionsSubsystem::OnCreateSessionComplete Callback Function

}

void UMultiplayerSessionsSubsystem::FindSessions(int32 MaxSearchResults)
{
	// Called From Menu.cpp

	if (!SessionInterface.IsValid())
		return;

	// Added Delegate To Delegate List => After OnlineSessionInterface->FindSessions Is Completed, 
	// Our Functions Will Be Called
	FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);

	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());
	LastSessionSearch->MaxSearchResults = MaxSearchResults;

	// IOnlineSubsystem::Get()->GetSubsystemName() : return "NULL" ?  it means LAN Connection ! -> Not Connected Steam �̶�� �ǹ�
	// 	But, Editor ������ null, -> In Packaged Project, It works
	LastSessionSearch->bIsLanQuery = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL" ? true : false;

	// Make Sure Any Sessions We Find Are Using Presence
	LastSessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();

	// Failure
	if (!SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef()))
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);

		// Call Custom Delegate (Put Empty Array)
		MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
	}

	// Successfully Found Session => Will Call OnFindSessionComplete Callback Function
}

void UMultiplayerSessionsSubsystem::JoinSession(const FOnlineSessionSearchResult& SessionResult)
{
	if (!SessionInterface.IsValid())
	{
		MultiplayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
		return;
	}

	// Add Delegate To Session Interface Delegate List
	JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!SessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SessionResult))
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);

		MultiplayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
	}

	// If Succedded => "OnJoinSessionComplete" Callback Will Be Called
}

void UMultiplayerSessionsSubsystem::DestroySession()
{
	if (!SessionInterface.IsValid())
	{
		// BroadCast Custom Delegate
		MultiplayerOnDestroySessionComplete.Broadcast(false);
		return;
	}

	// Add Delegate To Session Interface DelegateList
	DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);

	if (!SessionInterface->DestroySession(NAME_GameSession))
	{
		// Remove Delegate From Session Interface Delegate List
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);

		// BroadCast Custom Delegate
		MultiplayerOnDestroySessionComplete.Broadcast(false);
	}
}

void UMultiplayerSessionsSubsystem::StartSession()
{
}

// This Callback Called By Session Interface Delegate List
void UMultiplayerSessionsSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface)
	{
		// Remove Delegate From Session Interface Delegate List
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
	}

	// BroadCast our custom Delegate => Session Creation Successful
	// UMenu::OnCreateSession Will Be Called
	MultiplayerOnCreateSessionComplete.Broadcast(bWasSuccessful);
}

// This Callback Called By Session Interface Delegate List
void UMultiplayerSessionsSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	if (SessionInterface)
	{
		// Remove Delegate From Session Interface Delegate List
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
	}

	// Failure
	if (LastSessionSearch->SearchResults.Num() <= 0)
	{
		// Call Custom Delegate (Put Empty Array)
		MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		return;
	}

	// Find Session Successfult
	MultiplayerOnFindSessionsComplete.Broadcast(LastSessionSearch->SearchResults, bWasSuccessful);
}

// This Callback Called By Session Interface Delegate List
void UMultiplayerSessionsSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (SessionInterface)
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
	}

	// Will Call UMenu::OnJoinSession Callback Bound To This Delegate
	MultiplayerOnJoinSessionComplete.Broadcast(Result);
}

// This Callback Called By Session Interface Delegate List
void UMultiplayerSessionsSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	// Callback Function Bound To Session Interface Delegate List => Called When Destroy Session Tried 
	if (SessionInterface)
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
	}

	// Prevent Duplicate CreateSession() Function Call
	if (bWasSuccessful && bCreateSessionOnDestroy)
	{
		bCreateSessionOnDestroy = false;
		CreateSession(LastNumPublicConnections, LastMatchType);
	}

	// Broad Cast Custom Delegate
	MultiplayerOnDestroySessionComplete.Broadcast(bWasSuccessful);
}

// This Callback Called By Session Interface Delegate List
void UMultiplayerSessionsSubsystem::OnStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
}
