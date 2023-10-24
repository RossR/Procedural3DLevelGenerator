// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Data/FunctionLibraries/LevelGenerationLibrary.h"
#include "LevelStreamingProcedural.generated.h"

class AInteractableActor_Elevator;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProceduralLevelLoadedDelegate, ULevelStreamingProcedural*, LoadedLevelStreamingProcedural);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnDoorSlotLevelLoadedDelegate, ULevelStreamingProcedural*, LoadedLevelStreamingProcedural, ULevel*, LoadedLevel, FTileData, InLevelTileData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerSpawnRoomLoadedDelegate, ULevelStreamingProcedural*, LoadedLevelStreamingProcedural, ULevel*, LoadedLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnElevatorBottomLoadedDelegate, ULevelStreamingProcedural*, LoadedLevelStreamingProcedural, ULevel*, LoadedLevel, FElevatorBottomInfo, InElevatorBottomInfo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnElevatorTopLoadedDelegate, ULevelStreamingProcedural*, LoadedLevelStreamingProcedural, ULevel*, LoadedLevel, FElevatorTopInfo, InElevatorTopInfo);


UCLASS()
class PROJECTSCIFI_API ULevelStreamingProcedural : public ULevelStreamingDynamic
{
	GENERATED_BODY()
	
public:

	ULevelStreamingProcedural();

	// Override the CreateInstance function to use the custom level streaming class.
	UFUNCTION(BlueprintCallable, Category = "Game")
	ULevelStreamingProcedural* CreateProceduralInstance(UWorld* InWorld, ULevelStreaming* StreamingLevel, const FString& InstanceUniqueName);

protected:

	/** Virtual call for derived classes of ULevelStreaming to add their logic. */
	virtual void OnLevelLoadedChanged(ULevel* Level) override;

public:

	UPROPERTY(BlueprintAssignable)
	FOnProceduralLevelLoadedDelegate OnProceduralLevelLoaded;

	UPROPERTY(BlueprintAssignable)
	FOnDoorSlotLevelLoadedDelegate OnDoorSlotLevelLevelLoaded;
	
	UPROPERTY(BlueprintAssignable)
	FOnPlayerSpawnRoomLoadedDelegate OnPlayerSpawnRoomLoaded;

	UPROPERTY(BlueprintAssignable)
	FOnElevatorBottomLoadedDelegate OnElevatorBottomLoaded;

	UPROPERTY(BlueprintAssignable)
	FOnElevatorTopLoadedDelegate OnElevatorTopLoaded;

	FTileData LevelTileData;
	FElevatorBottomInfo ElevatorBottomInfo;
	FElevatorTopInfo ElevatorTopInfo;

};
