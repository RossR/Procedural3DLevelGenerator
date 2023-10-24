// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Data/LevelGenerationData.h"
#include "LevelGenerationLibrary.generated.h"


struct FPathGenerationData
{

public:

	FEdgeInfo PathData;

	FTileData* OriginTile = nullptr;
	FIntVector OriginAccessPoint = FIntVector::ZeroValue;
	FIntVector OriginAccessPointLocation = FIntVector::ZeroValue;
	EDirections OriginPathDirection = EDirections::None;
	
	FTileData* DestinationTile = nullptr;
	FIntVector DestinationAccessPoint = FIntVector::ZeroValue;
	FIntVector DestinationAccessPointLocation = FIntVector::ZeroValue;
	EDirections DestinationPathDirection = EDirections::None;
	

	FIntVector PathStart = FIntVector::ZeroValue;
	FIntVector PathEnd = FIntVector::ZeroValue;

	float PathDistance = FLT_MAX;

};


UCLASS()
class PROJECTSCIFI_API ULevelGenerationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:

	/// <summary>
	/// Procedurally generate a level.
	/// </summary>
	/// <param name="LevelGenerationSettings"> The settings that determine how the level generates. </param>
	/// <param name="GeneratedLevelData"> Struct containing all the generated level's data. </param>
	/// <param name="WorldRef"> Reference to the world. </param>
	UFUNCTION(BlueprintCallable, Category = "Level Generation")
	static void GenerateLevel(FLevelGenerationSettings& LevelGenerationSettings, FGeneratedLevelData& GeneratedLevelData, UObject* WorldRef);

	/// <summary>
	/// Rotates an IntVector about the origin point (0, 0, 0).
	/// </summary>
	/// <param name="InCoordinate"> The IntVector to be rotated. </param>
	/// <param name="TileRotation"> The amount of degrees to rotate the coordinate (must be in 90° intervals). </param>
	/// <returns> The rotated IntVector. </returns>
	UFUNCTION(BlueprintCallable, Category = "Level Generation")
	static FIntVector RotateIntVectorCoordinatefromOrigin(FIntVector InCoordinate, FRotator TileRotation);

	/// <summary>
	/// Rotates a vector about the origin point (0, 0, 0).
	/// </summary>
	/// <param name="InCoordinate"> The vector to be rotated. </param>
	/// <param name="TileRotation"> The amount of degrees to rotate the coordinate (must be in 90° intervals). </param>
	/// <returns> The rotated vector. </returns>
	UFUNCTION(BlueprintCallable, Category = "Level Generation")
	static FVector RotateVectorCoordinatefromOrigin(FVector InCoordinate, FRotator TileRotation);

	/// <summary>
	/// Rotates the direction by the given rotation.
	/// </summary>
	/// <param name="InDirection"> The direction to be rotated. </param>
	/// <param name="InRotation"> The amount of degrees to rotate the direction (must be in 90° intervals). </param>
	/// <returns> The new direction after rotating. </returns>
	UFUNCTION(BlueprintCallable, Category = "Level Generation")
	static EDirections RotateDirection(EDirections InDirection, FRotator InRotation);

