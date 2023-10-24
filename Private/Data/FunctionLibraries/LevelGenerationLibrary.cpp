// Fill out your copyright notice in the Description page of Project Settings.


#include "Data/FunctionLibraries/LevelGenerationLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/DataTableFunctionLibrary.h"
#include "Engine/LevelStreaming.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/StreamableManager.h"
#include "Data/FunctionLibraries/DelaunayTriangulationLibrary.h"
#include "Data/FunctionLibraries/KruskalMSTLibrary.h"
#include "Data/LevelGenerationData.h"

// TMap containing the coordinates for each cardinal direction.
static TMap<EDirections, FIntVector> DirectionCoordinates
{
	{EDirections::None,		FIntVector(0,0,0)},
	{EDirections::North,	FIntVector(1,0,0)},
	{EDirections::East,		FIntVector(0,1,0)},
	{EDirections::South,	FIntVector(-1,0,0)},
	{EDirections::West,		FIntVector(0,-1,0)},
	{EDirections::Above,	FIntVector(0, 0, 1)},
	{EDirections::Below,	FIntVector(0,0,-1)}
};

// Set of coordinates to check the buffer around a room.
static 	TSet<FIntVector> CoordinateChecklist
{
	{ 0, 0, 0 },
	{ 1, 0, 0 },
	{ 1, 1, 0 },
	{ 0, 1, 0 },
	{ -1, 1, 0 },
	{ -1, 0, 0 },
	{ -1, -1, 0 },
	{ 0, -1, 0 },
	{ 1, -1, 0 },
	{ 1, 0, 1 },
	{ 1, 1, 1 },
	{ 0, 1, 1 },
	{ -1, 1, 1 },
	{ -1, 0, 1 },
	{ -1, -1, 1 },
	{ 0, -1, 1},
	{ 1, -1, 1},
	{ 0, 0, -1},
	{ 1, 0, -1 },
	{ 1, 1, -1},
	{ 0, 1, -1 },
	{ -1, 1, -1 },
	{ -1, 0, -1 },
	{ -1, -1, -1 },
	{ 0, -1, -1 },
	{ 1, -1, -1 },
};

void ULevelGenerationLibrary::GenerateLevel(FLevelGenerationSettings& LevelGenerationSettings, FGeneratedLevelData& GeneratedLevelData, UObject* WorldRef)
{
	// Set the level seed
	if (LevelGenerationSettings.bUsePlayerSeed) { GeneratedLevelData.LevelStream = LevelGenerationSettings.LevelUserSeed; }
	else { GeneratedLevelData.LevelStream.GenerateNewSeed(); }

	LevelGenerationSettings.SpecialPathData.LoadSynchronous();

	UE_LOG(LogTemp, Warning, TEXT("ULevelGenerationLibrary::GenerateLevel Level Seed = %s"), *FString::FromInt(GeneratedLevelData.LevelStream.GetCurrentSeed()));

	if (LevelGenerationSettings.bGenerateKeyRooms)
	{
		GenerateKeyRooms(LevelGenerationSettings, GeneratedLevelData);
		UE_LOG(LogTemp, Warning, TEXT("ULevelGenerationLibrary::GenerateLevel Key Rooms Generated!"));
	}

	if (LevelGenerationSettings.bGenerateSpecialRooms)
	{
		GenerateSpecialRooms(LevelGenerationSettings, GeneratedLevelData);
		UE_LOG(LogTemp, Warning, TEXT("ULevelGenerationLibrary::GenerateLevel Special Rooms Generated!"));
	}

	if (LevelGenerationSettings.bGenerateBasicRooms)
	{
		GenerateBasicRooms(LevelGenerationSettings, GeneratedLevelData);
		UE_LOG(LogTemp, Warning, TEXT("ULevelGenerationLibrary::GenerateLevel Basic Rooms Generated!"));
	}

	GenerateCorridors3D(LevelGenerationSettings, GeneratedLevelData, WorldRef);
	UE_LOG(LogTemp, Warning, TEXT("ULevelGenerationLibrary::GenerateLevel Corridors Generated!"));
}

FIntVector ULevelGenerationLibrary::RotateIntVectorCoordinatefromOrigin(FIntVector InCoordinate, FRotator TileRotation)
{
	const int TotalRotations = (TileRotation.Yaw / 90.f);

	constexpr int COUNTER_ONCE = -1;
	constexpr int COUNTER_TWICE = -2;
	constexpr int COUNTER_THRICE = -3;
	constexpr int ONCE = 1;
	constexpr int TWICE = 2;
	constexpr int THRICE = 3;

	switch (TotalRotations)
	{
	case COUNTER_ONCE: // Rotate 90° anti-clockwise 
		return FIntVector(InCoordinate.Y, InCoordinate.X * -1.f, InCoordinate.Z);
		break;

	case COUNTER_TWICE: // Rotate 180° clockwise
		return FIntVector(InCoordinate.X * -1.f, InCoordinate.Y * -1.f, InCoordinate.Z);
		break;

	case COUNTER_THRICE: // Rotate 90° clockwise
		return FIntVector(InCoordinate.Y * -1.f, InCoordinate.X, InCoordinate.Z);
		break;

	case ONCE: // Rotate 90° clockwise 
		return FIntVector(InCoordinate.Y * -1.f, InCoordinate.X, InCoordinate.Z);
		break;

	case TWICE: // Rotate 180° clockwise
		return FIntVector(InCoordinate.X * -1.f, InCoordinate.Y * -1.f, InCoordinate.Z);
		break;

	case THRICE: // Rotate 90° anti-clockwise
		return FIntVector(InCoordinate.Y, InCoordinate.X * -1.f, InCoordinate.Z);
		break;

	default: // Don't rotate
		break;
	}

	return InCoordinate;
}

FVector ULevelGenerationLibrary::RotateVectorCoordinatefromOrigin(FVector InCoordinate, FRotator TileRotation)
{
	const int TotalRotations = (TileRotation.Yaw / 90.f);

	constexpr int COUNTER_ONCE = -1;
	constexpr int COUNTER_TWICE = -2;
	constexpr int COUNTER_THRICE = -3;
	constexpr int ONCE = 1;
	constexpr int TWICE = 2;
	constexpr int THRICE = 3;

	switch (TotalRotations)
	{
	case COUNTER_ONCE: // Rotate 90° anti-clockwise 
		return FVector(InCoordinate.Y, InCoordinate.X * -1.f, InCoordinate.Z);
		break;

	case COUNTER_TWICE: // Rotate 180° clockwise
		return FVector(InCoordinate.X * -1.f, InCoordinate.Y * -1.f, InCoordinate.Z);
		break;

	case COUNTER_THRICE: // Rotate 90° clockwise
		return FVector(InCoordinate.Y * -1.f, InCoordinate.X, InCoordinate.Z);
		break;

	case ONCE: // Rotate 90° clockwise 
		return FVector(InCoordinate.Y * -1.f, InCoordinate.X, InCoordinate.Z);
		break;

	case TWICE: // Rotate 180° clockwise
		return FVector(InCoordinate.X * -1.f, InCoordinate.Y * -1.f, InCoordinate.Z);
		break;

	case THRICE: // Rotate 90° anti-clockwise
		return FVector(InCoordinate.Y, InCoordinate.X * -1.f, InCoordinate.Z);
		break;

	default: // Don't rotate
		break;
	}

	return InCoordinate;
}

EDirections ULevelGenerationLibrary::RotateDirection(EDirections InDirection, FRotator InRotation)
{
	if (!DirectionCoordinates.Contains(InDirection)) { return EDirections::None; }

	const FIntVector RotatedDirectionVector = RotateIntVectorCoordinatefromOrigin(DirectionCoordinates[InDirection], InRotation);

	if (DirectionCoordinates.FindKey(RotatedDirectionVector))
	{
		return *DirectionCoordinates.FindKey(RotatedDirectionVector);
	}

	return EDirections::None;
}

void ULevelGenerationLibrary::GenerateKeyRooms(FLevelGenerationSettings& LevelGenerationSettings, FGeneratedLevelData& GeneratedLevelData)
{
	TArray<FKeyTileData> KeyTileDataArray;
	LevelGenerationSettings.KeyRooms.GenerateValueArray(KeyTileDataArray);

	for (FKeyTileData KeyTileData : KeyTileDataArray)
	{
		int RoomsToCreate = FMath::Clamp(KeyTileData.Quantity, 0, ((LevelGenerationSettings.GridSize.X * LevelGenerationSettings.GridSize.Y * LevelGenerationSettings.GridSize.Z) - GeneratedLevelData.LevelTileData.Num()));

		if (RoomsToCreate > 0)
		{
			for (int i = 0; i < RoomsToCreate; i++)
			{
				PlaceRoomInGrid(LevelGenerationSettings, GeneratedLevelData, KeyTileData.KeyRoomList);
			}
		}
	}
}

void ULevelGenerationLibrary::GenerateSpecialRooms(FLevelGenerationSettings& LevelGenerationSettings, FGeneratedLevelData& GeneratedLevelData)
{
	TArray<FSpecialTileData> SpecialTileDataArray;
	LevelGenerationSettings.SpecialRooms.GenerateValueArray(SpecialTileDataArray);

	for (FSpecialTileData SpecialTileData : SpecialTileDataArray)
	{
		const bool bIsThereSpaceToSpawnRoom = (LevelGenerationSettings.GridSize.X * LevelGenerationSettings.GridSize.Y * LevelGenerationSettings.GridSize.Z) - GeneratedLevelData.LevelTileData.Num() > 0.f;

		if (bIsThereSpaceToSpawnRoom && UKismetMathLibrary::RandomFloatInRangeFromStream(0.f, 1.f, GeneratedLevelData.LevelStream) <= SpecialTileData.ChanceToGenerate)
		{
			PlaceRoomInGrid(LevelGenerationSettings, GeneratedLevelData, SpecialTileData.SpecialRoomList);
		}
	}
}

void ULevelGenerationLibrary::GenerateBasicRooms(FLevelGenerationSettings& LevelGenerationSettings, FGeneratedLevelData& GeneratedLevelData)
{
	const int RoomMaximum = (LevelGenerationSettings.GridSize.X * LevelGenerationSettings.GridSize.Y * LevelGenerationSettings.GridSize.Z) - GeneratedLevelData.LevelTileData.Num();

	const int BasicRoomQuantity = UKismetMathLibrary::RandomFloatInRangeFromStream(FMath::Clamp(LevelGenerationSettings.BasicRoomsMinimum, 0, RoomMaximum), FMath::Clamp(LevelGenerationSettings.BasicRoomsMaximum, 0, RoomMaximum), GeneratedLevelData.LevelStream);
	

	if (BasicRoomQuantity == 0) { return; }

	int RoomsGenerated = 0;
	while (RoomsGenerated < BasicRoomQuantity)
	{
		if (UDataTable* RoomDataTable = GetRandomRoomListFromDataTable(LevelGenerationSettings.BasicRoomList, GeneratedLevelData.LevelStream))
		{
			PlaceRoomInGrid(LevelGenerationSettings, GeneratedLevelData, RoomDataTable);
			RoomsGenerated++;
		}
	}
}

