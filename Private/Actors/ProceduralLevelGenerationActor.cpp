// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/ProceduralLevelGenerationActor.h"
#include "Engine/LevelStreaming.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/StreamableManager.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/DataTableFunctionLibrary.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "LevelStreaming/LevelStreamingProcedural.h"
#include "GameModes/SciFiGameModeBase.h"
#include "Actors/ActorSlots/ActorSlot_Door.h"
#include "Actors/Interactables/InteractableActor_Base.h"
#include "Actors/Interactables/InteractableActor_Elevator.h"
#include "Actors/Interactables/InteractableActor_Terminal.h"

// Sets default values
AProceduralLevelGenerationActor::AProceduralLevelGenerationActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MinimapLocation = CreateDefaultSubobject<USceneComponent>(TEXT("Minimap Location"));
	MinimapLocation->SetupAttachment(RootComponent);

	PlayerMarker = CreateDefaultSubobject<UChildActorComponent>(TEXT("Player Marker"));
	PlayerMarker->SetupAttachment(MinimapLocation);

	MinimapArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Minimap Arm"));
	MinimapArm->SetupAttachment(PlayerMarker);
	MinimapArm->TargetArmLength = 600.f;

	MinimapSceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Minimap Scene Capture"));
	MinimapSceneCapture->SetupAttachment(MinimapArm);
	MinimapSceneCapture->ProjectionType = ECameraProjectionMode::Orthographic;
	MinimapSceneCapture->OrthoWidth = 600.f;
	MinimapSceneCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;

	MinimapOpacityMaskSceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Minimap Opacity Scene Capture"));
	MinimapOpacityMaskSceneCapture->SetupAttachment(MinimapArm);
	MinimapOpacityMaskSceneCapture->ProjectionType = ECameraProjectionMode::Orthographic;
	MinimapOpacityMaskSceneCapture->OrthoWidth = 600.f;
	MinimapOpacityMaskSceneCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
}

// Called when the game starts or when spawned
void AProceduralLevelGenerationActor::BeginPlay()
{
	Super::BeginPlay();
	
	if (LevelGenerationSettingsDataTable) { GetLevelGenerationSettings(); }

	PreloadLevels();

	ULevelGenerationLibrary::GenerateLevel(LevelGenerationSettings, GeneratedLevelData, GetWorld());

	PopulateLevel();

	SetupElevators();
	
	MinimapSceneCapture->ShowOnlyActors.Add(this);
	MinimapOpacityMaskSceneCapture->ShowOnlyActors.Add(this);
	if (PlayerMarker->GetChildActor()) 
	{ 
		MinimapArm->AttachToComponent(PlayerMarker->GetChildActor()->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale );
		MinimapArm->TargetArmLength = 600.f;
		MinimapArm->SetRelativeRotation(FRotator(-20.f, 0.f, 0.f));
		MinimapSceneCapture->ShowOnlyActors.Add(PlayerMarker->GetChildActor()); 
		MinimapOpacityMaskSceneCapture->ShowOnlyActors.Add(PlayerMarker->GetChildActor());
	}
}

// Called every frame
void AProceduralLevelGenerationActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update the transform of the minimap player marker
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (AActor* PlayerMarkerChild = PlayerMarker->GetChildActor())
		{
			PlayerMarkerChild->SetActorRotation(FRotator(PlayerMarkerChild->GetActorRotation().Pitch, PC->GetControlRotation().Yaw, 0.f));
			if (APawn* PawnRef = PC->GetPawn())
			{
				FVector PlayerMinimapLocation = PawnRef->GetActorLocation() * LevelGenerationSettings.MinimapScale;
				PlayerMarkerChild->SetActorRelativeLocation(PlayerMinimapLocation);
			}
		}
	}

	// Update the transform of the minimap door meshes
	if (!MinimapDoors.IsEmpty())
	{
		TArray<AInteractableActor_Door*> MinimapDoorArray;
		MinimapDoors.GenerateKeyArray(MinimapDoorArray);

		for (AInteractableActor_Door* CurrentDoor : MinimapDoorArray)
		{
			if (!CurrentDoor) { continue; }

			const FTransform DoorMeshTransform = CurrentDoor->GetDoorMeshTransform();
			MinimapDoors[CurrentDoor].SetInteractableTransform(DoorMeshTransform.GetLocation(), DoorMeshTransform.GetRotation().Rotator(), DoorMeshTransform.GetScale3D(), LevelGenerationSettings.MinimapScale, this);
			MinimapDoors[CurrentDoor].MinimapMesh->SetWorldTransform(MinimapDoors[CurrentDoor].InteractableTransform);
		}
	}

	// Update the transform of the minimap door meshes
	if (!MinimapInteractables.IsEmpty())
	{
		TArray<AInteractableActor_Base*> MinimapInteractableArray;
		MinimapInteractables.GenerateKeyArray(MinimapInteractableArray);

		for (AInteractableActor_Base* CurrentInteractable : MinimapInteractableArray)
		{
			if (!CurrentInteractable) { continue; }

			const FTransform DoorMeshTransform = CurrentInteractable->GetActorTransform();
			MinimapInteractables[CurrentInteractable].SetInteractableTransform(DoorMeshTransform.GetLocation(), DoorMeshTransform.GetRotation().Rotator(), DoorMeshTransform.GetScale3D(), LevelGenerationSettings.MinimapScale, this);
			MinimapInteractables[CurrentInteractable].MinimapMesh->SetWorldTransform(MinimapInteractables[CurrentInteractable].InteractableTransform);
		}
	}
}

void AProceduralLevelGenerationActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	GetLevelGenerationSettings();
}

TArray<FName> AProceduralLevelGenerationActor::GetSelectedLevelGenerationSettings() const
{
	if (LevelGenerationSettingsDataTable)
	{
		static const FString ContextString(TEXT("GetSelectedLevelGenerationSettings"));
		return LevelGenerationSettingsDataTable->GetRowNames();
	}

	UE_LOG(LogTemp, Warning, TEXT("AWeapon_Base::GetSelectedWeaponOptions WeaponDataTable is invalid!"));
	return TArray<FName>();
}

void AProceduralLevelGenerationActor::GetLevelGenerationSettings()
{
	FLevelGenerationSettings* LevelGenerationSettingsPtr = nullptr;

	if (LevelGenerationSettingsDataTable)
	{
		static const FString ContextString(TEXT("GetSelectedWeaponOptions"));
		LevelGenerationSettingsPtr = LevelGenerationSettingsDataTable->FindRow<FLevelGenerationSettings>(LevelSettingsProfile, ContextString, true);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AWeapon_Base::GetSelectedWeaponOptions WeaponDataTable is invalid!"));
	}

	if (LevelGenerationSettingsPtr)
	{
		LevelGenerationSettings = *LevelGenerationSettingsPtr;

	}
}

void AProceduralLevelGenerationActor::OnProceduralLevelLoaded(ULevelStreamingProcedural* LoadedLevelStreamingProcedural)
{
	LevelsLeftToLoad.Remove(LoadedLevelStreamingProcedural);
	
	// Spawns the player into the level once it has finished loading
	if (bLevelWaitingToLoad && LevelsLeftToLoad.IsEmpty())
	{
		BuildMinimap();

		if (ASciFiGameModeBase* GameModeRef = Cast<ASciFiGameModeBase>(GetWorld()->GetAuthGameMode()))
		{
			GameModeRef->SpawnPlayerInLevel();
		}
	}
}

void AProceduralLevelGenerationActor::OnDoorSlotLevelLevelLoaded(ULevelStreamingProcedural* LoadedLevelStreamingProcedural, ULevel* LoadedLevel, FTileData InLevelTileData)
{
	SetupDoors(LoadedLevel, InLevelTileData);
}

void AProceduralLevelGenerationActor::OnPlayerSpawnRoomLoaded(ULevelStreamingProcedural* LoadedLevelStreamingProcedural, ULevel* LoadedLevel)
{
	AActor* PlayerStart = nullptr;
	if (LoadedLevel)
	{
		for (AActor* CurrentActor : LoadedLevel->Actors)
		{
			if (APlayerStart* PotentialPlayerStart = Cast<APlayerStart>(CurrentActor))
			{
				PlayerStart = Cast<APlayerStart>(CurrentActor);
				break;
			}
		}
	}

	if (ASciFiGameModeBase* GameModeRef = Cast<ASciFiGameModeBase>(GetWorld()->GetAuthGameMode()))
	{
		GameModeRef->SetPlayerStart(PlayerStart);
	}
}

void AProceduralLevelGenerationActor::OnElevatorBottomLoaded(ULevelStreamingProcedural* LoadedLevelStreamingProcedural, ULevel* LoadedLevel, FElevatorBottomInfo InElevatorBottomInfo)
{
	AInteractableActor_Elevator* ElevatorRef = nullptr;

	// Check if the level is valid (not null)
	if (LoadedLevel)
	{
		for (AActor* CurrentActor : LoadedLevel->Actors)
		{
			if (CurrentActor->ActorHasTag("Elevator"))
			{
				ElevatorRef = Cast<AInteractableActor_Elevator>(CurrentActor);
				break;
			}
		}
	}

	if (ElevatorRef)
	{
		// Set the elevation level of the elevator
		ElevatorRef->SetElevationLevels(InElevatorBottomInfo.ElevationLevels);

		if (ULevelStreamingProcedural* ElevatorTopLevelStreaming = InElevatorBottomInfo.ElevatorTopTileData.LevelInstanceRef)
		{
			ElevatorTopLevelStreaming->ElevatorTopInfo.ElevatorRef = ElevatorRef;
			ULevel* ElevatorTopLoadedLevel = ElevatorTopLevelStreaming->GetLoadedLevel();
			// If the top level of the elevator hasn't been loaded yet, wait for it to load before configuring the actors in the level instance
			if (!ElevatorTopLoadedLevel)
			{
				ElevatorTopLevelStreaming->OnElevatorTopLoaded.AddDynamic(this, &AProceduralLevelGenerationActor::OnElevatorTopLoaded);
			}
			else
			{
				OnElevatorTopLoaded(ElevatorTopLevelStreaming, ElevatorTopLoadedLevel, ElevatorTopLevelStreaming->ElevatorTopInfo);
			}
		}	
	}
}

