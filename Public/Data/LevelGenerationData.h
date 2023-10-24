// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Data/FunctionLibraries/KruskalMSTLibrary.h"
#include "LevelGenerationData.generated.h"

class ULevelStreaming;
class ULevelStreamingDynamic;
class ULevelStreamingProcedural;
class AInteractableActor_Elevator;

UENUM(BlueprintType, meta = (DisplayName = "Tile Type"))
enum class ETileType : uint8
{
	Empty				UMETA(DisplayName = "Empty Space"),
	Room_Basic			UMETA(DisplayName = "Basic Room"),
	Room_Key			UMETA(DisplayName = "Key Room"),
	Room_Special		UMETA(DisplayName = "Special Room"),
	Room_Section		UMETA(DisplayName = "Room Section"),
	Corridor			UMETA(DisplayName = "Corridor"),
	Corridor_Section	UMETA(DisplayName = "Corridor Section"),
	Corridor_Special	UMETA(DisplayName = "Special Corridor"),
	RoomBuffer			UMETA(DisplayName = "Room Buffer"),

	MAX					UMETA(Hidden)
};

UENUM(BlueprintType, meta = (DisplayName = "Corridor Type"))
enum class ECorridorType : uint8
{
	None						UMETA(Hidden),
	ZeroWay						UMETA(DisplayName = "Corridor Zero-Way"),
	OneWay						UMETA(DisplayName = "Corridor One-Way"),
	TwoWay						UMETA(DisplayName = "Corridor Two-Way"),
	ThreeWay					UMETA(DisplayName = "Corridor Three-Way"),
	FourWay						UMETA(DisplayName = "Corridor Four-Way"),
	Corner						UMETA(DisplayName = "Corridor Corner"),

	MAX							UMETA(Hidden)
};

UENUM(BlueprintType, meta = (DisplayName = "Biome Type"))
enum class EBiomeType : uint8
{
	Lab				UMETA(DisplayName = "Lab"),

	MAX				UMETA(Hidden)
};

UENUM(BlueprintType, meta = (DisplayName = "Directions"))
enum class EDirections : uint8
{
	None			UMETA(Hidden, DisplayName = "None"),
	North			UMETA(DisplayName = "North"),
	East			UMETA(DisplayName = "East"),
	South			UMETA(DisplayName = "South"),
	West			UMETA(DisplayName = "West"),
	Above			UMETA(DisplayName = "Above"),
	Below			UMETA(DisplayName = "Below"),

	MAX				UMETA(Hidden)
};

UENUM(BlueprintType)
enum class ESpecialPathType : uint8
{
	None				UMETA(Hidden),
	SpecialPathSection	UMETA(DisplayName = "Special Path Section"),
	Stairs_1x1x2		UMETA(DisplayName = "Vertial Stairs"),
	Stairs_1x2x2		UMETA(DisplayName = "Horizontal Stairs"),
	Elevator_Bottom		UMETA(DisplayName = "Elevator Bottom"),
	Elevator_Middle		UMETA(DisplayName = "Elevator Middle"),
	Elevator_Top		UMETA(DisplayName = "Elevator Top"),
	Elevator_S2			UMETA(DisplayName = "Elevator - 2 Storeys"),
	Elevator_S3			UMETA(DisplayName = "Elevator - 3 Storeys"),
	Elevator_S4			UMETA(DisplayName = "Elevator - 4 Storeys"),
	Elevator_S5			UMETA(DisplayName = "Elevator - 5 Storeys"),
	Elevator_S6			UMETA(DisplayName = "Elevator - 6 Storeys"),
	Elevator_S7			UMETA(DisplayName = "Elevator - 7 Storeys"),
	Elevator_S8			UMETA(DisplayName = "Elevator - 8 Storeys"),
	Elevator_S9			UMETA(DisplayName = "Elevator - 9 Storeys"),
	Elevator_S10		UMETA(DisplayName = "Elevator - 10 Storeys"),

	MAX				UMETA(Hidden)

};

UENUM(BlueprintType)
enum class EActorSlotType : uint8
{
	None			UMETA(DisplayName = "None", Hidden),
	Door			UMETA(DisplayName = "Door Slot"),
	PlayerSpawn		UMETA(DisplayName = "Player Spawn Slot"),

