// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/FunctionLibraries/LevelGenerationLibrary.h"
#include "LevelStreaming/LevelStreamingProcedural.h"
#include "ProceduralLevelGenerationActor.generated.h"


class USpringArmComponent;
class USceneCaptureComponent2D;
class ULevelStreamingProcedural;
class AActorSlot_Door;
class AInteractableActor_Base;
class AInteractableActor_Door;

UCLASS()
class PROJECTSCIFI_API AProceduralLevelGenerationActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AProceduralLevelGenerationActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	
	/** Returns a list of all the available level generation settings. */
	UFUNCTION(CallInEditor)
	TArray<FName> GetSelectedLevelGenerationSettings() const;

	/** Updates LevelGenerationSettings with the latest level generation settings that are selected. */
	void GetLevelGenerationSettings();

protected:

	/** Called when a LevelStreamingProcedural is loaded. */
	UFUNCTION()
	void OnProceduralLevelLoaded(ULevelStreamingProcedural* LoadedLevelStreamingProcedural);

	/** Called when a LevelStreamingProcedural is loaded and is a sublevel for Door ActorSlots. */
	UFUNCTION()
	void OnDoorSlotLevelLevelLoaded(ULevelStreamingProcedural* LoadedLevelStreamingProcedural, ULevel* LoadedLevel, FTileData InLevelTileData);

	/** Called when a LevelStreamingProcedural is loaded and is a sublevel for PlayerSpawn ActorSlots. */
	UFUNCTION()
	void OnPlayerSpawnRoomLoaded(ULevelStreamingProcedural* LoadedLevelStreamingProcedural, ULevel* LoadedLevel);

	/** Called when a LevelStreamingProcedural is loaded and is the bottom section of an elevator shaft. */
	UFUNCTION(BlueprintCallable)
	void OnElevatorBottomLoaded(ULevelStreamingProcedural* LoadedLevelStreamingProcedural, ULevel* LoadedLevel, FElevatorBottomInfo InElevatorBottomInfo);

	/** Called when a LevelStreamingProcedural is loaded and is the top section of an elevator shaft. */
	UFUNCTION(BlueprintCallable)
	void OnElevatorTopLoaded(ULevelStreamingProcedural* LoadedLevelStreamingProcedural, ULevel* LoadedLevel, FElevatorTopInfo InElevatorTopInfo);

	/** Preloads every room and corridor before level generation begins. */
	void PreloadLevels();
	
	/** Instances and loads every room and corridor in the level. */
	void PopulateLevel();

	/** Builds the minimap */
	void BuildMinimap();

	/** Sets up all elevators in the level so that they traverse their shafts properly and can be called by terminals. */
	void SetupElevators();

	/** Places doors at access points that are in use. */
	void SetupDoors(ULevel* LoadedLevel, FTileData InLevelTileData);

	/// <summary>
	/// Checks if there is a door slot found at a certain direction of an access point.
	/// </summary>
	/// <param name="DoorSlotArray"> Array of all the door slots in the streamed level. </param> 
	/// <param name="AccessPointCoordinate"> The location of the access point being checked. </param>
	/// <param name="AccessPointDirection"> The direction to check for a door slot. </param>
	/// <returns>Reference to the door slot found.</returns>
	AActorSlot_Door* FindDoorSlot(const TArray<AActorSlot_Door*>& DoorSlotArray, FIntVector AccessPointCoordinate, EDirections AccessPointDirection);

	/** Loads all the levels in the provided data table. */
	void LoadLevelsFromDataTable(UDataTable* InDataTable);

	/** Loads the level with the provided package name. */
	void LoadLevel(const FName& LevelPackageName);

	/// <summary>
	/// Creates a procedural level instance of a room/corridor and binds various delegates depending on its tile type.
	/// </summary>
	/// <param name="CurrentCoordinate"> The location of the room/corridor. </param>
	/// <param name="RoomTileData"> The tile data of the room/corridor. </param>
	/// <param name="MapToInstance"> Reference to the level of the room/corridor. </param>
	/// <param name="MapSlotType"> The type of the room/corridor.</param>
	void CreateProceduralLevelInstance(FIntVector CurrentCoordinate, FTileData& RoomTileData, TSoftObjectPtr<UWorld> MapToInstance, EActorSlotType MapSlotType = EActorSlotType::None);

	/** Returns the static mesh component of the newly created minimap mesh. */
	UStaticMeshComponent* CreateMinimapMesh(UStaticMesh* MinimapMesh, const FString MeshName, const TArray<FName> MeshTags = TArray<FName>());

protected:

	// The currently selected profile for the level generation settings.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Procedural Level Generation", meta = (GetOptions = "GetSelectedLevelGenerationSettings"))
	FName LevelSettingsProfile = NAME_None;

	// Data table containing all the profiles of level generation settings.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Procedural Level Generation")
	UDataTable* LevelGenerationSettingsDataTable;

	// The currently loaded level generation settings
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Procedural Level Generation|Settings")
	FLevelGenerationSettings LevelGenerationSettings;

	// Data created from generating the procedural level.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Procedural Level Generation|Settings")
	FGeneratedLevelData GeneratedLevelData;

	// Map of all the levels preloaded for the level generation.
	TMap<FName, ULevelStreamingDynamic*> LoadedLevelMap;

	// Map containing all the room/corridor meshes needed for the minimap 
	TMap<FIntVector, FMinimapInfo_Room> LevelMinimap;
	// Map containing all the door meshes needed for the minimap 
	TMap<AInteractableActor_Door*, FMinimapInfo_Interactable> MinimapDoors;
	// Map containing all the interactable meshes needed for the minimap 
	TMap<AInteractableActor_Base*, FMinimapInfo_Interactable> MinimapInteractables;
	// Map containing all the blocked access point meshes needed for the minimap 
	TMap<AActor*, FMinimapInfo_Interactable> MinimapAccessBlockers;

	// Return true if the level generation has finished and we are waiting on all the levels to finish loading. 
	bool bLevelWaitingToLoad = false;

	// List containing all the levels that have not finised loading.
	TArray<ULevelStreamingProcedural*> LevelsLeftToLoad;

	// Location of where the minimap will be placed in world space.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Procedural Level Generation|Minimap")
	USceneComponent* MinimapLocation;

	// Component representing the player character on the minimap.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Procedural Level Generation|Minimap")
	UChildActorComponent* PlayerMarker;

	// Spring arm holding the minimap camera.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Procedural Level Generation|Minimap")
	USpringArmComponent* MinimapArm;

	// Captures the minimap from an orthographic perspective.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Procedural Level Generation|Minimap")
	USceneCaptureComponent2D* MinimapSceneCapture;
	// Acts as an opacity mask for MinimapSceneCapture.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Procedural Level Generation|Minimap")
	USceneCaptureComponent2D* MinimapOpacityMaskSceneCapture;

};