void AProceduralLevelGenerationActor::OnElevatorTopLoaded(ULevelStreamingProcedural* LoadedLevelStreamingProcedural, ULevel* LoadedLevel, FElevatorTopInfo InElevatorTopInfo)
{
	// Get Top Terminal
	AInteractableActor_Terminal* TerminalRef = nullptr;

	if (LoadedLevel)
	{
		for (AActor* CurrentActor : LoadedLevel->Actors)
		{
			if (CurrentActor->ActorHasTag("Terminal") && CurrentActor->GetParentActor() == nullptr)
			{
				TerminalRef = Cast<AInteractableActor_Terminal>(CurrentActor);
				break;
			}
		}

		if (TerminalRef && InElevatorTopInfo.ElevatorRef)
		{
			TerminalRef->GetActivatableActorArray().Add(InElevatorTopInfo.ElevatorRef);
		}
	}
}

void AProceduralLevelGenerationActor::PreloadLevels()
{
	// Load Key Rooms
	TArray<FKeyTileData> KeyTileDataArray;
	LevelGenerationSettings.KeyRooms.GenerateValueArray(KeyTileDataArray);

	for (FKeyTileData CurrentKeyTileData : KeyTileDataArray)
	{
		LoadLevelsFromDataTable(CurrentKeyTileData.KeyRoomList);
	}

	// Load Special Rooms

	TArray<FSpecialTileData> SpecialTileDataArray;
	LevelGenerationSettings.SpecialRooms.GenerateValueArray(SpecialTileDataArray);

	for (FSpecialTileData CurrentSpecialTileData : SpecialTileDataArray)
	{
		LoadLevelsFromDataTable(CurrentSpecialTileData.SpecialRoomList);
	}

	// Load Basic Rooms

	TArray<UDataTable*> BasicRoomDataTableArray;
	LevelGenerationSettings.BasicRoomList.GenerateKeyArray(BasicRoomDataTableArray);

	for (UDataTable* CurrentDataTable : BasicRoomDataTableArray)
	{
		LoadLevelsFromDataTable(CurrentDataTable);
	}

	// Load Basic Corridors

	TArray<UDataTable*> CorridorLevelDataTableArray;
	LevelGenerationSettings.CorridorLevelDataTableList.GenerateValueArray(BasicRoomDataTableArray);

	for (UDataTable* CurrentDataTable : BasicRoomDataTableArray)
	{
		LoadLevelsFromDataTable(CurrentDataTable);
	}
	
	// Load Special Corridors

	TArray<ESpecialPathType> SpecialCorridorTypeArray;
	LevelGenerationSettings.SpecialPathLevelDataTableList.GenerateKeyArray(SpecialCorridorTypeArray);

	for (ESpecialPathType CurrentSpecialPathType: SpecialCorridorTypeArray)
	{
		bool bOverride = false;

		if (CurrentSpecialPathType == ESpecialPathType::Elevator_Bottom || CurrentSpecialPathType == ESpecialPathType::Elevator_Middle || CurrentSpecialPathType == ESpecialPathType::Elevator_Top)
		{
			bOverride = true;
		}
		
		if (LevelGenerationSettings.AllowedSpecialPathTypes.Contains(CurrentSpecialPathType))
		{
			if (LevelGenerationSettings.AllowedSpecialPathTypes[CurrentSpecialPathType])
			{
				if (UDataTable* CurrentDataTable = LevelGenerationSettings.SpecialPathLevelDataTableList[CurrentSpecialPathType])
				{
					LoadLevelsFromDataTable(CurrentDataTable);
				}
			}
		}
		else if (bOverride)
		{
			if (UDataTable* CurrentDataTable = LevelGenerationSettings.SpecialPathLevelDataTableList[CurrentSpecialPathType])
			{
				LoadLevelsFromDataTable(CurrentDataTable);
			}
		}
	}
}