	MAX				UMETA(Hidden)
};


/** Structure containing the A* Pathfinding information for a special path. */
USTRUCT(BlueprintType, meta = (DisplayName = "Special Path Data"))
struct FSpecialPathInfo : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY(EditAnywhere)
	TSet<FIntVector> PathVolume;
	UPROPERTY(EditAnywhere)
	FIntVector ExitVector;

	UPROPERTY(EditAnywhere)
	float NodeWeight = 0.f;

	FSpecialPathInfo()
	{
		Init();
	}

	FORCEINLINE void Init()
	{
		PathVolume.Empty();
		PathVolume.Add(FIntVector(0, 0, 0));
		ExitVector = FIntVector(0, 0, 0);
		NodeWeight = 0.f;
	}

};

/** Data asset to store the pathfinding information for all special path types. */
UCLASS(BlueprintType)
class PROJECTSCIFI_API USpecialPathData : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere)
		TMap<ESpecialPathType, FSpecialPathInfo> SpecialPathSettings;

};

/* Structure containing information about a node for A* Pathfinding. */
USTRUCT(BlueprintType)
struct FPathNode
{
	GENERATED_USTRUCT_BODY()

public:

	FIntVector ParentNode = { -1,-1,-1 };

	// Distance from starting node, the heuristic to the start location
	float GCost = 0.f;
	// Distance from end node, the heuristic to the end location
	float HCost = 0.f;
	// GCost + HCost
	float FCost = 0.f;

};

/* Structure containing more detailed information about a node for my customised A* Pathfinding. */
USTRUCT(BlueprintType)
struct FAdvancedPathNode : public FPathNode
{
	GENERATED_USTRUCT_BODY()

public:

	FAdvancedPathNode()
	{
		Init();
	}

	FAdvancedPathNode(FIntVector InParentCoordinate, float InGCost, float InHCost, float InFCost, ESpecialPathType InSpecialPathType, FSpecialPathInfo InSpecialPathInfo, FIntVector InSpecialPathOriginVector, FRotator InSpecialPathRotation, bool bPathGoingUp, int InElevationToEnd, TMap<FIntVector, FAdvancedPathNode*> InPreviousPath)
	{
		ParentNode = InParentCoordinate;
		GCost = InGCost;
		HCost = InHCost;
		FCost = InFCost;

		SpecialPathType = InSpecialPathType;
		SpecialPathInfo = InSpecialPathInfo;
		SpecialPathOriginVector = InSpecialPathOriginVector;
		SpecialPathRotation = InSpecialPathRotation;
		bIsPathReversed = bPathGoingUp;
		ElevationToEnd = InElevationToEnd;

		PreviousPath = InPreviousPath;

	}

	FORCEINLINE void Init()
	{
		PreviousPath.Empty();
	}

	UPROPERTY(EditAnywhere)
	ESpecialPathType SpecialPathType = ESpecialPathType::None;

	UPROPERTY(EditAnywhere)
	FSpecialPathInfo SpecialPathInfo;

	UPROPERTY(EditAnywhere)
	FIntVector SpecialPathOriginVector = FIntVector::ZeroValue;

	UPROPERTY(EditAnywhere)
	FRotator SpecialPathRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere)
	bool bIsPathReversed = true;

	UPROPERTY(EditAnywhere)
	int ElevationToEnd = 0.f;

	TMap<FIntVector, FAdvancedPathNode*> PreviousPath;

};

/** Structure containing information about a tile's accessible directions and which ones are in use. */
USTRUCT(BlueprintType, meta = (DisplayName = "Tile Access Data"))
struct FTileAccessData
{
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSet<EDirections> AccessibleDirections;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSet<EDirections> DirectionsInUse;

};

/** Structure containing information about a room used in the level generation. */
USTRUCT(BlueprintType, meta = (DisplayName = "Tile Data"))
struct FTileData
{
	GENERATED_USTRUCT_BODY()

public:

	/** Reference to the level instance of this room. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	ULevelStreamingProcedural* LevelInstanceRef = nullptr;

	/** Level reference for the room */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSoftObjectPtr<UWorld> TileMap;

	/** Level references for the sublevels of this room. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<TSoftObjectPtr<UWorld>> TileSubMaps;

	/** Level references for the actor slot maps of this room. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TMap< EActorSlotType, TSoftObjectPtr<UWorld>> TileActorSlotMaps;

	/** The room's type. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	ETileType TileType = ETileType::Room_Basic;

	/** Used by room sections to know where the centre of the room is. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FIntVector ParentRoomCoordinate;

	/** The rotation of the room in world space. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (EditCondition = "bTileHasSetRotation", DisplayAfter = "bTileHasSetRotation", EditConditionHides))
	FRotator TileRotation = FRotator::ZeroRotator;

	/** The size of the room in the level grid space. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSet<FIntVector> TileSize;

	/** The sections of the room which have access points that can lead to other areas of the level. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TMap<FIntVector, FTileAccessData> TileAccessPoints;

	/** Mesh for the room's representation in the minimap. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UStaticMesh* MinimapMesh = nullptr;

};

/* Structure containing information about a corridor used in the level generation. */
USTRUCT(BlueprintType, meta = (DisplayName = "Corridor Tile Data"))
struct FCorridorTileData
{
	GENERATED_BODY()
public:

	/** The corridor's type. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "TileType", MakeStructureDefaultValue = "Corridor"))
	ETileType TileType;

	/** The corridor's special path type, if any. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "SpecialPathType"))
	ESpecialPathType SpecialPathType = ESpecialPathType::None;

	/** The corridor's rotation in world space. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "SpecialPathRotation"))
	FRotator SpecialPathRotation = FRotator::ZeroRotator;

	/** The size of the corridor in the level grid space. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "SpecialPathTileSize"))
	TSet<FIntVector> SpecialPathTileSize;

	/** Used by corridor sections to know where the centre of the corridor is. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "ParentPathNode"))
	FAdvancedPathNode ParentPathNode;

	/** The sections of the corridor which have access points that can lead to other areas of the level. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "AdjacentAccessPoints"))
	TMap<EDirections, ETileType> AdjacentAccessPoints;

	/** Mesh for the corridor's representation in the minimap. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UStaticMesh* MinimapMesh = nullptr;

};

/** Structure containing information about a key room to be used in the level generation. */
USTRUCT(BlueprintType)
struct FKeyTileData
{
	GENERATED_BODY()
public:
	/** The total amount times this key room needs to be generated. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "Quantity", MakeStructureDefaultValue = "1"))
	int32 Quantity;

	/** Reference to a data table containing the key room and its variations. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "KeyRoomList"))
	TObjectPtr<UDataTable> KeyRoomList;
};

/** Structure containing information about a special room to be used in the level generation. */
USTRUCT(BlueprintType)
struct FSpecialTileData
{
	GENERATED_BODY()
public:
	/** Chance for this special room to be generated. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "ChanceToGenerate", MakeStructureDefaultValue = "1.0", ClampMin = "0.0", ClampMax = "1.0"))
	double ChanceToGenerate;

	/** Reference to a data table containing the special room and its variations. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "SpecialRoomList", MakeStructureDefaultValue = "None"))
	TObjectPtr<UDataTable> SpecialRoomList;
};

/** Structure containing the information needed to generate a room in the level generation. */
USTRUCT(BlueprintType, meta = (DisplayName = "Tile Generation Data"))
struct FTileGenerationData : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

public:

	/** The chance for this room to be chosen for generation. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float RandomSelectionChance = 1.f;

	/** If enabled, the room will generate with a set world rotation. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bTileHasSetRotation = false;

	/** The room's set rotation in world space, will not be used if bTileHasSetRotation is false. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (EditCondition = "bTileHasSetRotation", DisplayAfter = "bTileHasSetRotation", EditConditionHides))
	FRotator TileSetRotation = FRotator::ZeroRotator;

	/** If enabled, the room will generate with a set location in the level grid. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bTileHasSetCoordinate = false;

	/** The room's set location in the level grid, will not be used if bTileHasSetCoordinate is false.  */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (EditCondition = "bTileHasSetCoordinate", DisplayAfter = "bTileHasSetCoordinate", EditConditionHides))
	FIntVector TileSetGridCoordinate = FIntVector::ZeroValue;