void ULevelGenerationLibrary::GenerateCorridors3D(FLevelGenerationSettings& LevelGenerationSettings, FGeneratedLevelData& GeneratedLevelData, UObject* WorldRef)
{
	// Store room coordinate data 
	TArray<FIntVector> LevelTileDataKeys;
	GeneratedLevelData.LevelTileData.GetKeys(LevelTileDataKeys);

	TArray<FIntVector>RoomCoordinates;

	for (FIntVector CurrentVector : LevelTileDataKeys)
	{
		switch (GeneratedLevelData.LevelTileData[CurrentVector].TileType)
		{
		case ETileType::Room_Basic:
			RoomCoordinates.Add(CurrentVector);
			break;
		case ETileType::Room_Key:
			RoomCoordinates.Add(CurrentVector);
			break;
		case ETileType::Room_Special:
			RoomCoordinates.Add(CurrentVector);
			break;
		default:
			break;
		}
	}

	// Get all possible connections between rooms
	TArray<FEdgeInfo> DelaunayArray = UDelaunayTriangulationLibrary::DelaunayTetrahedralization(LevelGenerationSettings.GridSize, RoomCoordinates, true);

	// Find the minimum spanning tree for all the rooms in the level (minimum paths needed for all rooms to be reachable in gameplay)
	TArray<FEdgeInfo> DiscardedEdgesArray;
	GeneratedLevelData.MinimumSpanningTree = UKruskalMSTLibrary::GetMinimumSpanningTreeV2(RoomCoordinates, DelaunayArray, DiscardedEdgesArray);

	// Randomly add some extra paths
	UKruskalMSTLibrary::RandomlyAddEdgesToMST(GeneratedLevelData.MinimumSpanningTree, DiscardedEdgesArray, GeneratedLevelData.LevelStream, LevelGenerationSettings.ExtraCorridorChance);

	// Display the MST + extra paths in the game session
	if (LevelGenerationSettings.bDrawMST) { UKruskalMSTLibrary::DrawMST(GeneratedLevelData.MinimumSpanningTree, WorldRef, LevelGenerationSettings.TileSize, FLinearColor::Green); }

	if (!LevelGenerationSettings.bGenerateCorridors) { return; }

	// Sort the MST + extra paths from shortest to longest path
	GeneratedLevelData.MinimumSpanningTree.Sort([](const FEdgeInfo& EdgeA, const FEdgeInfo& EdgeB) -> bool {
		// sort by weight
		if (EdgeA.Weight < EdgeB.Weight) return true;
		if (EdgeA.Weight > EdgeB.Weight) return false;
		else { return false; }
		});

	// Array containing all the paths that need to be built
	TArray<FPathGenerationData> PathGenerationDataArray;

	// Get the data needed for all paths to be built
	for (FEdgeInfo CurrentPath : GeneratedLevelData.MinimumSpanningTree)
	{
		FPathGenerationData PathGenerationData;
		PathGenerationData.PathData = CurrentPath;
		PathGenerationData.OriginTile = GeneratedLevelData.LevelTileData.Find(CurrentPath.Origin);
		PathGenerationData.DestinationTile = GeneratedLevelData.LevelTileData.Find(CurrentPath.Destination);

		if (PathGenerationData.OriginTile && PathGenerationData.DestinationTile)
		{
			// Find the best access point to use for the starting location and end location
			TArray<FIntVector> OriginTileAccessPoints;
			PathGenerationData.OriginTile->TileAccessPoints.GetKeys(OriginTileAccessPoints);

			TArray<FIntVector> DestinationTileAccessPoints;
			PathGenerationData.DestinationTile->TileAccessPoints.GetKeys(DestinationTileAccessPoints);

			// Check every access point the destination tile has
			for (FIntVector CurrentDestinationAP : DestinationTileAccessPoints)
			{
				TArray<EDirections> DestinationDirectionsArray = PathGenerationData.DestinationTile->TileAccessPoints[CurrentDestinationAP].AccessibleDirections.Array();
				for (EDirections DestinationDirection : DestinationDirectionsArray)
				{
					const FIntVector PotentialPathEnd = CurrentPath.Destination + RotateIntVectorCoordinatefromOrigin(CurrentDestinationAP + DirectionCoordinates[DestinationDirection], GeneratedLevelData.LevelTileData[CurrentPath.Destination].TileRotation);

					if (GeneratedLevelData.LevelTileData.Contains(PotentialPathEnd))
					{
						continue;
					}

					// Check every access point the origin tile has		
					for (FIntVector CurrentOriginAP : OriginTileAccessPoints)
					{
						TArray<EDirections> OriginDirectionsArray = PathGenerationData.OriginTile->TileAccessPoints[CurrentOriginAP].AccessibleDirections.Array();
						for (EDirections OriginDirection : OriginDirectionsArray)
						{
							const FIntVector PotentialPathStart = CurrentPath.Origin + RotateIntVectorCoordinatefromOrigin(CurrentOriginAP + DirectionCoordinates[OriginDirection], GeneratedLevelData.LevelTileData[CurrentPath.Origin].TileRotation);

							if (GeneratedLevelData.LevelTileData.Contains(PotentialPathEnd))
							{
								continue;
							}

							// TODO - Can both access points reach each other? Connected components (Island ID). Not sure if needed in 3D space.

							const float PotentialShortestDistance = FVector(PotentialPathEnd - PotentialPathStart).Length();
							// Shortest distance?
							if (PotentialShortestDistance < PathGenerationData.PathDistance)
							{
								PathGenerationData.OriginAccessPoint = CurrentOriginAP;
								PathGenerationData.OriginAccessPointLocation = CurrentPath.Origin + RotateIntVectorCoordinatefromOrigin(CurrentOriginAP, GeneratedLevelData.LevelTileData[CurrentPath.Origin].TileRotation);
								PathGenerationData.OriginPathDirection = OriginDirection;

								PathGenerationData.DestinationAccessPoint = CurrentDestinationAP;
								PathGenerationData.DestinationAccessPointLocation = CurrentPath.Destination + RotateIntVectorCoordinatefromOrigin(CurrentDestinationAP, GeneratedLevelData.LevelTileData[CurrentPath.Destination].TileRotation);
								PathGenerationData.DestinationPathDirection = DestinationDirection;

								PathGenerationData.PathEnd = PotentialPathEnd;
								PathGenerationData.PathStart = PotentialPathStart;

								PathGenerationData.PathDistance = PotentialShortestDistance;
							}

						}
					}
				}
			}

			PathGenerationDataArray.Add(PathGenerationData);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT(" ULevelGenerationLibrary::GenerateCorridors OriginTile/DestinationTile are invalid!"));
		}
	}

	// Use A* pathfinding to generate optimal paths
	for (FPathGenerationData CurrentPathGenData : PathGenerationDataArray)
	{
		// If ShortestDistance is 0 then no pathfinding is needed, room is adjacent
		if (CurrentPathGenData.PathDistance != 0.f)
		{
			FPathGenerationData PathGenerationData = CurrentPathGenData;
			TArray<FIntVector> ExcludedOriginAPs;
			TArray<FIntVector> ExcludedDestinationAPs;

			int MaxOriginStartingLocations = 0;
			int MaxDestinationEndLocations = 0;

			TArray<FTileAccessData> TileAccessDataArray;
			PathGenerationData.OriginTile->TileAccessPoints.GenerateValueArray(TileAccessDataArray);

			for (FTileAccessData CurrentAccessData : TileAccessDataArray)
			{
				MaxOriginStartingLocations += CurrentAccessData.AccessibleDirections.Num();
			}

			PathGenerationData.DestinationTile->TileAccessPoints.GenerateValueArray(TileAccessDataArray);

			for (FTileAccessData CurrentAccessData : TileAccessDataArray)
			{
				MaxDestinationEndLocations += CurrentAccessData.AccessibleDirections.Num();
			}

			do
			{
				TMap<FIntVector, FAdvancedPathNode> PathData;
				if (AdvancedAStarPathfinding(PathGenerationData.PathStart, PathGenerationData.PathEnd, PathData, PathGenerationDataArray, LevelGenerationSettings, GeneratedLevelData))
				{
					// Add the path to the generated level data

					PathGenerationData.OriginTile->TileAccessPoints[PathGenerationData.OriginAccessPoint].DirectionsInUse.Add(PathGenerationData.OriginPathDirection);

					PathGenerationData.DestinationTile->TileAccessPoints[PathGenerationData.DestinationAccessPoint].DirectionsInUse.Add(PathGenerationData.DestinationPathDirection);

					TArray<FIntVector> PathDataVectors;
					PathData.GenerateKeyArray(PathDataVectors);

					FIntVector PreviousPathVector = FIntVector{ -1, -1, -1 };
					ETileType PreviousPathTileType = ETileType::Corridor;

					for (FIntVector CurrentPathVector : PathDataVectors)
					{
						switch (PathData[CurrentPathVector].SpecialPathType)
						{
						case ESpecialPathType::None:
							// Normal Corridor
							GenerateNormalPathData(CurrentPathVector, PathData, PathGenerationData, PreviousPathVector, PreviousPathTileType, GeneratedLevelData);
							break;

						case ESpecialPathType::SpecialPathSection:
							PreviousPathVector = CurrentPathVector;
							PreviousPathTileType = ETileType::Corridor_Section;
							break;

						default:
							GenerateSpecialPathData(CurrentPathVector, PathData, PathGenerationData, PreviousPathVector, PreviousPathTileType, GeneratedLevelData);
							break;
						}
					}

					break;
				}
				// Try building a path using different access points
				else
				{
					if (ExcludedOriginAPs.Num() < MaxOriginStartingLocations)
					{
						ExcludedOriginAPs.Add(PathGenerationData.PathStart);
						PathGenerationData = GetShortestPathToTargetRoom(GeneratedLevelData, PathGenerationData.PathData, ExcludedOriginAPs, TArray<FIntVector>());
					}
					else
					{
						ExcludedDestinationAPs.Add(PathGenerationData.PathEnd);

						if (ExcludedDestinationAPs.Num() == MaxDestinationEndLocations) { break; }

						PathGenerationData = GetShortestPathToTargetRoom(GeneratedLevelData, PathGenerationData.PathData, TArray<FIntVector>(), ExcludedDestinationAPs);
					}
				}
			} while (true);
			
		}
		else
		{
			FCorridorTileData CorridorTileData;
			CorridorTileData.TileType = ETileType::Corridor;

			const EDirections DirectionToDestination = GetDirectionForIntVectors(CurrentPathGenData.PathStart, CurrentPathGenData.DestinationAccessPointLocation);
			const EDirections DirectionToOrigin = GetDirectionForIntVectors(CurrentPathGenData.PathStart, CurrentPathGenData.OriginAccessPointLocation);

			CorridorTileData.AdjacentAccessPoints.Add(DirectionToDestination, CurrentPathGenData.DestinationTile->TileType);
			CorridorTileData.AdjacentAccessPoints.Add(DirectionToOrigin, CurrentPathGenData.OriginTile->TileType);

			CurrentPathGenData.OriginTile->TileAccessPoints[CurrentPathGenData.OriginAccessPoint].DirectionsInUse.Add(RotateDirection(DirectionToDestination, CurrentPathGenData.OriginTile->TileRotation.GetInverse()));
			CurrentPathGenData.DestinationTile->TileAccessPoints[CurrentPathGenData.DestinationAccessPoint].DirectionsInUse.Add(RotateDirection(DirectionToOrigin, CurrentPathGenData.DestinationTile->TileRotation.GetInverse()));

			if (!GeneratedLevelData.LevelPathData.Contains(CurrentPathGenData.PathStart))
			{
				GeneratedLevelData.LevelPathData.Add(CurrentPathGenData.PathStart, CorridorTileData);
			}
		}
	}

	// Insert path data into GeneratedLevelData
	TArray<FIntVector> PathDataKeys;
	GeneratedLevelData.LevelPathData.GenerateKeyArray(PathDataKeys);

	for (FIntVector CurrentKey : PathDataKeys)
	{
		FTileData TileData;
		switch(GeneratedLevelData.LevelPathData[CurrentKey].TileType)
		{
		case ETileType::Corridor:
			TileData = GetTileDataFromCorridorTileData(GeneratedLevelData.LevelPathData[CurrentKey], LevelGenerationSettings, GeneratedLevelData.LevelStream);
			GeneratedLevelData.LevelTileData.Add(CurrentKey, TileData);
			break;

		case ETileType::Corridor_Special:
			AddTileDataFromSpecialCorridorTileData(CurrentKey, GeneratedLevelData.LevelPathData[CurrentKey], LevelGenerationSettings, GeneratedLevelData);
			break;

		default:
			break;
		}
	}
}

void ULevelGenerationLibrary::PlaceRoomInGrid(FLevelGenerationSettings& LevelGenerationSettings, FGeneratedLevelData& GeneratedLevelData, UDataTable* RoomList)
{
	int32 FailCounter = 0.f;

	while (true)
	{
		// Randomly select a room from the room data table
		const FTileGenerationData* TileGenerationData = GetRandomRoomFromRoomList(RoomList, GeneratedLevelData.LevelStream);
		if (!TileGenerationData) { continue; }

		// Set the room's rotation
		FRotator RoomRotation = FRotator::ZeroRotator;
		if (TileGenerationData->bTileHasSetRotation) { RoomRotation = TileGenerationData->TileSetRotation; }
		else { RoomRotation = GetRandomRoomRotation(GeneratedLevelData.LevelStream); }

		// Set the room's location in the level
		FIntVector TileCoordinate = FIntVector::ZeroValue;
		bool bEmptyCoordinateFound = false;

		if (TileGenerationData->bTileHasSetCoordinate && !GeneratedLevelData.LevelTileData.Contains(TileGenerationData->TileSetGridCoordinate)) { TileCoordinate = TileGenerationData->TileSetGridCoordinate; }
		else { TileCoordinate = GetRandomEmptyCoordinate(LevelGenerationSettings, GeneratedLevelData.LevelStream, GeneratedLevelData.LevelTileData, bEmptyCoordinateFound); }

		// Check if the room's placement is valid
		if (!bEmptyCoordinateFound || !RoomPlacementIsValid(LevelGenerationSettings, GeneratedLevelData, TileGenerationData->TileData.TileAccessPoints, TileCoordinate, RoomRotation, TileGenerationData->TileData.TileSize))
		{
			FailCounter++;

			// Return if the function has failed too often when trying to place the room
			if (FailCounter > FMath::RoundToInt((LevelGenerationSettings.GridSize.X * LevelGenerationSettings.GridSize.Y * LevelGenerationSettings.GridSize.Z) * 0.25f))
			{
				return;
			}
			else { continue; }
		}

		// Add the room to the level data
		FTileData TileData;
		TileData.TileMap = TileGenerationData->TileData.TileMap;
		TileData.TileSubMaps = TileGenerationData->TileData.TileSubMaps;
		TileData.TileActorSlotMaps = TileGenerationData->TileData.TileActorSlotMaps;
		TileData.TileType = TileGenerationData->TileData.TileType;
		TileData.TileRotation = RoomRotation;
		TileData.TileSize = TileGenerationData->TileData.TileSize;
		TileData.TileAccessPoints = TileGenerationData->TileData.TileAccessPoints;
		TileData.MinimapMesh = TileGenerationData->TileData.MinimapMesh;
		TileData.ParentRoomCoordinate = TileCoordinate;

		GeneratedLevelData.LevelTileData.Add(TileCoordinate, TileData);

		if (TileData.TileSize.Num() > 1.f)
		{
			for (FIntVector CurrentCoordinate : TileData.TileSize.Array())
			{
				// The centre section has already been added to the level
				if (CurrentCoordinate == FIntVector{ 0,0,0 }) { continue; }

				const FIntVector RoomSectionCoordinate = RotateIntVectorCoordinatefromOrigin(CurrentCoordinate, RoomRotation) + TileCoordinate;

				FTileData RoomSectionTileData;
				RoomSectionTileData.TileType = ETileType::Room_Section;
				RoomSectionTileData.TileRotation = RoomRotation;
				RoomSectionTileData.TileSize.Add({ 0,0,0 });
				RoomSectionTileData.ParentRoomCoordinate = TileCoordinate;

				if (TileData.TileAccessPoints.Contains(CurrentCoordinate))
				{
					RoomSectionTileData.TileAccessPoints.Add(FIntVector(0,0,0), TileData.TileAccessPoints[CurrentCoordinate]);

					GeneratedLevelData.LevelTileData.Add(RoomSectionCoordinate, RoomSectionTileData);
				}
				else
				{
					GeneratedLevelData.LevelTileData.Add(RoomSectionCoordinate, RoomSectionTileData);
				}
			}
		}
		return;
	}
}