void AProceduralLevelGenerationActor::PopulateLevel()
{
	TArray<FIntVector> CoordinateArray;
	GeneratedLevelData.LevelTileData.GenerateKeyArray(CoordinateArray);

	for (FIntVector CurrentCoordinate : CoordinateArray)
	{
		if (GeneratedLevelData.LevelTileData[CurrentCoordinate].TileType == ETileType::Room_Section) { continue; }

		FTileData& RoomTileData = GeneratedLevelData.LevelTileData[CurrentCoordinate];

		// Create instance for the base map
		CreateProceduralLevelInstance(CurrentCoordinate, RoomTileData, RoomTileData.TileMap);

		// Create instances for submaps
		for (TSoftObjectPtr<UWorld> CurrentSubMap : GeneratedLevelData.LevelTileData[CurrentCoordinate].TileSubMaps)
		{
			CreateProceduralLevelInstance(CurrentCoordinate, RoomTileData, CurrentSubMap);
		}

		// Create instances for actor slot maps
		TArray<EActorSlotType> ActorSlotTypeArray;
		RoomTileData.TileActorSlotMaps.GenerateKeyArray(ActorSlotTypeArray);

		for (EActorSlotType CurrentActorSlotType : ActorSlotTypeArray)
		{
			TSoftObjectPtr<UWorld> CurrentActorSlotMap = RoomTileData.TileActorSlotMaps[CurrentActorSlotType];
			CreateProceduralLevelInstance(CurrentCoordinate, RoomTileData, CurrentActorSlotMap, CurrentActorSlotType);
		}

		/*
		const FName RoomName = FName(GeneratedLevelData.LevelTileData[CurrentCoordinate].TileMap.GetAssetName());
		const FName RoomFullPackageName = FName(GeneratedLevelData.LevelTileData[CurrentCoordinate].TileMap.GetLongPackageName());// GeneratedLevelData.LevelTileData[CurrentCoordinate].LevelFilePath.IsNone() ? RoomName : FName(GeneratedLevelData.LevelTileData[CurrentCoordinate].LevelFilePath.ToString() + GeneratedLevelData.LevelTileData[CurrentCoordinate].LevelName.ToString());
		const FRotator RoomRotation = GeneratedLevelData.LevelTileData[CurrentCoordinate].TileRotation;
		const FVector RoomLocation = (FVector)CurrentCoordinate * LevelGenerationSettings.TileSize;

		ULevelStreaming* StreamingLevel = nullptr;//UGameplayStatics::GetStreamingLevel(GetWorld(), RoomFullPackageName);

		if (LoadedLevelMap.Contains(RoomFullPackageName))
		{
			StreamingLevel = LoadedLevelMap[RoomFullPackageName];
		}

		if (!StreamingLevel) { continue; }

		FString InstanceName = "[" + FString::FromInt(CurrentCoordinate.X) + "x" + FString::FromInt(CurrentCoordinate.Y) + "x" + FString::FromInt(CurrentCoordinate.Z) + "] ";
		InstanceName.Append(RoomName.ToString());

		//ULevelStreaming* LevelInstance = StreamingLevel->CreateInstance(InstanceName);
		ULevelStreamingProcedural* ProceduralLevelInstance = nullptr;
		ProceduralLevelInstance = ProceduralLevelInstance->CreateProceduralInstance(GetWorld(), StreamingLevel, InstanceName);

		if (ProceduralLevelInstance)
		{
			ProceduralLevelInstance->LevelTransform = FTransform(RoomRotation, RoomLocation, FVector{ 1.f, 1.f, 1.f });
			ProceduralLevelInstance->SetShouldBeLoaded(true);
			ProceduralLevelInstance->SetShouldBeVisible(true);

			GeneratedLevelData.LevelTileData[CurrentCoordinate].LevelInstanceRef = ProceduralLevelInstance;

			ProceduralLevelInstance->LevelTileData = GeneratedLevelData.LevelTileData[CurrentCoordinate];
			ProceduralLevelInstance->OnDoorSlotLevelLevelLoaded.AddDynamic(this, &AProceduralLevelGenerationActor::OnDoorSlotLevelLevelLoaded);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("ULevelGenerationLibrary::PopulateLevel LevelInstance is nullptr!"));
		}
		*/
	}
	bLevelWaitingToLoad = true;
}