	/** The room to be generated. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FTileData TileData;
	
};

/** Structure containing the information needed to generate a corridor in the level generation. */
USTRUCT(BlueprintType, meta = (DisplayName = "Corridor Level Data"))
struct FCorridorLevelData : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

public:

	/** The chance for this corridor to be chosen for generation. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float RandomSelectionChance = 1.f;

	/** The corridor to be generated. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSoftObjectPtr<UWorld> CorridorMap;

	/** Level references for the sublevels of this corridor. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<TSoftObjectPtr<UWorld>> CorridorSubMaps;

	/** Level references for the actor slot maps of this corridor. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TMap< EActorSlotType, TSoftObjectPtr<UWorld>> CorridorActorSlotMaps;

	/** The sections of the corridor which have access points that can lead to other areas of the level. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TMap<FIntVector, FTileAccessData> CorridorAccessPoints;

	/** Mesh for the corridor's representation in the minimap. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UStaticMesh* MinimapMesh = nullptr;

};

/** Structure containing all the settings needed to generate a random level. */
USTRUCT(BlueprintType)
struct FLevelGenerationSettings : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()
public:

	/** Not in use yet. */
	//UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "LevelBiome", MakeStructureDefaultValue = "Lab"))
	//EBiomeType LevelBiome;

	/** If enabled, the level will be generated using a predefined seed. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "bUsePlayerSeed", MakeStructureDefaultValue = "False"))
	bool bUsePlayerSeed;

	/** The predefined seed used to in the level's generation, will not be used if bUsePlayerSeed is false.  */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "LevelUserSeed", MakeStructureDefaultValue = "0" , EditCondition = "bUsePlayerSeed", DisplayAfter = "bUsePlayerSeed", EditConditionHides))
	int32 LevelUserSeed;