UDataTable* ULevelGenerationLibrary::GetRandomRoomListFromDataTable(TMap<UDataTable*, double>& DataTableList, const FRandomStream& LevelStream)
{
	if (DataTableList.IsEmpty()) { return nullptr; }

	TArray<double> ProbabilityArray;
	DataTableList.GenerateValueArray(ProbabilityArray);

	double ProbabilityTotal = 0.f;
	for (double CurrentProbability : ProbabilityArray)
	{
		ProbabilityTotal += CurrentProbability;
	}

	const double RandomNumber = UKismetMathLibrary::RandomFloatInRangeFromStream(0.f, ProbabilityTotal, LevelStream);
	double LevelResultMinimum = 0.00000001f;

	TArray<UDataTable*> DataTableArray;
	DataTableList.GenerateKeyArray(DataTableArray);

	for (UDataTable* CurrentDataTable : DataTableArray)
	{
		if (RandomNumber >= LevelResultMinimum && RandomNumber < DataTableList[CurrentDataTable] + LevelResultMinimum)
		{
			return CurrentDataTable;
		}
		else
		{
			LevelResultMinimum += DataTableList[CurrentDataTable];
		}
	}

	return nullptr;
}

FTileGenerationData* ULevelGenerationLibrary::GetRandomRoomFromRoomList(UDataTable* RoomDataTable, const FRandomStream& LevelStream)
{
	if (!RoomDataTable) { return nullptr; }

	TArray<FName> RowNames = RoomDataTable->GetRowNames();

	double ProbabilityTotal = 0.f;
	for (FName CurrentRowName : RowNames)
	{
		FTileGenerationData* TileGenerationData = nullptr;
		if (TileGenerationData = RoomDataTable->FindRow<FTileGenerationData>(CurrentRowName, ""))
		{
			ProbabilityTotal += TileGenerationData->RandomSelectionChance;
		}
	}

	const double RandomLevelResult = UKismetMathLibrary::RandomFloatInRangeFromStream(0.f, ProbabilityTotal, LevelStream);
	double LevelResultMinimum = 0.00000001f;

	for (FName CurrentRowName : RowNames)
	{
		FTileGenerationData* TileGenerationData = nullptr;
		if (TileGenerationData = RoomDataTable->FindRow<FTileGenerationData>(CurrentRowName, ""))
		{
			if (RandomLevelResult >= LevelResultMinimum && RandomLevelResult < (LevelResultMinimum + TileGenerationData->RandomSelectionChance))
			{
				return TileGenerationData;
			}
			else { LevelResultMinimum += TileGenerationData->RandomSelectionChance; }
		}
	}

	return nullptr;
}

FCorridorLevelData* ULevelGenerationLibrary::GetRandomCorridorFromCorridorList(UDataTable* RoomDataTable, const FRandomStream& LevelStream)
{
	if (!RoomDataTable) { return nullptr; }

	TArray<FName> RowNames = RoomDataTable->GetRowNames();

	double ProbabilityTotal = 0.f;
	for (FName CurrentRowName : RowNames)
	{
		FCorridorLevelData* TileGenerationData = nullptr;
		if (TileGenerationData = RoomDataTable->FindRow<FCorridorLevelData>(CurrentRowName, ""))
		{
			ProbabilityTotal += TileGenerationData->RandomSelectionChance;
		}
	}

	const double RandomLevelResult = UKismetMathLibrary::RandomFloatInRangeFromStream(0.f, ProbabilityTotal, LevelStream);
	double LevelResultMinimum = 0.00000001f;

	for (FName CurrentRowName : RowNames)
	{
		FCorridorLevelData* TileGenerationData = nullptr;
		if (TileGenerationData = RoomDataTable->FindRow<FCorridorLevelData>(CurrentRowName, ""))
		{
			if (RandomLevelResult >= LevelResultMinimum && RandomLevelResult < (LevelResultMinimum + TileGenerationData->RandomSelectionChance))
			{
				return TileGenerationData;
			}
			else { LevelResultMinimum += TileGenerationData->RandomSelectionChance; }
		}
	}

	return nullptr;
}

FRotator ULevelGenerationLibrary::GetRandomRoomRotation(const FRandomStream& LevelStream)
{
	return FRotator(0.f, 90.f * UKismetMathLibrary::RandomIntegerInRangeFromStream(0, 3, LevelStream), 0.f);
}

FIntVector ULevelGenerationLibrary::GetRandomEmptyCoordinate(FLevelGenerationSettings& LevelGenerationSettings, const FRandomStream& LevelStream, TMap<FIntVector, FTileData>& LevelTileData, bool& bEmptyCoordinateFound)
{
	bEmptyCoordinateFound = false;

	TArray<FIntVector> EmptyCoordinates;

	for (int Z = 0; Z < (LevelGenerationSettings.GridSize.Z - 1); Z++)
	{
		for (int Y = 0; Y < (LevelGenerationSettings.GridSize.Y - 1); Y++)
		{
			for (int X = 0; X < (LevelGenerationSettings.GridSize.X - 1); X++)
			{
				const FIntVector Coordinate{ X, Y, Z };

				if (!LevelTileData.Contains(Coordinate)) { EmptyCoordinates.Add(Coordinate); }
			}
		}
	}
	
	if (!EmptyCoordinates.IsEmpty())
	{
		bEmptyCoordinateFound = true;
		return EmptyCoordinates[UKismetMathLibrary::RandomIntegerInRangeFromStream(0, (EmptyCoordinates.Num() - 1), LevelStream)];
	}
	
	return FIntVector{ 0,0,0 };
}

bool ULevelGenerationLibrary::RoomPlacementIsValid(const FLevelGenerationSettings& LevelGenerationSettings, const FGeneratedLevelData& GeneratedLevelData, const TMap<FIntVector, FTileAccessData> TileAccessPoints, const FIntVector PlacementCoordinate, const FRotator RoomRotation, const TSet<FIntVector> TileSize)
{
	if (TileAccessPoints.IsEmpty() || TileSize.IsEmpty()) { return false; }

	for (FIntVector CurrentCoordinate : TileSize.Array())
	{
		FIntVector RoomSectionCoordinate = RotateIntVectorCoordinatefromOrigin(CurrentCoordinate, RoomRotation);
		FIntVector CoordinateToValidate = RoomSectionCoordinate + PlacementCoordinate;

		if (!IsCoordinateInGridSpace(CoordinateToValidate / LevelGenerationSettings.TileSize, LevelGenerationSettings.GridSize)) { return false; }
		if (!IsRoomBufferEmpty(LevelGenerationSettings, GeneratedLevelData.LevelTileData, CoordinateToValidate, RoomSectionCoordinate, RoomRotation, TileSize)) { return false; }
		if (GeneratedLevelData.LevelTileData.Contains(CoordinateToValidate)) { return false; }
	}

	return true;
}

bool ULevelGenerationLibrary::IsCoordinateInGridSpace(const FIntVector& Coordinate, const FIntVector& GridSize)
{
	if ((Coordinate.X >= 0 && Coordinate.X < GridSize.X) &&
		(Coordinate.Y >= 0 && Coordinate.Y < GridSize.Y) &&
		(Coordinate.Z >= 0 && Coordinate.Z < GridSize.Z))
	{
		return true;
	}

	return false;
}

bool ULevelGenerationLibrary::IsRoomBufferEmpty(const FLevelGenerationSettings& LevelGenerationSettings, const TMap<FIntVector, FTileData>& LevelTileData, const FIntVector& RoomCoordinate, const FIntVector& RoomSectionCoordinate, const FRotator& RoomRotation, const TSet<FIntVector>& TileSize)
{
	if (LevelGenerationSettings.RoomBufferSize == 0) { return true; }

	for (int i = 1; i <= LevelGenerationSettings.RoomBufferSize; i++)
	{
		for (FIntVector CurrentCoordinate : CoordinateChecklist.Array())
		{
			FIntVector RotatedCoordinate = RotateIntVectorCoordinatefromOrigin(CurrentCoordinate, RoomRotation) * i;
			FIntVector CoordinateToValidate = RotatedCoordinate + RoomCoordinate;
			FIntVector SecondCoordinateToValidate = (CurrentCoordinate * i) + RoomSectionCoordinate;

			if (LevelTileData.Contains(CoordinateToValidate) && !TileSize.Contains(SecondCoordinateToValidate)) { return false; }
		}
	}

	return true;
}

