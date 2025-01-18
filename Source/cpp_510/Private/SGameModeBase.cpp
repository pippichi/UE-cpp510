// Fill out your copyright notice in the Description page of Project Settings.


#include "SGameModeBase.h"
#include "AIModule\Classes\EnvironmentQuery\EnvQueryManager.h"
#include "AIModule\Classes\EnvironmentQuery\EnvQueryInstanceBlueprintWrapper.h"
#include "AI\SAICharacter.h"
#include "SAttributeComponent.h"
#include "EngineUtils.h"
#include "SCharacter.h"
#include "SPlayerState.h"
#include "Kismet\GameplayStatics.h"
#include "SSaveGame.h"
#include "GameFramework\GameStateBase.h"
#include "SGamePlayInterface.h"
#include "Serialization\ObjectAndNameAsStringProxyArchive.h"
#include "SMonsterData.h"
#include "cpp_510\cpp_510.h"
#include "SActionComponent.h"
#include "SSaveGameSubsystem.h"
#include "Engine\AssetManager.h"

static TAutoConsoleVariable<bool> CVarSpawnBots(TEXT("su.SpawnBots"), false, TEXT("Enable spawning of bots via timer."), ECVF_Cheat);

ASGameModeBase::ASGameModeBase()
{
	SpawnTimerInterval = 2.0f;
	CreditsPerKill = 20;
	CooldownTimeBetweenFailures = 8.0f;

	DesiredPowerupCount = 10;
	RequiredPowerupDistance = 2000;

	PlayerStateClass = ASPlayerState::StaticClass();
}

void ASGameModeBase::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	//FString SelectedSaveSlot = UGameplayStatics::ParseOption(Options, "SaveGame");
	//if (SelectedSaveSlot.Len() > 0) {
	//	SlotName = SelectedSaveSlot;
	//}
	//LoadSaveGame();
	
	// (Save/Load logic moved into new SaveGameSubsystem)
	USSaveGameSubsystem* SG = GetGameInstance()->GetSubsystem<USSaveGameSubsystem>();
	
	// Optional slot name (Falls back to slot specified in SaveGameSettings class/INI otherwise)
	FString SelectedSaveSlot = UGameplayStatics::ParseOption(Options, "SaveGame");
	SG->LoadSaveGame(SelectedSaveSlot);
}

void ASGameModeBase::StartPlay()
{
	Super::StartPlay();

	// Continuous timer to spawn in more bots.
	// Actual amount of bots and whether its allowed to spawn determined by spawn logic later in the chain...
	GetWorldTimerManager().SetTimer(TimerHandle_SpawnBots, this, &ASGameModeBase::SpawnBotTimerElapsed, SpawnTimerInterval, true);

	// Make sure we have assigned as least one power-up class
	if (ensure(PowerupClasses.Num() > 0)) {
		// Run EQS to find potential power-up spawn locations
		UEnvQueryInstanceBlueprintWrapper* QueryInstance = UEnvQueryManager::RunEQSQuery(this, PowerupSpawnQuery, this, EEnvQueryRunMode::AllMatching, nullptr);
		if (ensure(QueryInstance)) {
			QueryInstance->GetOnQueryFinishedEvent().AddDynamic(this, &ASGameModeBase::OnPowerupSpawnQueryCompleted);
		}
	}
}

void ASGameModeBase::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	// Calling Before Super:: so we set variables before 'beginplayingstate' is called in PlayerController (which is where we instantiate UI)
	//ASPlayerState* PS = NewPlayer->GetPlayerState<ASPlayerState>();
	//if (ensure(PS)) {
	//	PS->LoadPlayerState(CurrentSaveGame);
	//}
	
	// (Save/Load logic moved into new SaveGameSubsystem)
	USSaveGameSubsystem* SG = GetGameInstance()->GetSubsystem<USSaveGameSubsystem>();
	SG->HandleStartingNewPlayer(NewPlayer);

	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	// Now we're ready to override spawn location
	// Alternatively we could override core spawn location to use store locations immediately (skipping the whole 'find player start' logic)
	SG->OverrideSpawnTransform(NewPlayer);
}

void ASGameModeBase::KillAll()
{
	for (TActorIterator<ASAICharacter> It(GetWorld()); It; ++It) {
		ASAICharacter* Bot = *It;

		USAttributeComponent* AttributeComp = USAttributeComponent::GetAttributes(Bot);
		if (ensureAlways(AttributeComp) && AttributeComp->IsAlive()) {
			AttributeComp->Kill(this);
		}
	}
}