void AProceduralLevelGenerationActor::BuildMinimap()
{
	float MinimapGridSize = LevelGenerationSettings.TileSize * LevelGenerationSettings.MinimapScale;

	// Build the minimap in a location that won't collide with the generated level
	MinimapLocation->SetWorldLocation(FVector(MinimapGridSize * .5f, MinimapGridSize * .5f, -(LevelGenerationSettings.TileSize* LevelGenerationSettings.GridSize.Z)));

	TArray<FIntVector> CoordinateArray;
	GeneratedLevelData.LevelTileData.GenerateKeyArray(CoordinateArray);

	// Build minimap rooms
	for (FIntVector CurrentCoordinate : CoordinateArray)
	{
		// Create static mesh component
		const FString MeshString = "MinimapMesh_Room_" + FString::FromInt(LevelMinimap.Num());
		
		UStaticMeshComponent* CurrentMinimapMesh = CreateMinimapMesh(GeneratedLevelData.LevelTileData[CurrentCoordinate].MinimapMesh, MeshString);
		if (!CurrentMinimapMesh) { continue; }

		FMinimapInfo_Room CurrentMinimapRoomInfo;
		CurrentMinimapRoomInfo.MinimapMesh = CurrentMinimapMesh;
		CurrentMinimapRoomInfo.RoomRotation = GeneratedLevelData.LevelTileData[CurrentCoordinate].TileRotation;

		// Add to minimap room array
		LevelMinimap.Add(CurrentCoordinate, CurrentMinimapRoomInfo);
	}

	TArray<FIntVector> MinimapCoordinateArray;
	LevelMinimap.GenerateKeyArray(MinimapCoordinateArray);

	// Set the transform of the minimap rooms
	for (FIntVector CurrentCoordinate : MinimapCoordinateArray)
	{
		FMinimapInfo_Room CurrentMinimapRoomInfo = LevelMinimap[CurrentCoordinate];

		FVector MeshLocation{ CurrentCoordinate.X * MinimapGridSize, CurrentCoordinate.Y * MinimapGridSize, CurrentCoordinate.Z * MinimapGridSize };

		CurrentMinimapRoomInfo.MinimapMesh->SetWorldRotation(CurrentMinimapRoomInfo.RoomRotation);
		CurrentMinimapRoomInfo.MinimapMesh->SetRelativeLocation(MeshLocation);
	}

	// Build minimap meshes for interactables (excluding doors)
	TArray<AActor*> InteractableActorArray;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AInteractableActor_Base::StaticClass(), InteractableActorArray);

	for (AActor* CurrentActor : InteractableActorArray)
	{
		AInteractableActor_Door* IsInteractableDoor = Cast<AInteractableActor_Door>(CurrentActor);
		if (IsInteractableDoor) { continue; }

		// Create minimap mesh
		AInteractableActor_Base* CurrentInteractable = Cast<AInteractableActor_Base>(CurrentActor);

		// Skip actor if it does not have a minimap mesh set
		if (!CurrentInteractable->GetMinimapMesh()) { continue; }

		const FString MeshString = "MinimapMesh_Interactable_" + FString::FromInt(MinimapInteractables.Num() + 1);
		const TArray<FName> MeshTags{ "MinimapPaintFill" };

		UStaticMeshComponent* CurrentMinimapMesh = CreateMinimapMesh(CurrentInteractable->GetMinimapMesh(), MeshString, MeshTags);
		if (!CurrentMinimapMesh) { continue; }

		FMinimapInfo_Interactable CurrentMinimapInteractableInfo;
		CurrentMinimapInteractableInfo.MinimapMesh = CurrentMinimapMesh;
		const FTransform InteractableTransform = CurrentInteractable->GetActorTransform();
		CurrentMinimapInteractableInfo.SetInteractableTransform(InteractableTransform.GetLocation(), InteractableTransform.GetRotation().Rotator(), InteractableTransform.GetScale3D(), LevelGenerationSettings.MinimapScale, this);

		// Set the position of the door mesh component
		CurrentMinimapInteractableInfo.MinimapMesh->SetWorldTransform(CurrentMinimapInteractableInfo.InteractableTransform);

		MinimapInteractables.Add(CurrentInteractable, CurrentMinimapInteractableInfo);
	}

}

void AProceduralLevelGenerationActor::SetupElevators()
{
	UWorld* WorldRef = GetWorld();

	TArray<FIntVector> LevelPathDataKeyArray;
	GeneratedLevelData.LevelPathData.GenerateKeyArray(LevelPathDataKeyArray);

	TArray<FIntVector> ElevatorBottomArray;
	TArray<FIntVector> ElevatorTopArray;

	// Get All Elevator bottom pieces
	for (FIntVector CurrentCoordinate : LevelPathDataKeyArray)
	{
		const FCorridorTileData CurrentData = GeneratedLevelData.LevelPathData[CurrentCoordinate];

		if (((uint8)CurrentData.SpecialPathType >= (uint8)ESpecialPathType::Elevator_S2) && ((uint8)CurrentData.SpecialPathType < (uint8)ESpecialPathType::MAX))
		{
			const FIntVector ElevatorTopCoordinate = CurrentCoordinate + CurrentData.ParentPathNode.SpecialPathInfo.ExitVector;

			if (LevelPathDataKeyArray.Contains(ElevatorTopCoordinate))
			{
				ElevatorBottomArray.Add(CurrentCoordinate);
				ElevatorTopArray.Add(ElevatorTopCoordinate);
			}
		}
	}

	for (int i = 0; i < ElevatorBottomArray.Num(); i++)
	{
		const FIntVector BottomCoordinate = ElevatorBottomArray[i];
		const FIntVector TopCoordinate = ElevatorTopArray[i];

		if (ULevelStreamingProcedural* ElevatorBottomLevelInstance = GeneratedLevelData.LevelTileData[BottomCoordinate].LevelInstanceRef)
		{
			ElevatorBottomLevelInstance->ElevatorBottomInfo.ElevationLevels = TopCoordinate.Z - BottomCoordinate.Z;
			ElevatorBottomLevelInstance->ElevatorBottomInfo.ElevatorTopTileData = GeneratedLevelData.LevelTileData[TopCoordinate];
			ElevatorBottomLevelInstance->OnElevatorBottomLoaded.AddDynamic(this, &AProceduralLevelGenerationActor::OnElevatorBottomLoaded);
		}
	}
}