bool ULevelGenerationLibrary::AdvancedAStarPathfinding(FIntVector StartLocation, FIntVector EndLocation, TMap<FIntVector, FAdvancedPathNode>& PathData, const TArray<FPathGenerationData>& PathGenerationDataArray, FLevelGenerationSettings& LevelGenerationSettings, FGeneratedLevelData& GeneratedLevelData)
{
	FSpecialPathInfo BlankInfo;

	const TArray<EDirections> DirectionEvaluationOrder
	{
		EDirections::West,
		EDirections::North,
		EDirections::East,
		EDirections::South
	};

	// The set of nodes to be evaluated
	TMap<FIntVector, FAdvancedPathNode> OPEN;
	// The set of nodes already evaluated
	TMap<FIntVector, FAdvancedPathNode> CLOSED;

	// Get inaccessible nodes
	TSet<FIntVector> LevelTiles;
	GeneratedLevelData.LevelTileData.GetKeys(LevelTiles);

	TSet<FIntVector> InaccessibleNodes;

	for (FIntVector CurrentVector : LevelTiles)
	{
		switch (GeneratedLevelData.LevelTileData[CurrentVector].TileType)
		{
		case ETileType::Room_Basic:
			InaccessibleNodes.Add(CurrentVector);
			break;
		case ETileType::Room_Key:
			InaccessibleNodes.Add(CurrentVector);
			break;
		case ETileType::Room_Special:
			InaccessibleNodes.Add(CurrentVector);
			break;
		case ETileType::Room_Section:
			InaccessibleNodes.Add(CurrentVector);
			break;
		case ETileType::Corridor:
			break;
		case ETileType::Corridor_Section:
			break;
		case ETileType::Corridor_Special:
			break;
		default:
			break;
		}
	}

	if (InaccessibleNodes.Contains(StartLocation) || InaccessibleNodes.Contains(EndLocation))
	{
		FAdvancedPathNode StartingNode;
		StartingNode.ParentNode = StartLocation;
		
		FAdvancedPathNode EndingNode;
		EndingNode.ParentNode = EndLocation;

		PathData.Add(StartLocation, StartingNode);
		PathData.Add(EndLocation, EndingNode);
		return true;
	}

	FAdvancedPathNode StartingNode;

	UpdateAdvancedNode(StartingNode, LevelGenerationSettings, GeneratedLevelData, StartLocation, StartLocation, StartLocation, ESpecialPathType::None, BlankInfo, FRotator(0.f, 0.f, 0.f), StartingNode.PreviousPath, StartLocation, EndLocation);
	StartingNode.ParentNode = StartLocation;
	StartingNode.GCost = 0.f;
	StartingNode.HCost = FVector(EndLocation - StartLocation).Length();
	StartingNode.FCost = StartingNode.GCost + StartingNode.HCost;
	StartingNode.FCost += StartingNode.ElevationToEnd == 0 ? 0.f : 2.5f;

	CLOSED.Add(StartLocation, StartingNode);

	// Loop
	do
	{
		// Find all nodes that need to be evaluated (adjacent to CLOSED nodes)
		TArray<FIntVector> ClosedCoordinates;
		CLOSED.GenerateKeyArray(ClosedCoordinates);

		for (FIntVector CurrentClosedNode : ClosedCoordinates)
		{
			if (CLOSED[CurrentClosedNode].SpecialPathType == ESpecialPathType::SpecialPathSection) { continue; }

			

			for (EDirections CurrentDirection : DirectionEvaluationOrder)
			{
				// Ignore above and below nodes now that we are using special path objects
				if (CurrentDirection == EDirections::Above || CurrentDirection == EDirections::Below) { continue; }

				const FIntVector CurrentCoordinate = CurrentClosedNode + DirectionCoordinates[CurrentDirection];

				// Do not evaluate nodes that are on the previous path
				if (CLOSED[CurrentClosedNode].PreviousPath.Contains(CurrentCoordinate)) { continue; }

				bool bIsThereSpecialPathAtCoordinate = false;

				if (GeneratedLevelData.LevelPathData.Contains(CurrentCoordinate))
				{
					bIsThereSpecialPathAtCoordinate = GeneratedLevelData.LevelPathData[CurrentCoordinate].SpecialPathType != ESpecialPathType::None;
				}

				// Add basic nodes to OPEN
				if (!InaccessibleNodes.Contains(CurrentCoordinate) && !OPEN.Contains(CurrentCoordinate) && !CLOSED.Contains(CurrentCoordinate) && !bIsThereSpecialPathAtCoordinate)
				{
					TMap<FIntVector, FAdvancedPathNode*> PreviousNodePath = CLOSED[CurrentClosedNode].PreviousPath;
					FAdvancedPathNode* CurrentAdvancedPathNode = new FAdvancedPathNode(CLOSED[CurrentClosedNode].ParentNode, CLOSED[CurrentClosedNode].GCost, CLOSED[CurrentClosedNode].HCost, CLOSED[CurrentClosedNode].FCost, CLOSED[CurrentClosedNode].SpecialPathType, CLOSED[CurrentClosedNode].SpecialPathInfo, CLOSED[CurrentClosedNode].SpecialPathOriginVector, CLOSED[CurrentClosedNode].SpecialPathRotation, CLOSED[CurrentClosedNode].bIsPathReversed, CLOSED[CurrentClosedNode].ElevationToEnd, CLOSED[CurrentClosedNode].PreviousPath);
					PreviousNodePath.Add(CurrentClosedNode, CurrentAdvancedPathNode);

					FAdvancedPathNode NewNode;
					UpdateAdvancedNode(NewNode, LevelGenerationSettings, GeneratedLevelData, CurrentCoordinate, CurrentCoordinate, CurrentCoordinate, ESpecialPathType::None, BlankInfo, FRotator(0.f, 0.f, 0.f), PreviousNodePath, StartLocation, EndLocation);

					OPEN.Add(CurrentCoordinate, NewNode);
				}

				// Add advanced nodes to OPEN
				EvaluateSpecialCorridorStructures(OPEN, CLOSED, InaccessibleNodes, PathGenerationDataArray, LevelGenerationSettings, GeneratedLevelData, CurrentClosedNode, (EDirections)CurrentDirection, StartLocation, EndLocation);
			}
		}

		// Sort OPEN by lowest FCost 
		// if tied, go for lowest h cost 
		OPEN.ValueSort([](const FAdvancedPathNode& NodeA, const FAdvancedPathNode& NodeB) -> bool {
			// sort by weight
			if (NodeA.FCost < NodeB.FCost) return true;
			if (NodeA.FCost > NodeB.FCost) return false;
			else
			{
				if (NodeA.ElevationToEnd < NodeB.ElevationToEnd) return true;
				else if (NodeA.ElevationToEnd > NodeB.ElevationToEnd) return false;
				else
				{
					if (NodeA.HCost < NodeB.HCost) return true;
					else { return false; }
				}
			}
			});

		if (OPEN.IsEmpty())
		{
			return false;
		}

		TArray<FIntVector>OpenCoordinates;
		OPEN.GenerateKeyArray(OpenCoordinates);
		TArray<FAdvancedPathNode>OpenPathNodes;
		OPEN.GenerateValueArray(OpenPathNodes);

		// Current = node in OPEN with the lowest FCost
		FIntVector CurrentNodeCoordinate = OpenCoordinates[0];
		FAdvancedPathNode CurrentNode = OpenPathNodes[0];

		if (CurrentNode.SpecialPathType != ESpecialPathType::None && CurrentNode.SpecialPathType != ESpecialPathType::SpecialPathSection)
		{
			TArray<FIntVector> PreviousPathKeyArray;
			CurrentNode.PreviousPath.GenerateKeyArray(PreviousPathKeyArray);

			for (int i = 0; i < CurrentNode.SpecialPathInfo.PathVolume.Num(); i++)
			{
				const FIntVector PathVolumeCoordinate = PreviousPathKeyArray.Last(i);
				FAdvancedPathNode SectionNode = *CurrentNode.PreviousPath[PathVolumeCoordinate];

				CLOSED.Add(PathVolumeCoordinate, SectionNode);
				OPEN.Remove(PathVolumeCoordinate);
			}
		}

		// Add Current to CLOSED
		CLOSED.Add(CurrentNodeCoordinate, CurrentNode);

		// Remove Current from OPEN
		OPEN.Remove(CurrentNodeCoordinate);

		// If Current is the target node then the path has been found
		if (CurrentNodeCoordinate == EndLocation)
		{
			// Return
			TMap<FIntVector, FAdvancedPathNode*> PreviousNodePath = CLOSED[CurrentNodeCoordinate].PreviousPath;
			FAdvancedPathNode* CurrentAdvancedPathNode = new FAdvancedPathNode(CLOSED[CurrentNodeCoordinate].ParentNode, CLOSED[CurrentNodeCoordinate].GCost, CLOSED[CurrentNodeCoordinate].HCost, CLOSED[CurrentNodeCoordinate].FCost, CLOSED[CurrentNodeCoordinate].SpecialPathType, CLOSED[CurrentNodeCoordinate].SpecialPathInfo, CLOSED[CurrentNodeCoordinate].SpecialPathOriginVector, CLOSED[CurrentNodeCoordinate].SpecialPathRotation, CLOSED[CurrentNodeCoordinate].bIsPathReversed, CLOSED[CurrentNodeCoordinate].ElevationToEnd, CLOSED[CurrentNodeCoordinate].PreviousPath);
			PreviousNodePath.Add(CurrentNodeCoordinate, CurrentAdvancedPathNode);

			CLOSED[EndLocation].PreviousPath = PreviousNodePath;

			break;
		}

		// For each neighbour of the current node
		for (uint8 CurrentDirection = 1; (EDirections)CurrentDirection < EDirections::MAX; CurrentDirection++)
		{
			// Ignore above and below nodes now that we are using special path objects
			if ((EDirections)CurrentDirection == EDirections::Above || (EDirections)CurrentDirection == EDirections::Below) { continue; }

			const FIntVector NeighbourCoordinate = CurrentNodeCoordinate + DirectionCoordinates[(EDirections)CurrentDirection];

			if (CLOSED.Contains(NeighbourCoordinate))
			{
				if (CLOSED[NeighbourCoordinate].SpecialPathType == ESpecialPathType::SpecialPathSection) { continue; }
			}

			// If neighbour is not traversable or neighbour is in CLOSED, skip to the next neighbour
			if (InaccessibleNodes.Contains(NeighbourCoordinate) || CLOSED.Contains(NeighbourCoordinate)) { continue; }

			// If neighbour lies on the current node's path, ignore it
			if (CurrentNode.PreviousPath.Contains(NeighbourCoordinate)) { continue; }

			if (GeneratedLevelData.LevelPathData.Contains(NeighbourCoordinate))
			{
				if (GeneratedLevelData.LevelPathData[NeighbourCoordinate].SpecialPathType != ESpecialPathType::None) { continue; }
			}

			// If new path to neighbour is shorter OR neighbour is not in OPEN
			if ((CurrentNode.GCost + FVector(CurrentNodeCoordinate - NeighbourCoordinate).Length()) < FVector(StartLocation - NeighbourCoordinate).Length() || !OPEN.Contains(NeighbourCoordinate))
			{
				TMap<FIntVector, FAdvancedPathNode*> PreviousNodePath = CLOSED[CurrentNodeCoordinate].PreviousPath;
				FAdvancedPathNode* CurrentAdvancedPathNode = new FAdvancedPathNode(CLOSED[CurrentNodeCoordinate].ParentNode, CLOSED[CurrentNodeCoordinate].GCost, CLOSED[CurrentNodeCoordinate].HCost, CLOSED[CurrentNodeCoordinate].FCost, CLOSED[CurrentNodeCoordinate].SpecialPathType, CLOSED[CurrentNodeCoordinate].SpecialPathInfo, CLOSED[CurrentNodeCoordinate].SpecialPathOriginVector, CLOSED[CurrentNodeCoordinate].SpecialPathRotation, CLOSED[CurrentNodeCoordinate].bIsPathReversed, CLOSED[CurrentNodeCoordinate].ElevationToEnd, CLOSED[CurrentNodeCoordinate].PreviousPath);
				PreviousNodePath.Add(CurrentNodeCoordinate, CurrentAdvancedPathNode);

				// Set FCost of neighbour
				// Set parent of neighbour to current
				FAdvancedPathNode NeighbourNode;
				UpdateAdvancedNode(NeighbourNode, LevelGenerationSettings, GeneratedLevelData, NeighbourCoordinate, NeighbourCoordinate, NeighbourCoordinate, ESpecialPathType::None, BlankInfo, FRotator(0.f, 0.f, 0.f), PreviousNodePath, StartLocation, EndLocation);
				
				// If neighbour is not in OPEN
				// Add neighbour to OPEN
				OPEN.Add(NeighbourCoordinate, NeighbourNode);
			}
		}
	} while (true);

	// Assemble PathData

	TMap<FIntVector, FAdvancedPathNode*> ChosenPath = CLOSED[EndLocation].PreviousPath;

	TArray<FIntVector> PreviousPathKeyArray;
	ChosenPath.GenerateKeyArray(PreviousPathKeyArray);

	for (int i = 1; i < PreviousPathKeyArray.Num() + 1; i++)
	{
		int PreviousPathKey = PreviousPathKeyArray.Num() - i;

		const FIntVector PreviousPathCoordinate = PreviousPathKeyArray[PreviousPathKey];
		FAdvancedPathNode PreviousPathNode = *ChosenPath[PreviousPathCoordinate];

		PathData.Add(PreviousPathCoordinate, PreviousPathNode);

		if (PreviousPathCoordinate == StartLocation) { break; }
	}

	return true;
}