	/** The standard size for a tile in the level grid. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "TileSize", MakeStructureDefaultValue = "1000"))
	int32 TileSize;

	/** The size of the level grid to be used by the level generator. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "GridSize", MakeStructureDefaultValue = "(X=0,Y=0,Z=0)"))
	FIntVector GridSize;

	/** The scale of the minimap compared to the level grid. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "MinimapScale"))
	float MinimapScale = 0.2f;

	/** Map of all the basic corridor tiles to be used in the level generation. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "CorridorLevelDataTableList"), Category = "Corridors")
	TMap<ECorridorType, UDataTable*> CorridorLevelDataTableList;

	/** Map of all the enabled special path types that can be used in the level generation. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "AllowedSpecialPathTypes"), Category = "Corridors")
	TMap<ESpecialPathType, bool> AllowedSpecialPathTypes;

	/** Reference to a data asset storing the information of the special path types for use in the A* Pathfinding. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "SpecialPathSettings"), Category = "Corridors")
	TSoftObjectPtr<USpecialPathData> SpecialPathData;

	/** Map of all the special corridor tiles that can be used in the level generation. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "SpecialPathLevelDataTableList"), Category = "Corridors")
	TMap<ESpecialPathType, UDataTable*> SpecialPathLevelDataTableList;

	/** Percentage chance to add a non-essential path to the MST. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "ExtraCorridorChance", MakeStructureDefaultValue = "0.2f"), Category = "Corridors")
	float ExtraCorridorChance = 0.2f;

	/** Map of the node weights for each room/corridor type in the A* Pathfinding. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "TileTypeWeight"), Category = "Corridors")
	TMap<ETileType, float> TileTypeWeight;

	/** Map of the basic rooms to be used in the level generation. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "BasicRoomList", MakeStructureDefaultValue = "()"), Category = "Rooms")
	TMap<UDataTable*, double> BasicRoomList;

	/** The size of the buffer surrounding rooms in the level grid, preventing other rooms from being generated inside that buffer. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "RoomBufferSize", MakeStructureDefaultValue = "1"), Category = "Rooms")
	int32 RoomBufferSize = 1;
	
	/** The minimum amount of basic rooms to be generated in the level. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "BasicRoomsMinimum", MakeStructureDefaultValue = "1"), Category = "Rooms")
	int32 BasicRoomsMinimum;

	/** The maximum amount of basic rooms to be generated in the level. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "BasicRoomsMaximum", MakeStructureDefaultValue = "1"), Category = "Rooms")
	int32 BasicRoomsMaximum;

	/** Map of the key rooms to be used in the level generation. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "KeyRooms", MakeStructureDefaultValue = "()"), Category = "Rooms")
	TMap<FName, FKeyTileData> KeyRooms;

	/** Map of the special rooms to be used in the level generation. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "SpecialRooms", MakeStructureDefaultValue = "()"), Category = "Rooms")
	TMap<FName, FSpecialTileData> SpecialRooms;

	/** If enabled, key rooms will be generated in the level. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "Generate Key Rooms"), Category = "Debug")
	bool bGenerateKeyRooms = true;

	/** If enabled, special rooms will be generated in the level. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "Generate Special Rooms"), Category = "Debug")
	bool bGenerateSpecialRooms = true;

	/** If enabled, basic rooms will be generated in the level. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "Generate Basic Rooms"), Category = "Debug")
	bool bGenerateBasicRooms = true;

	/** If enabled, the MST (and any additional paths) will be displayed in the level. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "Draw MST"), Category = "Debug")
	bool bDrawMST = false;

	/** If enabled, corridors will be generated in the level. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "Generate Corridors"), Category = "Debug")
	bool bGenerateCorridors= true;
};

/** Structure containing all the information created during level generation. */
USTRUCT(BlueprintType)
struct FGeneratedLevelData
{
	GENERATED_BODY()
public:
	/** The seed of the level, determines how randomisation is handled in the level generation. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "LevelStream", MakeStructureDefaultValue = "(InitialSeed=0,Seed=0)"))
	FRandomStream LevelStream;

	/** Map containing all the tiles in the level grid. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "LevelTileData", MakeStructureDefaultValue = "()"))
	TMap<FIntVector, FTileData> LevelTileData;

	/** Map containing all the paths in the level grid, used in the A* Pathfinding. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "LevelPathData", MakeStructureDefaultValue = "()"))
	TMap<FIntVector, FCorridorTileData> LevelPathData;

	/** List of all the edges in the Minimum Spanning Tree. */
	TArray<FEdgeInfo> MinimumSpanningTree;
};

/** Structure containing information needed to set up the bottom of an elevator shaft. */
USTRUCT(BlueprintType)
struct FElevatorBottomInfo
{
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY(BlueprintReadWrite)
	int ElevationLevels = 0;

	UPROPERTY(BlueprintReadWrite)
	FTileData ElevatorTopTileData;

};

/** Structure containing information needed to set up the top of an elevator shaft. */
USTRUCT(BlueprintType)
struct FElevatorTopInfo
{
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY(BlueprintReadWrite)
	AInteractableActor_Elevator* ElevatorRef = nullptr;

};

/** Structure containing information needed to set up the minimap. */
USTRUCT(BlueprintType)
struct FMinimapInfo
{
	GENERATED_USTRUCT_BODY()

public:

	UStaticMeshComponent* MinimapMesh = nullptr;

};

/** Derived FMinimapInfo structure containing additional information about the minimap room. */
USTRUCT(BlueprintType)
struct FMinimapInfo_Room : public FMinimapInfo
{
	GENERATED_USTRUCT_BODY()

public:

	FRotator RoomRotation = FRotator::ZeroRotator;

};

/** Derived FMinimapInfo structure containing additional information about the minimap interactable. */
USTRUCT(BlueprintType)
struct FMinimapInfo_Interactable : public FMinimapInfo
{
	GENERATED_USTRUCT_BODY()

public:

	void SetInteractableTransform(const FVector& Location, const FRotator& Rotation, const FVector& Scale3D, const float MinimapScale, AActor* MinimapActor);

public:

	FTransform InteractableTransform{FRotator::ZeroRotator, FVector::ZeroVector, FVector(1.f, 1.f, 1.f)};

};