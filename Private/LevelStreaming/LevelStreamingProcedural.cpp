// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelStreaming/LevelStreamingProcedural.h"

ULevelStreamingProcedural::ULevelStreamingProcedural()
{

}

ULevelStreamingProcedural* ULevelStreamingProcedural::CreateProceduralInstance(UWorld* InWorld, ULevelStreaming* StreamingLevel, const FString& InstanceUniqueName)
{
	ULevelStreamingProcedural* ProceduralStreamingLevelInstance = nullptr;

	//UWorld* InWorld = GetWorld();
	if (InWorld && StreamingLevel)
	{
		// Create instance long package name 
		FString InstanceShortPackageName = InWorld->StreamingLevelsPrefix + FPackageName::GetShortName(InstanceUniqueName);
		FString InstancePackagePath = FPackageName::GetLongPackagePath(StreamingLevel->GetWorldAssetPackageName()) + TEXT("/");
		FName	InstanceUniquePackageName = FName(*(InstancePackagePath + InstanceShortPackageName));

		// check if instance name is unique among existing streaming level objects
		const bool bUniqueName = (InWorld->GetStreamingLevels().IndexOfByPredicate(ULevelStreaming::FPackageNameMatcher(InstanceUniquePackageName)) == INDEX_NONE);

		if (bUniqueName)
		{
			ProceduralStreamingLevelInstance = NewObject<ULevelStreamingProcedural>(InWorld, ULevelStreamingProcedural::StaticClass(), NAME_None, RF_Transient, NULL);
			// new level streaming instance will load the same map package as this object
			ProceduralStreamingLevelInstance->PackageNameToLoad = ((StreamingLevel->PackageNameToLoad == NAME_None) ? StreamingLevel->GetWorldAssetPackageFName() : StreamingLevel->PackageNameToLoad);
			// under a provided unique name

			FSoftObjectPath WorldAssetPath(*WriteToString<512>(InstanceUniquePackageName, TEXT("."), FPackageName::GetShortName(ProceduralStreamingLevelInstance->PackageNameToLoad)));
			ProceduralStreamingLevelInstance->SetWorldAsset(TSoftObjectPtr<UWorld>(WorldAssetPath));
			ProceduralStreamingLevelInstance->SetShouldBeLoaded(false);
			ProceduralStreamingLevelInstance->SetShouldBeVisible(false);
			ProceduralStreamingLevelInstance->LevelTransform = StreamingLevel->LevelTransform;

			// add a new instance to streaming level list
			InWorld->AddStreamingLevel(ProceduralStreamingLevelInstance);
		}
		else
		{
			UE_LOG(LogStreaming, Warning, TEXT("Provided streaming level instance name is not unique: %s"), *InstanceUniquePackageName.ToString());
		}
	}

	return ProceduralStreamingLevelInstance;
}

void ULevelStreamingProcedural::OnLevelLoadedChanged(ULevel* Level)
{
	if (!Level) { return; }

	OnPlayerSpawnRoomLoaded.Broadcast(this, Level);
	OnProceduralLevelLoaded.Broadcast(this);
	OnDoorSlotLevelLevelLoaded.Broadcast(this, Level, LevelTileData);
	OnElevatorBottomLoaded.Broadcast(this, Level, ElevatorBottomInfo);
	OnElevatorTopLoaded.Broadcast(this, Level, ElevatorTopInfo);

}