FTileData ULevelGenerationLibrary::GetTileDataFromCorridorTileData(FCorridorTileData CorridorTileData, FLevelGenerationSettings& LevelGenerationSettings, const FRandomStream& LevelStream)
{
	// The returned tile data
	FTileData OutTileData;

	const TSet<EDirections> AllConnections{EDirections::North, EDirections::East, EDirections::South, EDirections::West};

	TArray<EDirections> InAdjacentAccessPointsKeys;
	CorridorTileData.AdjacentAccessPoints.GenerateKeyArray(InAdjacentAccessPointsKeys);

	TSet<EDirections> CorridorConnections;
	for (EDirections CurrentDirection : InAdjacentAccessPointsKeys)
	{
		CorridorConnections.Add(CurrentDirection);
	}
	
	// Find out what kind of basic corridor section the corridor tile data is
	const TSet<EDirections> MissingConnections = AllConnections.Difference(CorridorConnections);
	
	// Four-Way
	if (MissingConnections.IsEmpty())
	{
		OutTileData.TileType = ETileType::Corridor;
		OutTileData.TileRotation = FRotator(0.f, 0.f, 0.f);

		if (LevelGenerationSettings.CorridorLevelDataTableList.Contains(ECorridorType::FourWay))
		{
			const FCorridorLevelData* CorridorLevelData = GetRandomCorridorFromCorridorList(LevelGenerationSettings.CorridorLevelDataTableList[ECorridorType::FourWay], LevelStream);
			OutTileData.TileMap = CorridorLevelData->CorridorMap;
			OutTileData.TileSubMaps = CorridorLevelData->CorridorSubMaps;
			OutTileData.TileActorSlotMaps = CorridorLevelData->CorridorActorSlotMaps;
			OutTileData.MinimapMesh = CorridorLevelData->MinimapMesh;
		}
		return OutTileData;
	}

	// Three-Way
	if (MissingConnections.Num() == 1.f)
	{
		OutTileData.TileType = ETileType::Corridor;

		if (MissingConnections.Contains(EDirections::West))
		{
			OutTileData.TileRotation = FRotator(0.f, 0.f, 0.f);
		}
		else if (MissingConnections.Contains(EDirections::North))
		{
			OutTileData.TileRotation = FRotator(0.f, 90.f, 0.f);
		}
		else if (MissingConnections.Contains(EDirections::East))
		{
			OutTileData.TileRotation = FRotator(0.f, 180.f, 0.f);
		}
		else if (MissingConnections.Contains(EDirections::South))
		{
			OutTileData.TileRotation = FRotator(0.f, 270.f, 0.f);
		}

		if (LevelGenerationSettings.CorridorLevelDataTableList.Contains(ECorridorType::ThreeWay))
		{
			const FCorridorLevelData* CorridorLevelData = GetRandomCorridorFromCorridorList(LevelGenerationSettings.CorridorLevelDataTableList[ECorridorType::ThreeWay], LevelStream);
			OutTileData.TileMap = CorridorLevelData->CorridorMap;
			OutTileData.TileSubMaps = CorridorLevelData->CorridorSubMaps;
			OutTileData.TileActorSlotMaps = CorridorLevelData->CorridorActorSlotMaps;
			OutTileData.MinimapMesh = CorridorLevelData->MinimapMesh;
		}

		return OutTileData;
	}

	// Two-Way Corner
	if (MissingConnections.Num() == 2.f)
	{
		// Corner
		if ((CorridorConnections.Contains(EDirections::North) && CorridorConnections.Contains(EDirections::East)) ||
			(CorridorConnections.Contains(EDirections::East) && CorridorConnections.Contains(EDirections::South)) ||
			(CorridorConnections.Contains(EDirections::South) && CorridorConnections.Contains(EDirections::West)) ||
			(CorridorConnections.Contains(EDirections::West) && CorridorConnections.Contains(EDirections::North)))
		{

			OutTileData.TileType = ETileType::Corridor;

			if (CorridorConnections.Contains(EDirections::North) && CorridorConnections.Contains(EDirections::East))
			{
				OutTileData.TileRotation = FRotator(0.f, 0.f, 0.f);
			}
			else if (CorridorConnections.Contains(EDirections::East) && CorridorConnections.Contains(EDirections::South))
			{
				OutTileData.TileRotation = FRotator(0.f, 90.f, 0.f);
			}
			else if (CorridorConnections.Contains(EDirections::South) && CorridorConnections.Contains(EDirections::West))
			{
				OutTileData.TileRotation = FRotator(0.f, 180.f, 0.f);
			}
			else if (CorridorConnections.Contains(EDirections::West) && CorridorConnections.Contains(EDirections::North))
			{
				OutTileData.TileRotation = FRotator(0.f, 270.f, 0.f);
			}

			if (LevelGenerationSettings.CorridorLevelDataTableList.Contains(ECorridorType::Corner))
			{
				const FCorridorLevelData* CorridorLevelData = GetRandomCorridorFromCorridorList(LevelGenerationSettings.CorridorLevelDataTableList[ECorridorType::Corner], LevelStream);
				OutTileData.TileMap = CorridorLevelData->CorridorMap;
				OutTileData.TileSubMaps = CorridorLevelData->CorridorSubMaps;
				OutTileData.TileActorSlotMaps = CorridorLevelData->CorridorActorSlotMaps;
				OutTileData.MinimapMesh = CorridorLevelData->MinimapMesh;
			}

			return OutTileData;
		}
		else
		{
			OutTileData.TileType = ETileType::Corridor;

			// Straight
			if (CorridorConnections.Contains(EDirections::North) || CorridorConnections.Contains(EDirections::South))
			{
				OutTileData.TileRotation = FRotator(0.f, 0.f, 0.f);
			}
			else if (CorridorConnections.Contains(EDirections::East) || CorridorConnections.Contains(EDirections::West))
			{
				OutTileData.TileRotation = FRotator(0.f, 90.f, 0.f);
			}

			if (LevelGenerationSettings.CorridorLevelDataTableList.Contains(ECorridorType::TwoWay))
			{
				const FCorridorLevelData* CorridorLevelData = GetRandomCorridorFromCorridorList(LevelGenerationSettings.CorridorLevelDataTableList[ECorridorType::TwoWay], LevelStream);
				OutTileData.TileMap = CorridorLevelData->CorridorMap;
				OutTileData.TileSubMaps = CorridorLevelData->CorridorSubMaps;
				OutTileData.TileActorSlotMaps = CorridorLevelData->CorridorActorSlotMaps;
				OutTileData.MinimapMesh = CorridorLevelData->MinimapMesh;
			}

			return OutTileData;
		}
	}

	if (MissingConnections.Num() == 3.f)
	{
		OutTileData.TileType = ETileType::Corridor;
		OutTileData.TileRotation = FRotator(0.f, 0.f, 0.f);

		// One-Way
		if (CorridorConnections.Contains(EDirections::North))
		{
			OutTileData.TileRotation = FRotator(0.f, 0.f, 0.f);
		}
		else if (CorridorConnections.Contains(EDirections::East))
		{
			OutTileData.TileRotation = FRotator(0.f, 90.f, 0.f);
		}
		else if (CorridorConnections.Contains(EDirections::South))
		{
			OutTileData.TileRotation = FRotator(0.f, 180.f, 0.f);
		}
		else if (CorridorConnections.Contains(EDirections::West))
		{
			OutTileData.TileRotation = FRotator(0.f, 270.f, 0.f);
		}

		if (LevelGenerationSettings.CorridorLevelDataTableList.Contains(ECorridorType::OneWay))
		{
			const FCorridorLevelData* CorridorLevelData = GetRandomCorridorFromCorridorList(LevelGenerationSettings.CorridorLevelDataTableList[ECorridorType::OneWay], LevelStream);
			OutTileData.TileMap = CorridorLevelData->CorridorMap;
			OutTileData.TileSubMaps = CorridorLevelData->CorridorSubMaps;
			OutTileData.TileActorSlotMaps = CorridorLevelData->CorridorActorSlotMaps;
			OutTileData.MinimapMesh = CorridorLevelData->MinimapMesh;
		}

		return OutTileData;
	}

	OutTileData.TileType = ETileType::Corridor;
	OutTileData.TileRotation = FRotator(0.f, 0.f, 0.f);

	if (LevelGenerationSettings.CorridorLevelDataTableList.Contains(ECorridorType::ZeroWay))
	{
		const FCorridorLevelData* CorridorLevelData = GetRandomCorridorFromCorridorList(LevelGenerationSettings.CorridorLevelDataTableList[ECorridorType::ZeroWay], LevelStream);
		OutTileData.TileMap = CorridorLevelData->CorridorMap;
		OutTileData.TileSubMaps = CorridorLevelData->CorridorSubMaps;
		OutTileData.TileActorSlotMaps = CorridorLevelData->CorridorActorSlotMaps;
		OutTileData.MinimapMesh = CorridorLevelData->MinimapMesh;
	}

	return OutTileData;
}

void ULevelGenerationLibrary::AddTileDataFromSpecialCorridorTileData(FIntVector Coordinate, FCorridorTileData CorridorTileData, FLevelGenerationSettings& LevelGenerationSettings, FGeneratedLevelData& GeneratedLevelData)
{
	FTileData OutTileData;

	OutTileData.TileType = CorridorTileData.TileType;
	OutTileData.TileRotation = CorridorTileData.SpecialPathRotation;
	OutTileData.TileSize = CorridorTileData.SpecialPathTileSize;

	ESpecialPathType CorridorSpecialPathType = CorridorTileData.SpecialPathType;

	if (((uint8)CorridorTileData.SpecialPathType >= (uint8)ESpecialPathType::Elevator_S2) && ((uint8)CorridorTileData.SpecialPathType < (uint8)ESpecialPathType::MAX))
	{
		CorridorSpecialPathType = ESpecialPathType::Elevator_Bottom;
	}

	// Elevator Bottom
	if (LevelGenerationSettings.SpecialPathLevelDataTableList.Contains(CorridorSpecialPathType))
	{
		const FCorridorLevelData* SpecialCorridor = GetRandomCorridorFromCorridorList(LevelGenerationSettings.SpecialPathLevelDataTableList[CorridorSpecialPathType], GeneratedLevelData.LevelStream);
		OutTileData.TileMap = SpecialCorridor ? SpecialCorridor->CorridorMap : nullptr;
		OutTileData.TileSubMaps = SpecialCorridor ? SpecialCorridor->CorridorSubMaps : TArray<TSoftObjectPtr<UWorld>>();
		OutTileData.TileActorSlotMaps = SpecialCorridor ? SpecialCorridor->CorridorActorSlotMaps : TMap<EActorSlotType, TSoftObjectPtr<UWorld>>();
		OutTileData.TileAccessPoints = SpecialCorridor->CorridorAccessPoints;
		OutTileData.MinimapMesh = SpecialCorridor->MinimapMesh;

		if (OutTileData.TileAccessPoints.Contains(FIntVector(0.f, 0.f, 0.f)))
		{
			// Get used directions and add them to OutTileData
			AddUsedAccessPointsToTileData(OutTileData, FIntVector(0.f, 0.f, 0.f), CorridorTileData);
		}
	}



	GeneratedLevelData.LevelTileData.Add(Coordinate, OutTileData);

	if (OutTileData.TileSize.Num() > 1.f)
	{
		for (FIntVector CurrentCoordinate : OutTileData.TileSize.Array())
		{
			// The centre section has already been added to the level
			if (CurrentCoordinate == FIntVector{ 0,0,0 }) { continue; }

			const FIntVector CorridorSectionCoordinate = RotateIntVectorCoordinatefromOrigin(CurrentCoordinate, CorridorTileData.SpecialPathRotation) + Coordinate;

			FTileData CorridorSectionTileData;

			CorridorSectionTileData.TileRotation = OutTileData.TileRotation;
			CorridorSectionTileData.TileSize.Add({ 0,0,0 });

			if (((uint8)CorridorTileData.SpecialPathType >= (uint8)ESpecialPathType::Elevator_S2) && ((uint8)CorridorTileData.SpecialPathType < (uint8)ESpecialPathType::MAX))
			{
				CorridorSectionTileData.TileType = ETileType::Corridor_Special;

				// Elevator Top
				if (CurrentCoordinate == OutTileData.TileSize.Array().Last())
				{
					if (LevelGenerationSettings.SpecialPathLevelDataTableList.Contains(ESpecialPathType::Elevator_Top))
					{
						const FCorridorLevelData* SpecialCorridor = GetRandomCorridorFromCorridorList(LevelGenerationSettings.SpecialPathLevelDataTableList[ESpecialPathType::Elevator_Top], GeneratedLevelData.LevelStream);
						CorridorSectionTileData.TileMap = SpecialCorridor->CorridorMap;
						CorridorSectionTileData.TileSubMaps = SpecialCorridor->CorridorSubMaps;
						CorridorSectionTileData.TileActorSlotMaps = SpecialCorridor->CorridorActorSlotMaps;
						CorridorSectionTileData.TileAccessPoints = SpecialCorridor->CorridorAccessPoints;
						CorridorSectionTileData.MinimapMesh = SpecialCorridor->MinimapMesh;

						if (CorridorSectionTileData.TileAccessPoints.Contains(FIntVector(0.f, 0.f, 0.f)) && GeneratedLevelData.LevelPathData.Contains(CorridorSectionCoordinate))
						{
							// Get used directions and add them to OutTileData
							AddUsedAccessPointsToTileData(CorridorSectionTileData, FIntVector(0.f, 0.f, 0.f), GeneratedLevelData.LevelPathData[CorridorSectionCoordinate]);
						}
					}
				}
				// Elevator Middle
				else
				{
					if (LevelGenerationSettings.SpecialPathLevelDataTableList.Contains(ESpecialPathType::Elevator_Middle))
					{
						const FCorridorLevelData* SpecialCorridor = GetRandomCorridorFromCorridorList(LevelGenerationSettings.SpecialPathLevelDataTableList[ESpecialPathType::Elevator_Middle], GeneratedLevelData.LevelStream);
						CorridorSectionTileData.TileMap = SpecialCorridor->CorridorMap;
						CorridorSectionTileData.TileSubMaps = SpecialCorridor->CorridorSubMaps;
						CorridorSectionTileData.TileActorSlotMaps = SpecialCorridor->CorridorActorSlotMaps;
						CorridorSectionTileData.MinimapMesh = SpecialCorridor->MinimapMesh;
					}
				}
			}
			else
			{
				CorridorSectionTileData.TileType = ETileType::Corridor_Section;
			}

			if (OutTileData.TileAccessPoints.Contains(CurrentCoordinate))
			{
				CorridorSectionTileData.TileAccessPoints.Add(FIntVector(0, 0, 0), OutTileData.TileAccessPoints[CurrentCoordinate]);
			}
			GeneratedLevelData.LevelTileData.Add(CorridorSectionCoordinate, CorridorSectionTileData);
		}
	}
}