void AProceduralLevelGenerationActor::SetupDoors(ULevel* LoadedLevel, FTileData InLevelTileData)
{
	// Get all the door slots in the loaded level
	TArray<AActorSlot_Door*> DoorSlots;
	if (LoadedLevel)
	{
		for (AActor* CurrentActor : LoadedLevel->Actors)
		{
			if (AActorSlot_Door* CurrentDoorSlot = Cast<AActorSlot_Door>(CurrentActor))
			{
				DoorSlots.Add(CurrentDoorSlot);
			}
		}
	}
	

	TArray<FIntVector> AdjacentAccessPointArray;
	InLevelTileData.TileAccessPoints.GenerateKeyArray(AdjacentAccessPointArray);

	for (FIntVector CurrentAccessPointCoordinate : AdjacentAccessPointArray)
	{
		FTileAccessData CurrentAccessPoint = InLevelTileData.TileAccessPoints[CurrentAccessPointCoordinate];

		for (EDirections CurrentDirection : CurrentAccessPoint.AccessibleDirections)
		{
			// Access point in use? Add door
			if (CurrentAccessPoint.DirectionsInUse.Contains(CurrentDirection))
			{
				if (AActorSlot_Door* DoorSlot = FindDoorSlot(DoorSlots, CurrentAccessPointCoordinate, CurrentDirection))
				{
					DoorSlot->SetChildActorClass((TSubclassOf<AActor>)DoorSlot->GetDoorClass());
					DoorSlot->GetChildActorComponent()->CreateChildActor();

					if (DoorSlot->GetChildActor() && InLevelTileData.LevelInstanceRef)
					{
						// Create minimap door mesh
						AInteractableActor_Door* CurrentDoor = Cast<AInteractableActor_Door>(DoorSlot->GetChildActor());
						if (!CurrentDoor) { continue; }

						const FString MeshString = "MinimapMesh_Door_" + FString::FromInt(MinimapDoors.Num() + 1);
						const TArray<FName> MeshTags{ "MinimapPaintFill" };

						UStaticMeshComponent* CurrentMinimapMesh = CreateMinimapMesh(CurrentDoor->GetMinimapMesh(), MeshString, MeshTags);
						if (!CurrentMinimapMesh) { continue; }

						FMinimapInfo_Interactable CurrentMinimapDoorInfo;
						CurrentMinimapDoorInfo.MinimapMesh = CurrentMinimapMesh;
						
						const FVector DoorWorldLocation = InLevelTileData.LevelInstanceRef->LevelTransform.GetLocation() + ULevelGenerationLibrary::RotateVectorCoordinatefromOrigin(DoorSlot->GetRootComponent()->GetRelativeLocation(), InLevelTileData.LevelInstanceRef->LevelTransform.Rotator());
						const FTransform DoorMeshTransform = CurrentDoor->GetDoorMeshTransform();
						CurrentMinimapDoorInfo.SetInteractableTransform(DoorWorldLocation, DoorMeshTransform.GetRotation().Rotator(), DoorMeshTransform.GetScale3D(), LevelGenerationSettings.MinimapScale, this);

						// Set the position of the door mesh component
						CurrentMinimapDoorInfo.MinimapMesh->SetWorldTransform(CurrentMinimapDoorInfo.InteractableTransform);

						MinimapDoors.Add(CurrentDoor, CurrentMinimapDoorInfo);
					}
				}
			}
			// Else add wall
			else
			{
				if (AActorSlot_Door* DoorSlot = FindDoorSlot(DoorSlots, CurrentAccessPointCoordinate, CurrentDirection))
				{
					DoorSlot->SetChildActorClass(DoorSlot->GetWallClass());
					DoorSlot->GetChildActorComponent()->CreateChildActor();

					if (DoorSlot->GetChildActor() && InLevelTileData.LevelInstanceRef)
					{
						// Create minimap access blocker mesh
						AActor* CurrentAccessBlocker = DoorSlot->GetChildActor();

						const FString MeshString = "MinimapMesh_AccessBlocker_" + FString::FromInt(MinimapAccessBlockers.Num() + 1);

						UStaticMeshComponent* CurrentMinimapMesh = CreateMinimapMesh(DoorSlot->GetWallMinimapMesh(), MeshString);

						FMinimapInfo_Interactable CurrentMinimapDoorInfo;
						CurrentMinimapDoorInfo.MinimapMesh = CurrentMinimapMesh;

						const FVector DoorWorldLocation = InLevelTileData.LevelInstanceRef->LevelTransform.GetLocation() + ULevelGenerationLibrary::RotateVectorCoordinatefromOrigin(DoorSlot->GetRootComponent()->GetRelativeLocation(), InLevelTileData.LevelInstanceRef->LevelTransform.Rotator());
						const FTransform DoorMeshTransform = DoorSlot->GetActorTransform();
						CurrentMinimapDoorInfo.SetInteractableTransform(DoorWorldLocation, (DoorMeshTransform.GetRotation().Rotator() + InLevelTileData.LevelInstanceRef->LevelTransform.Rotator()), DoorMeshTransform.GetScale3D(), LevelGenerationSettings.MinimapScale, this);

						// Set the position of the access blocker mesh component
						CurrentMinimapDoorInfo.MinimapMesh->SetWorldTransform(CurrentMinimapDoorInfo.InteractableTransform);

						MinimapAccessBlockers.Add(CurrentAccessBlocker, CurrentMinimapDoorInfo);
					}
				}
			}
		}

	}


}