protected:

	/// <summary>
	/// Generates the key rooms for the level (if any).
	/// </summary>
	/// <param name="LevelGenerationSettings"> The settings that determine how the level generates.</param>
	/// <param name="GeneratedLevelData"> Struct containing all the generated level's data. </param>
	static void GenerateKeyRooms(FLevelGenerationSettings& LevelGenerationSettings, FGeneratedLevelData& GeneratedLevelData);

	/// <summary>
	/// Generates the special rooms for the level (if any).
	/// </summary>
	/// <param name="LevelGenerationSettings"> The settings that determine how the level generates.</param>
	/// <param name="GeneratedLevelData"> Struct containing all the generated level's data. </param>
	static void GenerateSpecialRooms(FLevelGenerationSettings& LevelGenerationSettings, FGeneratedLevelData& GeneratedLevelData);

	/// <summary>
	/// Generates the basic rooms for the level.
	/// </summary>
	/// <param name="LevelGenerationSettings"> The settings that determine how the level generates.</param>
	/// <param name="GeneratedLevelData"> Struct containing all the generated level's data. </param>
	static void GenerateBasicRooms(FLevelGenerationSettings& LevelGenerationSettings, FGeneratedLevelData& GeneratedLevelData);

	/// <summary>
	/// Generates the corridors for the level.
	/// </summary>
	/// <param name="LevelGenerationSettings"> The settings that determine how the level generates.</param>
	/// <param name="GeneratedLevelData"> Struct containing all the generated level's data. </param>
	/// <param name="WorldRef"> Reference to the world. </param>
	static void GenerateCorridors3D(FLevelGenerationSettings& LevelGenerationSettings, FGeneratedLevelData& GeneratedLevelData, UObject* WorldRef);
	
	/// <summary>
	/// Attempts to place a room in the level grid.
	/// </summary>
	/// <param name="LevelGenerationSettings"> The settings that determine how the level generates.</param>
	/// <param name="GeneratedLevelData"> Struct containing all the generated level's data. </param>
	/// <param name="RoomList"> List of rooms to take a room from, then place in the grid. </param>
	static void PlaceRoomInGrid(FLevelGenerationSettings& LevelGenerationSettings, FGeneratedLevelData& GeneratedLevelData, UDataTable* RoomList);

	/// <summary>
	/// Randomly selects a room list from the provided data table.
	/// </summary>
	/// <param name="RoomListDataTable"> Data table containing the room lists. </param>
	/// <param name="LevelStream"> The seed of the level, determines how randomisation is handled in the level generation. </param>
	/// <returns> A random room list from the data table. </returns>
	static UDataTable* GetRandomRoomListFromDataTable(TMap<UDataTable*, double>& RoomListDataTable, const FRandomStream& LevelStream);

	/// <summary>
	/// Randomly selects a room from the room list.
	/// </summary>
	/// <param name="RoomList"> List of rooms to use in the level generation. </param>
	/// <param name="LevelStream"> The seed of the level, determines how randomisation is handled in the level generation. </param>
	/// <returns> A random room from the list. </returns>
	static FTileGenerationData* GetRandomRoomFromRoomList(UDataTable* RoomList, const FRandomStream& LevelStream);

	/// <summary>
	/// Randomly selects a corridor section from the corridor list.
	/// </summary>
	/// <param name="CorridorList"> List of corridor sections to use in the level generation. </param>
	/// <param name="LevelStream"> The seed of the level, determines how randomisation is handled in the level generation. </param>
	/// <returns> A random corridor section from the list. </returns>
	static FCorridorLevelData* GetRandomCorridorFromCorridorList(UDataTable* CorridorList, const FRandomStream& LevelStream);

	/// <summary>
	/// Randomly selects a rotation to be used by a room tile. The possible rotations are 0°, 90°, 180°, 270°.
	/// </summary>
	/// <param name="LevelStream"> The seed of the level, determines how randomisation is handled in the level generation. </param>
	/// <returns> A random room rotation. </returns>
	static FRotator GetRandomRoomRotation(const FRandomStream& LevelStream);

	/// <summary>
	/// Randomly select an empty space in the level grid.
	/// </summary>
	/// <param name="LevelGenerationSettings"> The settings that determine how the level generates.</param>
	/// <param name="LevelStream"> The seed of the level, determines how randomisation is handled in the level generation. </param>
	/// <param name="LevelTileData"> TMap containing the all the rooms and corridors currently generated in the level.</param>
	/// <param name="bEmptyCoordinateFound"> Returns true if an empty space is found in the level grid. </param>
	/// <returns> The location of an empty tile in the level grid. </returns>
	static FIntVector GetRandomEmptyCoordinate(FLevelGenerationSettings& LevelGenerationSettings, const FRandomStream& LevelStream, TMap<FIntVector, FTileData>& LevelTileData, bool& bEmptyCoordinateFound);

	/// <summary>
	/// Checks if the room can be placed at the desired coordinate.
	/// </summary>
	/// <param name="LevelGenerationSettings"> The settings that determine how the level generates.</param>
	/// <param name="GeneratedLevelData"> Struct containing all the generated level's data. </param>
	/// <param name="TileAccessPoints"> The sections of the room which have access points that can lead to other areas of the level. </param>
	/// <param name="PlacementCoordinate">The coordinate that the room will be placed at. </param>
	/// <param name="RoomRotation"> The rotation of the room in world space. </param>
	/// <param name="TileSize"> The size of the room in the level grid space. </param>
	/// <returns> True if the room can be placed at the desired coordinate.</returns>
	static bool RoomPlacementIsValid(const FLevelGenerationSettings& LevelGenerationSettings, const FGeneratedLevelData& GeneratedLevelData, const TMap<FIntVector, FTileAccessData> TileAccessPoints, const FIntVector PlacementCoordinate, const FRotator RoomRotation, const TSet<FIntVector> TileSize);

	/// <summary>
	/// Checks if the provided coordinate is inside or outside the level grid.
	/// </summary>
	/// <param name="Coordinate"> The coordinate we are checking. </param>
	/// <param name="GridSize"> The size of the level grid. </param>
	/// <returns> True if the coordinate is inside the level grid. </returns>
	static bool IsCoordinateInGridSpace(const FIntVector& Coordinate, const FIntVector& GridSize);

	/// <summary>
	/// Checks if the buffer surrounding a room is empty.
	/// </summary>
	/// <param name="LevelGenerationSettings"> The settings that determine how the level generates.</param>
	/// <param name="LevelTileData"> TMap containing the all the rooms and corridors currently generated in the level.</param>
	/// <param name="RoomCoordinate"> The coordinate of the room we are checking the buffer of. </param>
	/// <param name="RoomSectionCoordinate"> The coordinate of the current room section we are checking the buffer of. </param>
	/// <param name="RoomRotation"> The rotation of the room in world space. </param>
	/// <param name="TileSize"> The size of the room in the level grid space. </param>
	/// <returns> True if there is nothing in the room's buffer space. </returns>
	static bool IsRoomBufferEmpty(const FLevelGenerationSettings& LevelGenerationSettings, const TMap<FIntVector, FTileData>& LevelTileData, const FIntVector& RoomCoordinate, const FIntVector& RoomSectionCoordinate, const FRotator& RoomRotation, const TSet<FIntVector>& TileSize);

	/// <summary>
	/// An altered version of the A* Pathfinding algorithm. Used to build a path between two access points in 3D space using special corridor structures, e.g. Stairways, elevators.
	/// </summary>
	/// <param name="StartLocation"> The starting point of the path. </param>
	/// <param name="EndLocation"> The end goal of the path. </param>
	/// <param name="PathData"> TMap containing all the data of the generated path. </param>
	/// <param name="PathGenerationDataArray"> Array containing all the paths that need to be built. </param>
	/// <param name="LevelGenerationSettings"> The settings that determine how the level generates.</param>
	/// <param name="GeneratedLevelData"> Struct containing all the generated level's data. </param>
	/// <returns> True if the path is successfully created. </returns>
	static bool AdvancedAStarPathfinding(FIntVector StartLocation, FIntVector EndLocation, TMap<FIntVector, FAdvancedPathNode>& PathData, const TArray<FPathGenerationData>& PathGenerationDataArray, FLevelGenerationSettings& LevelGenerationSettings, FGeneratedLevelData& GeneratedLevelData);

	/// <summary>
	/// Creates tile data from the provided corridor tile data.
	/// </summary>
	/// <param name="CorridorTileData"> The data we are going to convert into an FTileData. </param>
	/// <param name="LevelGenerationSettings"> The settings that determine how the level generates. </param>
	/// <param name="LevelStream"> The seed of the level, determines how randomisation is handled in the level generation. </param>
	/// <returns> FTileData made from the corridor tile data. </returns>
	static FTileData GetTileDataFromCorridorTileData(FCorridorTileData CorridorTileData, FLevelGenerationSettings& LevelGenerationSettings, const FRandomStream& LevelStream);

	/// <summary>
	/// Creates tile data from the provided special corridor tile data and adds it to the generated level data.
	/// </summary>
	/// <param name="Coordinate"> Coordinate of the current path section we are adding to the generated level data. </param>
	/// <param name="CorridorTileData"> The data we are going to convert into an FTileData and add to the generated level data. </param>
	/// <param name="LevelGenerationSettings"> The settings that determine how the level generates. </param>
	/// <param name="GeneratedLevelData"> Struct containing all the generated level's data. </param>
	static void AddTileDataFromSpecialCorridorTileData(FIntVector Coordinate, FCorridorTileData CorridorTileData, FLevelGenerationSettings& LevelGenerationSettings, FGeneratedLevelData& GeneratedLevelData);

	/// <summary>
	/// Finds the direction of the target vector from the perspective of the start vector. 
	/// </summary>
	/// <param name="StartVector"> The vector we are looking from. </param>
	/// <param name="TargetVector"> The vector we are looking at. </param>
	/// <returns> The direction of the target vector. </returns>
	static EDirections GetDirectionForIntVectors(FIntVector StartVector, FIntVector TargetVector);

	/// <summary>
	/// Evaluates the nearby nodes to see if they can be reached by a special corridor structure and get the path closer to the end location. 
	/// </summary>
	/// <param name="OPEN"> Set of nodes in the A* Pathfinding that are to be evaluated. </param>
	/// <param name="CLOSED"> Set of nodes in the A* Pathfinding that have been evaluated. </param>
	/// <param name="InaccessibleNodes"> Set of nodes in the A* Pathfinding which cannot be traversed. </param>
	/// <param name="PathGenerationDataArray"> Array containing all the paths that need to be built. </param>
	/// <param name="LevelGenerationSettings"> The settings that determine how the level generates. </param>
	/// <param name="GeneratedLevelData"> Struct containing all the generated level's data. </param>
	/// <param name="CurrentClosedNode"> The current closed node which is being used to find nodes that need to be evaluated. </param>
	/// <param name="CurrentDirection"> The current direction of the closed node which we are checking for nodes that need to be evaluated. </param>
	/// <param name="StartLocation"> The starting point of the path. </param>
	/// <param name="EndLocation"> The end goal of the path. </param>
	static void EvaluateSpecialCorridorStructures(TMap<FIntVector, FAdvancedPathNode>& OPEN, TMap<FIntVector, FAdvancedPathNode>& CLOSED, TSet<FIntVector>& InaccessibleNodes, const TArray<FPathGenerationData>& PathGenerationDataArray, const FLevelGenerationSettings& LevelGenerationSettings, const FGeneratedLevelData& GeneratedLevelData, const FIntVector CurrentClosedNode, EDirections CurrentDirection, const FIntVector StartLocation, const FIntVector EndLocation);