void ASGameModeBase::SpawnBotTimerElapsed()
{
	if (!CVarSpawnBots.GetValueOnGameThread()) {
		UE_LOG(LogTemp, Warning, TEXT("Bot spawning disabled via cvar 'CVarSpawnBots'."));
		return;
	}
	int32 NrofAliveBots = 0;
	for (TActorIterator<ASAICharacter> It(GetWorld()); It; ++It) {
		ASAICharacter* Bot = *It;
		USAttributeComponent* AttributeComp = USAttributeComponent::GetAttributes(Bot);
		if (ensureAlways(AttributeComp) && AttributeComp->IsAlive()) {
			++NrofAliveBots;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Found %i alive bots."), NrofAliveBots);

	float MaxBotCount = 10.0f;

	if (DifficultyCurve) {
		MaxBotCount = DifficultyCurve->GetFloatValue(GetWorld()->TimeSeconds);
	}

	if (NrofAliveBots >= MaxBotCount) {
		UE_LOG(LogTemp, Log, TEXT("At maximum bot capacity. Skipping bot spawn."));
		return;
	}

	UEnvQueryInstanceBlueprintWrapper* QueryInstance = UEnvQueryManager::RunEQSQuery(this, SpawnBotQuery, this, EEnvQueryRunMode::RandomBest5Pct, nullptr);
	if (ensureAlways(QueryInstance)) {
		QueryInstance->GetOnQueryFinishedEvent().AddDynamic(this, &ASGameModeBase::OnQueryCompleted);
	}
}

void ASGameModeBase::OnQueryCompleted(UEnvQueryInstanceBlueprintWrapper* QueryInstance, EEnvQueryStatus::Type QueryStatus)
{
	if (QueryStatus != EEnvQueryStatus::Success) {
		UE_LOG(LogTemp, Warning, TEXT("Spawn bot EQS Query failed!"));
		return;
	}

	TArray<FVector> Locations = QueryInstance->GetResultsAsLocations();
	//if (Locations.Num() > 0) {}
	if (Locations.IsValidIndex(0)) {
		
		if (MonsterTable)
		{

			TArray<FMonsterInfoRow*> Rows;
			MonsterTable->GetAllRows("", Rows);

			int32 RandomIndex = FMath::RandRange(0, Rows.Num() - 1);

			FMonsterInfoRow* SelectedRow = Rows[RandomIndex];

			UAssetManager* Manager = UAssetManager::GetIfValid();
			if (Manager) {
				LogOnScreen(this, "Loading monster...", FColor::Green);
				// 按需指定加载某些字段，比方说对象有若干个TSoftObjectPtr类型字段，则可以指定加载某些字段
				TArray<FName> Bundles;

				FVector Location = Locations[0];
				Location.Z = 88.0f;
				FStreamableDelegate Delegate = FStreamableDelegate::CreateUObject(this, &ASGameModeBase::OnMonsterLoaded, SelectedRow->MonsterId, Location);

				Manager->LoadPrimaryAsset(SelectedRow->MonsterId, Bundles, Delegate);
			}
			//AActor* NewBot = GetWorld()->SpawnActor<AActor>(SelectedRow->MonsterData->MonsterClass, Location, FRotator::ZeroRotator);;
			//if (NewBot) {
			//	LogOnScreen(this, FString::Printf(TEXT("Spawned enemy: %s (%s)"), *GetNameSafe(NewBot), *GetNameSafe(SelectedRow->MonsterData)));
			//	// Grant special actions, buffs etc.
			//	USActionComponent* ActionComp = Cast<USActionComponent>(NewBot->GetComponentByClass(USActionComponent::StaticClass()));
			//	if (ActionComp) {
			//		for (TSubclassOf<USAction> ActionClass : SelectedRow->MonsterData->Actions) {
			//			ActionComp->AddAction(NewBot, ActionClass);
			//		}
			//	}

			//	// track all the used spawn location
			//	DrawDebugSphere(GetWorld(), Location, 50.0f, 20, FColor::Blue, false, 60.0f);
			//}
		}
	}
}

void ASGameModeBase::OnMonsterLoaded(FPrimaryAssetId LoadedId, FVector SpawnLocation)
{
	LogOnScreen(this, "Finished loading.", FColor::Green);

	UAssetManager* Manager = UAssetManager::GetIfValid();
	if (Manager) {
		USMonsterData* MonsterData = Cast<USMonsterData>(Manager->GetPrimaryAssetObject(LoadedId));
		if (MonsterData) {
			AActor* NewBot = GetWorld()->SpawnActor<AActor>(MonsterData->MonsterClass, SpawnLocation, FRotator::ZeroRotator);;
			if (NewBot) {
				LogOnScreen(this, FString::Printf(TEXT("Spawned enemy: %s (%s)"), *GetNameSafe(NewBot), *GetNameSafe(MonsterData)));
				// Grant special actions, buffs etc.
				USActionComponent* ActionComp = Cast<USActionComponent>(NewBot->GetComponentByClass(USActionComponent::StaticClass()));
				if (ActionComp) {
					for (TSubclassOf<USAction> ActionClass : MonsterData->Actions) {
						ActionComp->AddAction(NewBot, ActionClass);
					}
				}

				// track all the used spawn location
				DrawDebugSphere(GetWorld(), SpawnLocation, 50.0f, 20, FColor::Blue, false, 60.0f);
			}
		}
	}

}

void ASGameModeBase::OnPowerupSpawnQueryCompleted(UEnvQueryInstanceBlueprintWrapper* QueryInstance, EEnvQueryStatus::Type QueryStatus)
{
	if (QueryStatus != EEnvQueryStatus::Success) {
		UE_LOG(LogTemp, Warning, TEXT("Spawn powerup EQS Query Failed!"));
		return;
	}

	TArray<FVector> Locations = QueryInstance->GetResultsAsLocations();

	// Keep used Locations to easily check distance between points
	TArray<FVector> UsedLocations;

	int32 SpawnCounter = 0;
	// Break out if we reached the desired count or if we have no more potential positions remaining
	while (SpawnCounter < DesiredPowerupCount && Locations.Num() > 0) {
		// pick a random location from remaining points
		int32 RandomLocationIndex = FMath::RandRange(0, Locations.Num() - 1);

		FVector PickedLocation = Locations[RandomLocationIndex];
		// Remove to avoid picking again
		Locations.RemoveAt(RandomLocationIndex);

		// Check minimum distance requirement
		bool bValidLocation = true;
		for (FVector OtherLocation : UsedLocations) {
			float DistanceTo = (PickedLocation - OtherLocation).Size();
			if (DistanceTo < RequiredPowerupDistance) {
				// Show skipped Locations due to distance
				//DrawDebugSphere(GetWorld(), PickedLocation, 50.0f, 20, FColor::Red, false, 10.0f);
				bValidLocation = false;
				break;
			}
		}
		if (!bValidLocation) {
			continue;
		}

		// Pick a random powerup-class
		int32 RandomClassIndex = FMath::RandRange(0, PowerupClasses.Num() - 1);
		TSubclassOf<AActor> RandomPowerupClass = PowerupClasses[RandomClassIndex];

		GetWorld()->SpawnActor<AActor>(RandomPowerupClass, PickedLocation, FRotator::ZeroRotator);

		// Keep for distance checks
		UsedLocations.Add(PickedLocation);
		++SpawnCounter;
	}
}


void ASGameModeBase::OnActorKilled(AActor* VictimActor, AActor* Killer)
{
	UE_LOG(LogTemp, Log, TEXT("OnActorKilled: victim: %s, Killer: %s"), *GetNameSafe(VictimActor), *GetNameSafe(Killer));

	// Respawn Player after delay
	ASCharacter* Player = Cast<ASCharacter>(VictimActor);
	if (Player) {
		// Disabled auto-respawn
		/*FTimerHandle TimerHandle_RespawnDelay;
		FTimerDelegate Delegate;
		Delegate.BindUFunction(this, "RespawnPlayerElapsed", Player->GetController());

		float RespawnDelay = 2.0f;
		GetWorldTimerManager().SetTimer(TimerHandle_RespawnDelay, Delegate, RespawnDelay, false);*/

		// Store time if it was better than previous record
		ASPlayerState* PS = Player->GetPlayerState<ASPlayerState>();
		if (PS)
		{
			PS->UpdatePersonalRecord(GetWorld()->TimeSeconds);
		}
		USSaveGameSubsystem* SG = GetGameInstance()->GetSubsystem<USSaveGameSubsystem>();
		// Immediately auto save on death
		SG->WriteSaveGame();
	}

	// Give Credits for kill
	APawn* KillerPawn = Cast<APawn>(Killer);
	if (KillerPawn && KillerPawn != VictimActor) {

		// Only Players will have a 'PlayerState' instance, bots have nullptr
		ASPlayerState* PS = KillerPawn->GetPlayerState<ASPlayerState>();
		if (PS) {
			PS->AddCredits(CreditsPerKill);
		}
	}
}

void ASGameModeBase::RespawnPlayerElapsed(AController* Controller)
{
	if (ensureAlways(Controller)) {
		Controller->UnPossess();
		RestartPlayer(Controller);
	}
}

void ASGameModeBase::WriteSaveGame()
{
	for (int32 i = 0; i < AGameModeBase::GameState->PlayerArray.Num(); ++i) {
		ASPlayerState* PS = Cast<ASPlayerState>(AGameModeBase::GameState->PlayerArray[i]);
		if (PS) {
			PS->SavePlayerState(CurrentSaveGame);
			break; // single player only at this point
		}
	}

	CurrentSaveGame->SavedActors.Empty();

	for (FActorIterator It(GetWorld()); It; ++It) {
		AActor* Actor = *It;
		// Only interested in our 'gameplay actors'
		if (!Actor->Implements<USGamePlayInterface>()) {
			continue;
		}
		FActorSaveData ActorData;
		ActorData.ActorName = Actor->GetFName();
		ActorData.Transform = Actor->GetActorTransform();

		// Pass the array to fill with data from Actor
		FMemoryWriter MemWriter(ActorData.ByteData, true);

		FObjectAndNameAsStringProxyArchive Ar(MemWriter, false);
		// Find only vatiables with UPROPERTY(SaveGame)
		Ar.ArIsSaveGame = true;
		Ar.ArNoDelta = true;
		// Convert Actor's SaveGame UPROPERTIES into binary array
		Actor->Serialize(Ar);

		CurrentSaveGame->SavedActors.Add(ActorData);
	}

	UGameplayStatics::SaveGameToSlot(CurrentSaveGame, SlotName, 0);
}

void ASGameModeBase::LoadSaveGame()
{
	if (UGameplayStatics::DoesSaveGameExist(SlotName, 0)) {
		CurrentSaveGame = Cast<USSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
		if (CurrentSaveGame == nullptr) {
			UE_LOG(LogTemp, Warning, TEXT("Failed to load SaveGame Data."));
			return;
		}
		UE_LOG(LogTemp, Warning, TEXT("Loaded SaveGame Data."));

		//// Iterate the entire world of actors
		//// @fixme 这里还有问题，UE5 TreasureChest此时还没初始化
		//for (FActorIterator It(GetWorld()); It; ++It) {
  // 			AActor* Actor = *It;
		//	// Only interested in our 'gameplay actors'
		//	if (!Actor->Implements<USGamePlayInterface>()) {
		//		continue;
		//	}
		//	for (FActorSaveData ActorData : CurrentSaveGame->SavedActors) {
		//		if (ActorData.ActorName == Actor->GetName()) {
		//			Actor->SetActorTransform(ActorData.Transform);
		//			break;
		//		}
		//	}
		//}
	}
	else {
		CurrentSaveGame = Cast<USSaveGame>(UGameplayStatics::CreateSaveGameObject(USSaveGame::StaticClass()));
		UE_LOG(LogTemp, Warning, TEXT("Created New SaveGame Data."));
	}

}

void ASGameModeBase::UpdateInteractsInfo()
{
	if (!ensure(CurrentSaveGame)) {
		return;
	}
	// Iterate the entire world of actors
	for (FActorIterator It(GetWorld()); It; ++It) {
		AActor* Actor = *It;
		// Only interested in our 'gameplay actors'
		if (!Actor->Implements<USGamePlayInterface>()) {
			continue;
		}
		for (FActorSaveData ActorData : CurrentSaveGame->SavedActors) {
			if (ActorData.ActorName == Actor->GetFName()) {
				Actor->SetActorTransform(ActorData.Transform);

				FMemoryReader MemReader(ActorData.ByteData, true);

				FObjectAndNameAsStringProxyArchive Ar(MemReader, false);
				// Find only vatiables with UPROPERTY(SaveGame)
				Ar.ArIsSaveGame = true;
				Ar.ArNoDelta = true;
				// Convert binary array back into actor's variables
				Actor->Serialize(Ar);

				ISGamePlayInterface::Execute_OnActorLoaded(Actor);

				break;
			}
		}
	}
}