AActorSlot_Door* AProceduralLevelGenerationActor::FindDoorSlot(const TArray<AActorSlot_Door*>& DoorSlotArray, FIntVector AccessPointCoordinate, EDirections AccessPointDirection)
{
	for (AActorSlot_Door* CurrentDoorSlot : DoorSlotArray)
	{
		if (CurrentDoorSlot->GetAccessPointCoordinate() == AccessPointCoordinate && CurrentDoorSlot->GetAccessPointDirection() == AccessPointDirection)
		{
			return CurrentDoorSlot;
		}
	}

	return nullptr;
}

void AProceduralLevelGenerationActor::LoadLevelsFromDataTable(UDataTable* InDataTable)
{
	if (InDataTable)
	{
		TArray<FName> RowNames = InDataTable->GetRowNames();

		for (FName CurrentRowName : RowNames)
		{
			if (FTileGenerationData* TileGenerationData = InDataTable->FindRow<FTileGenerationData>(CurrentRowName, ""))
			{
				const FName RoomFullPackageName = FName(TileGenerationData->TileData.TileMap.GetLongPackageName());
				if (!RoomFullPackageName.IsNone()) { LoadLevel(RoomFullPackageName); }

				// Load submaps
				for (TSoftObjectPtr<UWorld> CurrentSubMap : TileGenerationData->TileData.TileSubMaps)
				{
					const FName FullPackageName = FName(CurrentSubMap.GetLongPackageName());
					if (!FullPackageName.IsNone()) { LoadLevel(FullPackageName); }
				}

				// Load actor slot maps
				TArray<EActorSlotType> ActorSlotTypeArray;
				TileGenerationData->TileData.TileActorSlotMaps.GenerateKeyArray(ActorSlotTypeArray);

				for (EActorSlotType CurrentActorSlotType : ActorSlotTypeArray)
				{
					TSoftObjectPtr<UWorld> CurrentActorSlotMap = TileGenerationData->TileData.TileActorSlotMaps[CurrentActorSlotType];

					const FName FullPackageName = FName(CurrentActorSlotMap.GetLongPackageName());
					if (!FullPackageName.IsNone()) { LoadLevel(FullPackageName); }
				}


			}
			else if (FCorridorLevelData* CorridorLevelData = InDataTable->FindRow<FCorridorLevelData>(CurrentRowName, ""))
			{
				const FName CorridorFullPackageName = FName(CorridorLevelData->CorridorMap.GetLongPackageName());
				if (!CorridorFullPackageName.IsNone()) { LoadLevel(CorridorFullPackageName); }

				// Load submaps
				for (TSoftObjectPtr<UWorld> CurrentSubMap : CorridorLevelData->CorridorSubMaps)
				{
					const FName FullPackageName = FName(CurrentSubMap.GetLongPackageName());
					if (!FullPackageName.IsNone()) { LoadLevel(FullPackageName); }
				}

				// Load actor slot maps
				TArray<EActorSlotType> ActorSlotTypeArray;
				CorridorLevelData->CorridorActorSlotMaps.GenerateKeyArray(ActorSlotTypeArray);

				for (EActorSlotType CurrentActorSlotType : ActorSlotTypeArray)
				{
					TSoftObjectPtr<UWorld> CurrentActorSlotMap = CorridorLevelData->CorridorActorSlotMaps[CurrentActorSlotType];

					const FName FullPackageName = FName(CurrentActorSlotMap.GetLongPackageName());
					if (!FullPackageName.IsNone()) { LoadLevel(FullPackageName); }
				}
			}
		}
	}
}