EDirections ULevelGenerationLibrary::GetDirectionForIntVectors(FIntVector StartVector, FIntVector TargetVector)
{
	FIntVector DirectionVector = TargetVector - StartVector;
	DirectionVector.X = UKismetMathLibrary::Clamp(DirectionVector.X, -1.f, 1.f);
	DirectionVector.Y = UKismetMathLibrary::Clamp(DirectionVector.Y, -1.f, 1.f);
	DirectionVector.Z = UKismetMathLibrary::Clamp(DirectionVector.Z, -1.f, 1.f);

	if (abs(DirectionVector.Z) > 0.f)
	{
		if (DirectionVector.Z > 0) { return EDirections::Above; }
		else { return EDirections::Below; }
	}
	else if (abs(DirectionVector.X) > abs(DirectionVector.Y))
	{
		if (DirectionVector.X > 0) { return EDirections::North; }
		else { return EDirections::South; }
	}
	else if (abs(DirectionVector.Y) > abs(DirectionVector.X))
	{
		if (DirectionVector.Y > 0) { return EDirections::East; }
		else { return EDirections::West; }
	}
	else { return EDirections::None; }

	return EDirections();
}

void ULevelGenerationLibrary::EvaluateSpecialCorridorStructures(TMap<FIntVector, FAdvancedPathNode>& OPEN, TMap<FIntVector, FAdvancedPathNode>& CLOSED, TSet<FIntVector>& InaccessibleNodes, const TArray<FPathGenerationData>& PathGenerationDataArray, const FLevelGenerationSettings& LevelGenerationSettings, const FGeneratedLevelData& GeneratedLevelData, const FIntVector CurrentClosedNode, EDirections CurrentDirection, const FIntVector StartLocation, const FIntVector EndLocation)
{
	TMap<EDirections, FRotator> RotationMap
	{
		{EDirections::North, FRotator(0.f, 0.f, 0.f)},
		{EDirections::East, FRotator(0.f, 90.f, 0.f)},
		{EDirections::South, FRotator(0.f, 180.f, 0.f)},
		{EDirections::West, FRotator(0.f, 270.f, 0.f)},
	};

	TArray<ESpecialPathType> SpecialPathTypeArray;
	LevelGenerationSettings.AllowedSpecialPathTypes.GenerateKeyArray(SpecialPathTypeArray);

	USpecialPathData* SpecialPathData = LevelGenerationSettings.SpecialPathData.IsValid() ? LevelGenerationSettings.SpecialPathData.Get() : nullptr;

	const FIntVector CurrentCoordinate = CurrentClosedNode + DirectionCoordinates[(EDirections)CurrentDirection];
	const FRotator PathRotation = RotationMap[CurrentDirection];
	FRotator ReversedPathRotation = RotationMap[CurrentDirection] + FRotator(0.f, 180.f, 0.f);

	ReversedPathRotation.Yaw -= ReversedPathRotation.Yaw >= 360.f ? 360.f : 0.f;

	if (InaccessibleNodes.Contains(CurrentCoordinate) || CLOSED.Contains(CurrentCoordinate)) { return; }

	// Check to see which special path objects can be used for the next node in the path
	for (ESpecialPathType CurrentSpecialPathType : SpecialPathTypeArray)
	{
		if (!LevelGenerationSettings.AllowedSpecialPathTypes[CurrentSpecialPathType]) { continue; }
		if (!SpecialPathData->SpecialPathSettings.Contains(CurrentSpecialPathType)) { continue; }

		FSpecialPathInfo SpecialPathInfo = SpecialPathData->SpecialPathSettings[CurrentSpecialPathType];

		// Check both variations of the special paths (e.g. stairs going up, stairs going down)
		for (int i = 0; i < 2; i++)
		{
			bool bInvalidPlacement = false;
			bool bOverrideInvalidPlacement = false;
			bool bPathReversed = true;
			if (i != 0) { bPathReversed = false; }

			if (bPathReversed)
			{
				const FIntVector ExitVector = CurrentCoordinate + RotateIntVectorCoordinatefromOrigin(SpecialPathInfo.ExitVector, PathRotation);

				if (OPEN.Contains(ExitVector)) { continue; }

				// Allow the use of existing paths if they are at the same location and rotation
				if (GeneratedLevelData.LevelPathData.Contains(CurrentCoordinate))
				{
					FAdvancedPathNode ExistingPathNode = GeneratedLevelData.LevelPathData[CurrentCoordinate].ParentPathNode;

					// If is same path type, rotation and origin vector, use it
					if (ExistingPathNode.SpecialPathType == CurrentSpecialPathType && ExistingPathNode.SpecialPathRotation == PathRotation && ExistingPathNode.SpecialPathOriginVector == CurrentCoordinate)
					{
						bOverrideInvalidPlacement = true;
					}
					/*
					// If is same path but flipped (up instead of down), use it
					else if (GeneratedLevelData.LevelPathData.Contains(ExistingPathNode.SpecialPathOriginVector))
					{
						FAdvancedPathNode ExistingPathNodeParent = GeneratedLevelData.LevelPathData[ExistingPathNode.SpecialPathOriginVector].ParentPathNode;

						// Guess what the expected origin vector, exit vector and rotation are using data from the current evaluation, then compare with the actual details

						const FIntVector ExpectedExitVector = ExitVector + RotateIntVectorCoordinatefromOrigin(SpecialPathInfo.ExitVector, PathRotation);
						const FIntVector ExpectedOriginVector = ExitVector + DirectionCoordinates[(EDirections)CurrentDirection];

						const bool bSameExit = ExistingPathNodeParent.SpecialPathOriginVector == ExpectedExitVector;
						const bool bSameOriginVector = ExistingPathNodeParent.SpecialPathOriginVector == ExpectedOriginVector;
						const bool bSameRotation = ExistingPathNodeParent.SpecialPathRotation == PathRotation;

						if (bSameExit && bSameOriginVector && bSameRotation) { bOverrideInvalidPlacement = true; }
					}
					*/
				}

				// Check that the special path's volume does not collide with any inaccessible nodes or the path of the current node
				for (FIntVector CurrentVector : SpecialPathInfo.PathVolume)
				{
					const FIntVector RotatedCoordinate = CurrentCoordinate + RotateIntVectorCoordinatefromOrigin(CurrentVector, PathRotation);
					if (!IsCoordinateEmpty(RotatedCoordinate, CurrentClosedNode, OPEN, CLOSED, InaccessibleNodes, PathGenerationDataArray, GeneratedLevelData) || RotatedCoordinate == EndLocation)
					{
						bInvalidPlacement = true;
						break;
					}

					if (CLOSED[CurrentClosedNode].PreviousPath.Contains(CurrentVector))
					{
						bInvalidPlacement = true;
						break;
					}
				}

				// Check that the exit location for the special path is not blocked
				if (!IsCoordinateEmpty(ExitVector, CurrentClosedNode, OPEN, CLOSED, InaccessibleNodes, TArray<FPathGenerationData>(), GeneratedLevelData))
				{
					bInvalidPlacement = true;
				}
				else if (CLOSED[CurrentClosedNode].PreviousPath.Contains(ExitVector))
				{
					bInvalidPlacement = true;
				}

				// Add to OPEN
				if (!bInvalidPlacement || bOverrideInvalidPlacement)
				{
					TMap<FIntVector, FAdvancedPathNode*> PreviousNodePath = CLOSED[CurrentClosedNode].PreviousPath;
					FAdvancedPathNode* CurrentAdvancedPathNode = new FAdvancedPathNode(CLOSED[CurrentClosedNode].ParentNode, CLOSED[CurrentClosedNode].GCost, CLOSED[CurrentClosedNode].HCost, CLOSED[CurrentClosedNode].FCost, CLOSED[CurrentClosedNode].SpecialPathType, CLOSED[CurrentClosedNode].SpecialPathInfo, CLOSED[CurrentClosedNode].SpecialPathOriginVector, CLOSED[CurrentClosedNode].SpecialPathRotation, CLOSED[CurrentClosedNode].bIsPathReversed, CLOSED[CurrentClosedNode].ElevationToEnd, CLOSED[CurrentClosedNode].PreviousPath);
					PreviousNodePath.Add(CurrentClosedNode, CurrentAdvancedPathNode);

					FAdvancedPathNode NewNode;
					UpdateAdvancedNode(NewNode, LevelGenerationSettings, GeneratedLevelData, CurrentCoordinate, CurrentCoordinate, ExitVector, CurrentSpecialPathType, SpecialPathInfo, PathRotation, PreviousNodePath, StartLocation, EndLocation);

					OPEN.Add(ExitVector, NewNode);
				}

			}
			else
			{
				const FIntVector ExitVector = CurrentCoordinate + RotateIntVectorCoordinatefromOrigin(SpecialPathInfo.ExitVector * -1.f, ReversedPathRotation);

				const int XYDifference = abs(CurrentClosedNode.X - ExitVector.X) + abs(CurrentClosedNode.Y - ExitVector.Y);

				FIntVector OriginVector = FIntVector::ZeroValue;
				//const FIntVector OriginVector = XYDifference == 0.f ? CurrentClosedNode + RotateIntVectorCoordinatefromOrigin(SpecialPathInfo.ExitVector * -1.f, PathRotation) : ExitVector - DirectionCoordinates[(EDirections)CurrentDirection];

				if (XYDifference > 1.f)
				{
					OriginVector = ExitVector - DirectionCoordinates[(EDirections)CurrentDirection];
				}
				else if (XYDifference == 1.f)
				{
					OriginVector = ExitVector;
				}
				else
				{
					OriginVector = CurrentClosedNode + RotateIntVectorCoordinatefromOrigin(SpecialPathInfo.ExitVector * -1.f, PathRotation);
				}

				if (OPEN.Contains(ExitVector)) { continue; }

				// Allow the use of existing paths if they are at the same location and rotation
				if (GeneratedLevelData.LevelPathData.Contains(CurrentCoordinate))
				{
					FAdvancedPathNode ExistingPathNode = GeneratedLevelData.LevelPathData[CurrentCoordinate].ParentPathNode;

					// If is same path type, rotation and origin vector, use it
					if (ExistingPathNode.SpecialPathType == CurrentSpecialPathType && ExistingPathNode.SpecialPathRotation == ReversedPathRotation && ExistingPathNode.SpecialPathOriginVector == OriginVector)
					{
						bOverrideInvalidPlacement = true;
					}
					// If is same path but flipped (up instead of down), use it
					else if (GeneratedLevelData.LevelPathData.Contains(ExistingPathNode.SpecialPathOriginVector))
					{
						FAdvancedPathNode ExistingPathNodeParent = GeneratedLevelData.LevelPathData[ExistingPathNode.SpecialPathOriginVector].ParentPathNode;

						// Guess what the expected origin vector, exit vector and rotation are using data from the current evaluation, then compare with the actual details

						const FIntVector ExpectedExitVector = ExitVector + RotateIntVectorCoordinatefromOrigin(SpecialPathInfo.ExitVector, PathRotation);
						const FIntVector ExpectedOriginVector = OriginVector;

						const bool bSameExit = ExistingPathNodeParent.SpecialPathOriginVector == ExpectedExitVector;
						const bool bSameOriginVector = ExistingPathNodeParent.SpecialPathOriginVector == ExpectedOriginVector;
						const bool bSameRotation = ExistingPathNodeParent.SpecialPathRotation == PathRotation;

						if (bSameExit && bSameOriginVector && bSameRotation) { bOverrideInvalidPlacement = true; }

					}
				}

				// Check that the special path's volume does not collide with any inaccessible nodes or the path of the current node
				for (FIntVector CurrentVector : SpecialPathInfo.PathVolume)
				{
					const FIntVector RotatedCoordinate = CurrentCoordinate + RotateIntVectorCoordinatefromOrigin(CurrentVector * -1.f, ReversedPathRotation);
					if (!IsCoordinateEmpty(RotatedCoordinate, CurrentClosedNode, OPEN, CLOSED, InaccessibleNodes, PathGenerationDataArray, GeneratedLevelData) || RotatedCoordinate == EndLocation)
					{
						bInvalidPlacement = true;
						break;
					}

					if (CLOSED[CurrentClosedNode].PreviousPath.Contains(CurrentVector))
					{
						bInvalidPlacement = true;
						break;
					}
				}

				// Check that the exit location for the special path is not blocked
				if (!IsCoordinateEmpty(ExitVector, CurrentClosedNode, OPEN, CLOSED, InaccessibleNodes, TArray<FPathGenerationData>(), GeneratedLevelData))
				{
					bInvalidPlacement = true;
				}
				else if (CLOSED[CurrentClosedNode].PreviousPath.Contains(ExitVector))
				{
					bInvalidPlacement = true;
					break;
				}

				// Add to OPEN
				if (!bInvalidPlacement || bOverrideInvalidPlacement)
				{
					TMap<FIntVector, FAdvancedPathNode*> PreviousNodePath = CLOSED[CurrentClosedNode].PreviousPath;
					FAdvancedPathNode* CurrentAdvancedPathNode = new FAdvancedPathNode(CLOSED[CurrentClosedNode].ParentNode, CLOSED[CurrentClosedNode].GCost, CLOSED[CurrentClosedNode].HCost, CLOSED[CurrentClosedNode].FCost, CLOSED[CurrentClosedNode].SpecialPathType, CLOSED[CurrentClosedNode].SpecialPathInfo, CLOSED[CurrentClosedNode].SpecialPathOriginVector, CLOSED[CurrentClosedNode].SpecialPathRotation, CLOSED[CurrentClosedNode].bIsPathReversed, CLOSED[CurrentClosedNode].ElevationToEnd, CLOSED[CurrentClosedNode].PreviousPath);
					PreviousNodePath.Add(CurrentClosedNode, CurrentAdvancedPathNode);

					// Do not use the reveresed path rotation of there is exit vector shares the same X and Y coordinates of the current closed node
					const FRotator NodeRotation = XYDifference == 0.f ? PathRotation : ReversedPathRotation;

					FAdvancedPathNode NewNode;
					NewNode.bIsPathReversed = false;
					UpdateAdvancedNode(NewNode, LevelGenerationSettings, GeneratedLevelData, OriginVector, CurrentCoordinate, ExitVector, CurrentSpecialPathType, SpecialPathInfo, NodeRotation, PreviousNodePath, StartLocation, EndLocation);

					OPEN.Add(ExitVector, NewNode);
				}

			}
		}
	}
}