private:

	/// <summary>
	/// Checks to see if the provided coordinate is an empty tile in the level grid or is not a closed or inaccessible node in the current A* Pathfinding execution. 
	/// </summary>
	/// <param name="Coordinate"> The location in the level grid/A* Pathfinding we are evaluating. </param>
	/// <param name="CurrentClosedNode"> The current closed node which is being used to check if the coordinate is empty. </param>
	/// <param name="OPEN"> Set of nodes in the A* Pathfinding that are to be evaluated. </param>
	/// <param name="CLOSED"> Set of nodes in the A* Pathfinding that have been evaluated. </param>
	/// <param name="InaccessibleNodes"> Set of nodes in the A* Pathfinding which cannot be traversed. </param>
	/// <param name="PathGenerationDataArray"> Array containing all the paths that need to be built. </param>
	/// <param name="GeneratedLevelData"> Struct containing all the generated level's data. </param>
	/// <returns>True if the provided coordinate does not contain a tile or is not a closed/inaccessible node. </returns>
	static bool IsCoordinateEmpty(const FIntVector Coordinate, const FIntVector CurrentClosedNode, const TMap<FIntVector, FAdvancedPathNode> OPEN, const TMap<FIntVector, FAdvancedPathNode> CLOSED, const TSet<FIntVector> InaccessibleNodes, const TArray<FPathGenerationData> PathGenerationDataArray, const FGeneratedLevelData& GeneratedLevelData);

	/// <summary>
	/// Generates FCorridorTileData for a basic corridor structure from the provided path data and adds it to the generated level data.
	/// </summary>
	/// <param name="CurrentPathVector"> The location of the current path section that we are adding to the generated level data. </param>
	/// <param name="PathData"> TMap containing all the data of the path generated by A* Pathfinding. </param>
	/// <param name="CurrentPathGenData"> The FPathGenerationData of the current path that has been generated by A* Pathfinding. </param>
	/// <param name="PreviousPathVector"> The coordinate of the previous path section. </param>
	/// <param name="PreviousPathTileType"> The ETileType of the previous path section. </param>
	/// <param name="GeneratedLevelData"> Struct containing all the generated level's data. </param>
	static void GenerateNormalPathData(const FIntVector CurrentPathVector, const TMap<FIntVector, FAdvancedPathNode>& PathData, const FPathGenerationData& CurrentPathGenData, FIntVector& PreviousPathVector, ETileType& PreviousPathTileType, FGeneratedLevelData& GeneratedLevelData);

	/// <summary>
	/// Generates FCorridorTileData for a special corridor structure from the provided path data and adds it to the generated level data.
	/// </summary>
	/// <param name="CurrentPathVector"> The location of the current path section that we are adding to the generated level data. </param>
	/// <param name="PathData"> TMap containing all the data of the path generated by A* Pathfinding. </param>
	/// <param name="CurrentPathGenData"> The FPathGenerationData of the current path that has been generated by A* Pathfinding. </param>
	/// <param name="PreviousPathVector"> The coordinate of the previous path section. </param>
	/// <param name="PreviousPathTileType"> The ETileType of the previous path section. </param>
	/// <param name="GeneratedLevelData"> Struct containing all the generated level's data. </param>
	static void GenerateSpecialPathData(const FIntVector CurrentPathVector, const TMap<FIntVector, FAdvancedPathNode>& PathData, const FPathGenerationData& CurrentPathGenData, FIntVector& PreviousPathVector, ETileType& PreviousPathTileType, FGeneratedLevelData& GeneratedLevelData);
	
	/// <summary>
	/// Updates the pathfinding node with new data.
	/// </summary>
	/// <param name="AdvancedPathNode"> The path node being updated. </param>
	/// <param name="LevelGenerationSettings"> The settings that determine how the level generates. </param>
	/// <param name="GeneratedLevelData"> Struct containing all the generated level's data. </param>
	/// <param name="InSpecialPathOriginVector"> The origin vector of the path node, used by special corridor structures.</param>
	/// <param name="InCurrentCoordinate"> The location of the path node in the level grid. </param>
	/// <param name="InExitLocation"> The endpoint of the path node where the pathfinding can continue. </param>
	/// <param name="InSpecialPathType"> The ESpecialPathType of the node, if any.  </param>
	/// <param name="InSpecialPathInfo"> Struct containing the A* Pathfinding info for the special corridor structure of the node, if any. </param>
	/// <param name="InSpecialPathRotation"> The rotation of the node's special corridor structure, if any. </param>
	/// <param name="InPreviousPath"> TMap containing all the previous evaluated path nodes needed to reach this node. </param>
	/// <param name="StartLocation"> The starting point of the path. </param>
	/// <param name="EndLocation"> The end goal of the path. </param>
	static void UpdateAdvancedNode(FAdvancedPathNode& AdvancedPathNode, FLevelGenerationSettings LevelGenerationSettings, FGeneratedLevelData GeneratedLevelData, FIntVector InSpecialPathOriginVector, FIntVector InCurrentCoordinate, FIntVector InExitLocation, ESpecialPathType InSpecialPathType, FSpecialPathInfo InSpecialPathInfo, FRotator InSpecialPathRotation, TMap<FIntVector, FAdvancedPathNode*> InPreviousPath, FIntVector StartLocation, FIntVector EndLocation);

	/// <summary>
	/// Takes the adjacent access points of the corridor tile data and adds them as used directions to the specified access point of the tile data.
	/// </summary>
	/// <param name="TileData"> The FTileData of the corridor section. </param>
	/// <param name="AccessPointCoordinate"> The location of the access point that needs to be updated. </param>
	/// <param name="CorridorTileData"> The FCorridorTileData of the corridor section. </param>
	static void AddUsedAccessPointsToTileData(FTileData& TileData, FIntVector AccessPointCoordinate, FCorridorTileData CorridorTileData);

	/// <summary>
	/// Finds the parent node of the node at the target coordinate.
	/// </summary>
	/// <param name="PathData"> TMap containing all the data of the path generated by A* Pathfinding. </param>
	/// <param name="TargetCoordinate"> The location of the node who we want to find the parent of. </param>
	/// <param name="ParentNode"> The location of the parent node if one is found. </param>
	/// <returns> True if the parent node is found. </returns>
	static bool GetParentNode(const TMap<FIntVector, FAdvancedPathNode>& PathData, FIntVector TargetCoordinate, FIntVector& ParentNode);

	/// <summary>
	/// Finds the best access points needed to connect two rooms together.
	/// </summary>
	/// <param name="GeneratedLevelData"> Struct containing all the generated level's data. </param>
	/// <param name="InPathData"> TMap containing all the data of the path generated by A* Pathfinding.</param>
	/// <param name="ExcludedOriginAPs"> List of access points in the origin tile to ignore when finding the shortest path. </param>
	/// <param name="ExcludedDestinationAPs"> List of access points in the destination tile to ignore when finding the shortest path. </param>
	/// <returns> The shortest path connecting both rooms together. </returns>
	static FPathGenerationData GetShortestPathToTargetRoom(FGeneratedLevelData& GeneratedLevelData, const FEdgeInfo InPathData, const TArray<FIntVector> ExcludedOriginAPs, const TArray<FIntVector> ExcludedDestinationAPs);

};