void AProceduralLevelGenerationActor::LoadLevel(const FName& LevelName)
{
	ULevelStreamingDynamic* StreamedLevel = Cast<ULevelStreamingDynamic>(UGameplayStatics::GetStreamingLevel(GetWorld(), LevelName));

	if (StreamedLevel)
	{
		LoadedLevelMap.Emplace(LevelName, StreamedLevel);
		return;
	}

	ULevelStreamingDynamic::FLoadLevelInstanceParams Params(GetWorld(), LevelName.ToString(), FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector(1.f, 1.f, 1.f)));

	bool bSuccess = false;
	ULevelStreamingDynamic* StreamingLevel = ULevelStreamingDynamic::LoadLevelInstance(Params, bSuccess);
	if (StreamingLevel)
	{
		StreamingLevel->SetShouldBeLoaded(false);
		StreamingLevel->SetShouldBeVisible(false);

		LoadedLevelMap.Emplace(LevelName, StreamingLevel);
	}
}

void AProceduralLevelGenerationActor::CreateProceduralLevelInstance(FIntVector CurrentCoordinate, FTileData& RoomTileData, TSoftObjectPtr<UWorld> MapToInstance, EActorSlotType MapSlotType)
{
	const FName RoomName = FName(MapToInstance.GetAssetName());
	const FName RoomFullPackageName = FName(MapToInstance.GetLongPackageName());
	const FRotator RoomRotation = RoomTileData.TileRotation;
	const FVector RoomLocation = (FVector)CurrentCoordinate * LevelGenerationSettings.TileSize;

	ULevelStreaming* StreamingLevel = nullptr;

	if (LoadedLevelMap.Contains(RoomFullPackageName))
	{
		StreamingLevel = LoadedLevelMap[RoomFullPackageName];
	}

	if (!StreamingLevel) { return; }

	FString InstanceName = "[" + FString::FromInt(CurrentCoordinate.X) + "x" + FString::FromInt(CurrentCoordinate.Y) + "x" + FString::FromInt(CurrentCoordinate.Z) + "] ";
	InstanceName.Append(RoomName.ToString());

	ULevelStreamingProcedural* ProceduralLevelInstance = nullptr;
	ProceduralLevelInstance = ProceduralLevelInstance->CreateProceduralInstance(GetWorld(), StreamingLevel, InstanceName);

	if (ProceduralLevelInstance)
	{
		LevelsLeftToLoad.Add(ProceduralLevelInstance);

		ProceduralLevelInstance->LevelTransform = FTransform(RoomRotation, RoomLocation, FVector{ 1.f, 1.f, 1.f });
		ProceduralLevelInstance->SetShouldBeLoaded(true);
		ProceduralLevelInstance->SetShouldBeVisible(true);

		if (MapToInstance == RoomTileData.TileMap)
		{
			RoomTileData.LevelInstanceRef = ProceduralLevelInstance;
		}

		ProceduralLevelInstance->LevelTileData = RoomTileData;
		switch (MapSlotType)
		{
		case EActorSlotType::Door:
			ProceduralLevelInstance->OnDoorSlotLevelLevelLoaded.AddDynamic(this, &AProceduralLevelGenerationActor::OnDoorSlotLevelLevelLoaded);
			break;

		case EActorSlotType::PlayerSpawn:
			ProceduralLevelInstance->OnPlayerSpawnRoomLoaded.AddDynamic(this, &AProceduralLevelGenerationActor::OnPlayerSpawnRoomLoaded);
			break;

		default:
			break;
		}
		ProceduralLevelInstance->OnProceduralLevelLoaded.AddDynamic(this, &AProceduralLevelGenerationActor::OnProceduralLevelLoaded);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ULevelGenerationLibrary::PopulateLevel LevelInstance is nullptr!"));
	}
}

UStaticMeshComponent* AProceduralLevelGenerationActor::CreateMinimapMesh(UStaticMesh* MinimapMesh, const FString MeshName, const TArray<FName> MeshTags)
{
	if (!MinimapMesh) { return nullptr; }

	UStaticMeshComponent* NewMinimapMesh = NewObject<UStaticMeshComponent>(this, UStaticMeshComponent::StaticClass(), FName(MeshName));
	NewMinimapMesh->RegisterComponent();
	NewMinimapMesh->AttachToComponent(MinimapLocation, FAttachmentTransformRules::SnapToTargetIncludingScale);
	NewMinimapMesh->CreationMethod = EComponentCreationMethod::Instance;
	NewMinimapMesh->SetStaticMesh(MinimapMesh);
	NewMinimapMesh->SetCollisionProfileName("MinimapMesh");
	NewMinimapMesh->SetRenderCustomDepth(true);
	NewMinimapMesh->SetCustomDepthStencilValue(0);
	if (!MeshTags.IsEmpty()) { NewMinimapMesh->ComponentTags.Append(MeshTags); }

	return NewMinimapMesh;
}