bool ULevelGenerationLibrary::IsCoordinateEmpty(const FIntVector Coordinate, const FIntVector CurrentClosedNode, const TMap<FIntVector, FAdvancedPathNode> OPEN, const TMap<FIntVector, FAdvancedPathNode> CLOSED, const TSet<FIntVector> InaccessibleNodes, const TArray<FPathGenerationData> PathGenerationDataArray, const FGeneratedLevelData& GeneratedLevelData)
{
	if (InaccessibleNodes.Contains(Coordinate)) { return false; }
	else if (CLOSED.Contains(Coordinate)) { return false; }
	else if (GeneratedLevelData.LevelTileData.Contains(Coordinate)) { return false; }
	else if (GeneratedLevelData.LevelPathData.Contains(Coordinate)) { return false; }
	else if (CLOSED[CurrentClosedNode].PreviousPath.Contains(Coordinate)) { return false; }
	
	if (!PathGenerationDataArray.IsEmpty())
	{
		for (FPathGenerationData CurrentPathGenData : PathGenerationDataArray)
		{
			if (Coordinate == CurrentPathGenData.PathStart || Coordinate == CurrentPathGenData.PathEnd) { return false; }
		}
	}
	
	return true;
}

void ULevelGenerationLibrary::GenerateNormalPathData(const FIntVector CurrentPathVector, const TMap<FIntVector, FAdvancedPathNode>& PathData, const FPathGenerationData& CurrentPathGenData, FIntVector& PreviousPathVector, ETileType& PreviousPathTileType, FGeneratedLevelData& GeneratedLevelData)
{
	TArray<FIntVector> PreviousPathKeyArray;

	FIntVector ParentNode = FIntVector::ZeroValue;

	if (PathData[CurrentPathVector].PreviousPath.IsEmpty())
	{
		ParentNode = CurrentPathGenData.OriginAccessPointLocation;
	}
	else 
	{ 
		PathData[CurrentPathVector].PreviousPath.GenerateKeyArray(PreviousPathKeyArray); 
		ParentNode = !PreviousPathKeyArray.IsEmpty() ? PreviousPathKeyArray.Last() : PathData[CurrentPathVector].ParentNode;
	}

	const EDirections DirectionToParentNode = GetDirectionForIntVectors(CurrentPathVector, ParentNode/*PreviousPathKeyArray.Last()*/);
	EDirections DirectionToPreviousPath;

	if (PreviousPathVector != FIntVector{ -1, -1, -1 } && DirectionCoordinates.FindKey(PreviousPathVector - CurrentPathVector))
	{
		DirectionToPreviousPath = *DirectionCoordinates.FindKey(PreviousPathVector - CurrentPathVector);
	}

	FCorridorTileData CorridorTileData;
	CorridorTileData.TileType = ETileType::Corridor;

	// If current path is adjacent to the path destination
	if (CurrentPathVector == CurrentPathGenData.PathEnd)
	{
		const EDirections DirectionToDestination = GetDirectionForIntVectors(CurrentPathVector, CurrentPathGenData.DestinationAccessPointLocation);

		CorridorTileData.AdjacentAccessPoints.Add(DirectionToDestination, CurrentPathGenData.DestinationTile->TileType);
		CorridorTileData.AdjacentAccessPoints.Add(DirectionToParentNode, ETileType::Corridor);
	}
	// If current path is adjacent to the path origin
	else if (CurrentPathVector == CurrentPathGenData.PathStart)
	{
		const EDirections DirectionToOrigin = GetDirectionForIntVectors(CurrentPathVector, CurrentPathGenData.OriginAccessPointLocation);

		if (PreviousPathVector != FIntVector{ -1, -1, -1 })
		{
			CorridorTileData.AdjacentAccessPoints.Add(DirectionToPreviousPath, PreviousPathTileType);
		}
		CorridorTileData.AdjacentAccessPoints.Add(DirectionToOrigin, CurrentPathGenData.OriginTile->TileType);
	}
	else
	{
		if (PreviousPathVector != FIntVector{ -1, -1, -1 })
		{
			CorridorTileData.AdjacentAccessPoints.Add(DirectionToPreviousPath, PreviousPathTileType);
		}
		CorridorTileData.AdjacentAccessPoints.Add(DirectionToParentNode, ETileType::Corridor);
	}

	if (!GeneratedLevelData.LevelPathData.Contains(CurrentPathVector))
	{
		GeneratedLevelData.LevelPathData.Add(CurrentPathVector, CorridorTileData);
	}
	else
	{
		GeneratedLevelData.LevelPathData[CurrentPathVector].AdjacentAccessPoints.Append(CorridorTileData.AdjacentAccessPoints);
	}

	// Let rooms know which access points are in use
	/*TArray<EDirections>AdjacentAccessPointArray;
	CorridorTileData.AdjacentAccessPoints.GenerateKeyArray(AdjacentAccessPointArray);
	for (EDirections CurrentDirection : AdjacentAccessPointArray)
	{
		// Ignore any access points that do not connect to rooms
		switch (CorridorTileData.AdjacentAccessPoints[CurrentDirection])
		{
		case ETileType::Room_Basic:
		case ETileType::Room_Key:
		case ETileType::Room_Special:
		case ETileType::Room_Section:
			break;

		default:
			continue;
			break;
		}

		// Get the room's coordinate
		const FIntVector TargetAccessPointCoordinate = CurrentPathVector + DirectionCoordinates[CurrentDirection];

		if (!GeneratedLevelData.LevelTileData.Contains(TargetAccessPointCoordinate)) { continue; }
		const FIntVector RoomCoordinate = GeneratedLevelData.LevelTileData[TargetAccessPointCoordinate].ParentRoomCoordinate;

		if (!GeneratedLevelData.LevelTileData.Contains(RoomCoordinate)) { continue; }
		FTileData RoomTileData = GeneratedLevelData.LevelTileData[RoomCoordinate];

		// Find out which of the room's access point is being used
		const FIntVector RoomAccessPoint = RotateIntVectorCoordinatefromOrigin((TargetAccessPointCoordinate - RoomCoordinate), RoomTileData.TileRotation);
		if (!RoomTileData.TileAccessPoints.Contains(RoomAccessPoint)) { continue; }

		RoomTileData.TileAccessPoints[RoomAccessPoint].DirectionsInUse.Add(GetDirectionForIntVectors(TargetAccessPointCoordinate, CurrentPathVector));
	}*/

	PreviousPathVector = CurrentPathVector;
	PreviousPathTileType = CorridorTileData.TileType;
}

void ULevelGenerationLibrary::GenerateSpecialPathData(const FIntVector CurrentPathVector, const TMap<FIntVector, FAdvancedPathNode>& PathData, const FPathGenerationData& CurrentPathGenData, FIntVector& PreviousPathVector, ETileType& PreviousPathTileType, FGeneratedLevelData& GeneratedLevelData)
{
	FCorridorTileData CorridorTileData;
	if (GeneratedLevelData.LevelPathData.Contains(PathData[CurrentPathVector].SpecialPathOriginVector))
	{
		CorridorTileData = GeneratedLevelData.LevelPathData[PathData[CurrentPathVector].SpecialPathOriginVector];
	}
	else
	{
		CorridorTileData.TileType = ETileType::Corridor_Special;
		CorridorTileData.SpecialPathType = PathData[CurrentPathVector].SpecialPathType;
		CorridorTileData.SpecialPathRotation = PathData[CurrentPathVector].SpecialPathRotation;
		CorridorTileData.SpecialPathTileSize = PathData[CurrentPathVector].SpecialPathInfo.PathVolume;
		CorridorTileData.ParentPathNode = PathData[CurrentPathVector];
	}

	FCorridorTileData CorridorSectionTileData;
	CorridorSectionTileData.TileType = ETileType::Corridor_Section;
	CorridorSectionTileData.SpecialPathType = ESpecialPathType::SpecialPathSection;
	CorridorSectionTileData.ParentPathNode = PathData[CurrentPathVector];

	// Build Exit Corridor if needed
	if (!PathData[CurrentPathVector].SpecialPathInfo.PathVolume.Contains(PathData[CurrentPathVector].SpecialPathInfo.ExitVector))
	{
		GenerateNormalPathData(CurrentPathVector, PathData, CurrentPathGenData, PreviousPathVector, PreviousPathTileType, GeneratedLevelData);
	}

	// Sections
	for (FIntVector CurrentVector : PathData[CurrentPathVector].SpecialPathInfo.PathVolume)
	{
		const FIntVector RotatedCoordinate = PathData[CurrentPathVector].SpecialPathOriginVector + RotateIntVectorCoordinatefromOrigin(CurrentVector, CorridorTileData.SpecialPathRotation);
		if (!GeneratedLevelData.LevelPathData.Contains(RotatedCoordinate)) { GeneratedLevelData.LevelPathData.Add(RotatedCoordinate, CorridorSectionTileData); }
	}

	// Build Special Path
	GeneratedLevelData.LevelPathData.Add(PathData[CurrentPathVector].SpecialPathOriginVector, CorridorTileData);

	if (((uint8)PathData[CurrentPathVector].SpecialPathType >= (uint8)ESpecialPathType::Elevator_S2) && ((uint8)PathData[CurrentPathVector].SpecialPathType < (uint8)ESpecialPathType::MAX))
	{
		// Store adjacent access points
		FIntVector ParentNode = FIntVector::ZeroValue;

		if (!PathData[CurrentPathVector].bIsPathReversed)
		{
			const FIntVector ExitVector = CurrentPathVector + RotateIntVectorCoordinatefromOrigin(CorridorTileData.ParentPathNode.SpecialPathInfo.ExitVector, CorridorTileData.SpecialPathRotation);

			if (!GetParentNode(PathData, ExitVector, ParentNode))
			{
				ParentNode = CurrentPathGenData.OriginAccessPointLocation;
			}


			// Previous node
			EDirections DirectionToPreviousPath = *DirectionCoordinates.FindKey(PreviousPathVector - CurrentPathVector);
			GeneratedLevelData.LevelPathData[CurrentPathVector].AdjacentAccessPoints.Add(DirectionToPreviousPath, PreviousPathTileType);

			// Next node
			const EDirections DirectionToParentNode = GetDirectionForIntVectors(ExitVector, ParentNode);
			const ETileType ParentNodeTileType = PathData[CurrentPathVector].PreviousPath[ParentNode]->SpecialPathType == ESpecialPathType::None ? ETileType::Corridor : ETileType::Corridor_Special;

			GeneratedLevelData.LevelPathData[ExitVector].AdjacentAccessPoints.Add(DirectionToParentNode, ParentNodeTileType);
		}
		else
		{
			FRotator ReversedPathRotation = CorridorTileData.SpecialPathRotation + FRotator(0.f, 180.f, 0.f);
			ReversedPathRotation.Yaw -= ReversedPathRotation.Yaw >= 360.f ? 360.f : 0.f;

			const FIntVector ExitVector = PathData[CurrentPathVector].SpecialPathOriginVector;//CurrentPathVector + RotateIntVectorCoordinatefromOrigin(CorridorTileData.ParentPathNode.SpecialPathInfo.ExitVector * -1.f, ReversedPathRotation);

			if (!GetParentNode(PathData, ExitVector, ParentNode))
			{
				ParentNode = CurrentPathGenData.OriginAccessPointLocation;
			}

			// Previous node
			EDirections DirectionToPreviousPath = *DirectionCoordinates.FindKey(PreviousPathVector - CurrentPathVector);
			GeneratedLevelData.LevelPathData[CurrentPathVector].AdjacentAccessPoints.Add(DirectionToPreviousPath, PreviousPathTileType);

			// Next node
			const EDirections DirectionToParentNode = GetDirectionForIntVectors(ExitVector, ParentNode);
			const ETileType ParentNodeTileType = PathData[CurrentPathVector].PreviousPath[ParentNode]->SpecialPathType == ESpecialPathType::None ? ETileType::Corridor : ETileType::Corridor_Special;
			GeneratedLevelData.LevelPathData[ExitVector].AdjacentAccessPoints.Add(DirectionToParentNode, ParentNodeTileType);
		}
	}

	PreviousPathVector = PathData[CurrentPathVector].bIsPathReversed ? CurrentPathVector : CurrentPathVector + RotateIntVectorCoordinatefromOrigin(PathData[CurrentPathVector].SpecialPathInfo.ExitVector, CorridorTileData.SpecialPathRotation);
	PreviousPathTileType = CorridorTileData.TileType;
}

void ULevelGenerationLibrary::UpdateAdvancedNode(FAdvancedPathNode& AdvancedPathNode, FLevelGenerationSettings LevelGenerationSettings, FGeneratedLevelData GeneratedLevelData, FIntVector InSpecialPathOriginVector, FIntVector InCurrentCoordinate, FIntVector InExitLocation, ESpecialPathType InSpecialPathType, FSpecialPathInfo InSpecialPathInfo, FRotator InSpecialPathRotation, TMap<FIntVector, FAdvancedPathNode*> InPreviousPath, FIntVector StartLocation, FIntVector EndLocation)
{
	AdvancedPathNode.SpecialPathType = InSpecialPathType;
	AdvancedPathNode.SpecialPathInfo = InSpecialPathInfo;
	AdvancedPathNode.SpecialPathOriginVector = InSpecialPathOriginVector;
	AdvancedPathNode.SpecialPathRotation = InSpecialPathRotation;

	AdvancedPathNode.ElevationToEnd = abs(InExitLocation.Z - EndLocation.Z);

	float NodeWeight = LevelGenerationSettings.TileTypeWeight.Contains(ETileType::Empty) ? LevelGenerationSettings.TileTypeWeight[ETileType::Empty] : 0.f;
	if (FCorridorTileData* CorridorData = GeneratedLevelData.LevelPathData.Find(InCurrentCoordinate))
	{
		NodeWeight = LevelGenerationSettings.TileTypeWeight.Contains(CorridorData->TileType) ? LevelGenerationSettings.TileTypeWeight[CorridorData->TileType] : 0.f;
	}
	if (AdvancedPathNode.SpecialPathType != ESpecialPathType::None && AdvancedPathNode.SpecialPathType != ESpecialPathType::SpecialPathSection) { NodeWeight += AdvancedPathNode.SpecialPathInfo.NodeWeight; }

	AdvancedPathNode.ParentNode = InCurrentCoordinate;
	AdvancedPathNode.PreviousPath = InPreviousPath;

	if (AdvancedPathNode.SpecialPathType != ESpecialPathType::None && AdvancedPathNode.SpecialPathType != ESpecialPathType::SpecialPathSection)
	{
		if (AdvancedPathNode.bIsPathReversed)
		{
			for (FIntVector CurrentVolumeVector : AdvancedPathNode.SpecialPathInfo.PathVolume)
			{
				const FIntVector VolumeCoordinate = AdvancedPathNode.SpecialPathOriginVector + RotateIntVectorCoordinatefromOrigin(CurrentVolumeVector, AdvancedPathNode.SpecialPathRotation);

				FAdvancedPathNode* VolumePathNode = new FAdvancedPathNode();
				VolumePathNode->bIsPathReversed = AdvancedPathNode.bIsPathReversed;
				UpdateAdvancedNode(*VolumePathNode, LevelGenerationSettings, GeneratedLevelData, AdvancedPathNode.SpecialPathOriginVector, VolumeCoordinate, InExitLocation, ESpecialPathType::SpecialPathSection, FSpecialPathInfo(), AdvancedPathNode.SpecialPathRotation, InPreviousPath, StartLocation, EndLocation);
				AdvancedPathNode.PreviousPath.Add(VolumeCoordinate, VolumePathNode);
			}
		}
		else
		{
			TArray<FIntVector> PathVolumeArray = AdvancedPathNode.SpecialPathInfo.PathVolume.Array();
			for (int i = 1; i < PathVolumeArray.Num() + 1; i++)
			{
				FIntVector CurrentVolumeVector = PathVolumeArray[PathVolumeArray.Num() - i];
				const FIntVector VolumeCoordinate = AdvancedPathNode.SpecialPathOriginVector + RotateIntVectorCoordinatefromOrigin(CurrentVolumeVector, AdvancedPathNode.SpecialPathRotation);

				FAdvancedPathNode* VolumePathNode = new FAdvancedPathNode();
				VolumePathNode->bIsPathReversed = AdvancedPathNode.bIsPathReversed;
				UpdateAdvancedNode(*VolumePathNode, LevelGenerationSettings, GeneratedLevelData, AdvancedPathNode.SpecialPathOriginVector, VolumeCoordinate, InExitLocation, ESpecialPathType::SpecialPathSection, FSpecialPathInfo(), AdvancedPathNode.SpecialPathRotation, InPreviousPath, StartLocation, EndLocation);
				AdvancedPathNode.PreviousPath.Add(VolumeCoordinate, VolumePathNode);
			}
		}
	}

	if (AdvancedPathNode.SpecialPathType != ESpecialPathType::None && AdvancedPathNode.SpecialPathType != ESpecialPathType::SpecialPathSection)
	{
		AdvancedPathNode.GCost = FVector(StartLocation - InExitLocation).Length();
		AdvancedPathNode.HCost = FVector(EndLocation - InExitLocation).Length();
		AdvancedPathNode.FCost = AdvancedPathNode.GCost + AdvancedPathNode.HCost + NodeWeight;
		AdvancedPathNode.FCost += AdvancedPathNode.ElevationToEnd == 0 ? 0.f : AdvancedPathNode.ElevationToEnd * 2.5f;
		return;
	}
	AdvancedPathNode.GCost = FVector(StartLocation - InCurrentCoordinate).Length();
	AdvancedPathNode.HCost = FVector(EndLocation - InCurrentCoordinate).Length();
	AdvancedPathNode.FCost = AdvancedPathNode.GCost + AdvancedPathNode.HCost + NodeWeight;
	AdvancedPathNode.FCost += AdvancedPathNode.ElevationToEnd == 0 ? 0.f : AdvancedPathNode.ElevationToEnd * 2.5f;
}

void ULevelGenerationLibrary::AddUsedAccessPointsToTileData(FTileData& TileData, FIntVector AccessPointCoordinate, FCorridorTileData CorridorTileData)
{
	// Let rooms know which access points are in use
	TArray<EDirections>AdjacentAccessPointArray;
	CorridorTileData.AdjacentAccessPoints.GenerateKeyArray(AdjacentAccessPointArray);
	for (EDirections CurrentDirection : AdjacentAccessPointArray)
	{
		FIntVector ActualDirectionVector = RotateIntVectorCoordinatefromOrigin(DirectionCoordinates[CurrentDirection], TileData.TileRotation.GetInverse());
		EDirections AccessPointDirection = DirectionCoordinates.FindKey(ActualDirectionVector) ? *DirectionCoordinates.FindKey(ActualDirectionVector) : EDirections::None;

		if (AccessPointDirection == EDirections::None) { continue; }

		TileData.TileAccessPoints[AccessPointCoordinate].DirectionsInUse.Add(AccessPointDirection);
	}
}

bool ULevelGenerationLibrary::GetParentNode(const TMap<FIntVector, FAdvancedPathNode>& PathData, FIntVector TargetCoordinate, FIntVector& ParentNode)
{
	// Return false if there is no node at the target coordinate
	if (!PathData.Contains(TargetCoordinate)) { return false; }

	TArray<FIntVector> PreviousPathKeyArray;
	if (!PathData[TargetCoordinate].PreviousPath.IsEmpty())
	{
		PathData[TargetCoordinate].PreviousPath.GenerateKeyArray(PreviousPathKeyArray);
		ParentNode = !PreviousPathKeyArray.IsEmpty() ? PreviousPathKeyArray.Last() : PathData[TargetCoordinate].ParentNode;

		return true;
	}

	ParentNode = FIntVector::ZeroValue;
	return false;
}

FPathGenerationData ULevelGenerationLibrary::GetShortestPathToTargetRoom(FGeneratedLevelData& GeneratedLevelData, const FEdgeInfo InPathData, const TArray<FIntVector> ExcludedOriginAPs, const TArray<FIntVector> ExcludedDestinationAPs)
{
	FPathGenerationData PathGenerationData;
	PathGenerationData.PathData = InPathData;
	PathGenerationData.OriginTile = GeneratedLevelData.LevelTileData.Find(InPathData.Origin);
	PathGenerationData.DestinationTile = GeneratedLevelData.LevelTileData.Find(InPathData.Destination);

	if (PathGenerationData.OriginTile && PathGenerationData.DestinationTile)
	{
		// Find the best access point to use for the starting location and end location
		TArray<FIntVector> OriginTileAccessPoints;
		PathGenerationData.OriginTile->TileAccessPoints.GetKeys(OriginTileAccessPoints);

		TArray<FIntVector> DestinationTileAccessPoints;
		PathGenerationData.DestinationTile->TileAccessPoints.GetKeys(DestinationTileAccessPoints);

		// Check every access point the destination tile has
		for (FIntVector CurrentDestinationAP : DestinationTileAccessPoints)
		{
			TArray<EDirections> DestinationDirectionsArray = PathGenerationData.DestinationTile->TileAccessPoints[CurrentDestinationAP].AccessibleDirections.Array();
			for (EDirections DestinationDirection : DestinationDirectionsArray)
			{
				const FIntVector PotentialPathEnd = PathGenerationData.PathData.Destination + RotateIntVectorCoordinatefromOrigin(CurrentDestinationAP + DirectionCoordinates[DestinationDirection], GeneratedLevelData.LevelTileData[PathGenerationData.PathData.Destination].TileRotation);
				if (ExcludedDestinationAPs.Contains(PotentialPathEnd)) { continue; }

				if (GeneratedLevelData.LevelTileData.Contains(PotentialPathEnd))
				{
					continue;
				}

				// Check every access point the origin tile has		
				for (FIntVector CurrentOriginAP : OriginTileAccessPoints)
				{
					TArray<EDirections> OriginDirectionsArray = PathGenerationData.OriginTile->TileAccessPoints[CurrentOriginAP].AccessibleDirections.Array();
					for (EDirections OriginDirection : OriginDirectionsArray)
					{
						const FIntVector PotentialPathStart = PathGenerationData.PathData.Origin + RotateIntVectorCoordinatefromOrigin(CurrentOriginAP + DirectionCoordinates[OriginDirection], GeneratedLevelData.LevelTileData[PathGenerationData.PathData.Origin].TileRotation);
						if (ExcludedOriginAPs.Contains(PotentialPathStart)) { continue; }

						if (GeneratedLevelData.LevelTileData.Contains(PotentialPathEnd))
						{
							continue;
						}

						// Can both access points reach each other? Connected components (Island ID)
						if (true)
						{
							const float PotentialShortestDistance = FVector(PotentialPathEnd - PotentialPathStart).Length();
							// Shortest distance?
							if (PotentialShortestDistance < PathGenerationData.PathDistance)
							{
								PathGenerationData.OriginAccessPoint = CurrentOriginAP;
								PathGenerationData.OriginAccessPointLocation = PathGenerationData.PathData.Origin + RotateIntVectorCoordinatefromOrigin(CurrentOriginAP, GeneratedLevelData.LevelTileData[PathGenerationData.PathData.Origin].TileRotation);
								PathGenerationData.OriginPathDirection = OriginDirection;

								PathGenerationData.DestinationAccessPoint = CurrentDestinationAP;
								PathGenerationData.DestinationAccessPointLocation = PathGenerationData.PathData.Destination + RotateIntVectorCoordinatefromOrigin(CurrentDestinationAP, GeneratedLevelData.LevelTileData[PathGenerationData.PathData.Destination].TileRotation);
								PathGenerationData.DestinationPathDirection = DestinationDirection;

								PathGenerationData.PathEnd = PotentialPathEnd;
								PathGenerationData.PathStart = PotentialPathStart;

								PathGenerationData.PathDistance = PotentialShortestDistance;
							}
						}
					}
				}
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT(" ULevelGenerationLibrary::GenerateCorridors OriginTile/DestinationTile are invalid!"));
	}

	return PathGenerationData;
}
