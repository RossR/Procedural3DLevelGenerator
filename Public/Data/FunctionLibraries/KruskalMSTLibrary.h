// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Templates/Sorting.h"
#include "Data/FunctionLibraries/DelaunayTriangulationLibrary.h"
#include "KruskalMSTLibrary.generated.h"

UCLASS()
class PROJECTSCIFI_API UKruskalMSTLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/// <summary>
	/// Makes a Minimum Spanning Tree for the results of Delaunay Triangulation.
	/// </summary>
	/// <param name="PointArray"> List of points used in the Delaunay Triangulation. </param>
	/// <param name="QuarterEdgeArray"> List of edges made by the Delaunay Triangulation. </param>
	/// <param name="DiscardedEdgesArray"> Returned list of edges which are not part of the MST. </param>
	/// <returns> The Minimum Spanning Tree of the provided points and edges. </returns>
	UFUNCTION(BlueprintCallable, Category = "Kruskal MST")
	static TArray<FEdgeInfo> GetMinimumSpanningTree(TArray<FIntVector> PointArray, TArray<FQuarterEdge> QuarterEdgeArray, TArray<FEdgeInfo>& DiscardedEdgesArray);

	/// <summary>
	/// Makes a Minimum Spanning Tree for the results of Delaunay Tetrahedralization.
	/// </summary>
	/// <param name="PointArray"> List of points used in the Delaunay Tetrahedralization. </param>
	/// <param name="MSTEdgeArray"> List of edges made by the Delaunay Tetrahedralization. </param>
	/// <param name="DiscardedEdgesArray"> Returned list of edges which are not part of the MST. </param>
	/// <returns> The Minimum Spanning Tree of the provided points and edges. </returns>
	UFUNCTION(BlueprintCallable, Category = "Kruskal MST")
	static TArray<FEdgeInfo> GetMinimumSpanningTreeV2(TArray<FIntVector> PointArray, TArray<FEdgeInfo> EdgeArray, TArray<FEdgeInfo>&DiscardedEdgesArray);

	/// <summary>
	/// Displays the MST in the game world.
	/// </summary>
	/// <param name="MSTEdgeArray"> The MST to be drawn. </param>
	/// <param name="WorldRef"> Reference of the game world. </param>
	/// <param name="TileSize"> The size of the level grid used in the procedural level generation. </param>
	/// <param name="TraceColour"> The colour of the MST drawing. </param>
	UFUNCTION(BlueprintCallable, Category = "Kruskal MST")
	static void DrawMST(TArray<FEdgeInfo> MSTEdgeArray, UObject* WorldRef, int TileSize, FLinearColor TraceColour);
	
	/// <summary>
	/// Randomly adds some of the discarded edges to the MST array.
	/// </summary>
	/// <param name="MinimumSpanningTree"> List of edges that represents the MST. </param>
	/// <param name="DiscardedEdgesArray"> List of edges which are not part of the MST. </param>
	/// <param name="LevelSeed"> The seed being used for the current level generation. </param>
	/// <param name="AddToMSTChance"> Percentage chance for each discarded edge to be added to the MST. </param>
	UFUNCTION(BlueprintCallable, Category = "Kruskal MST")
	static void RandomlyAddEdgesToMST(TArray<FEdgeInfo>& MinimumSpanningTree, TArray<FEdgeInfo> DiscardedEdgesArray, FRandomStream LevelSeed, float AddToMSTChance = 0.2f);

protected:

	//function to compare edges using qsort() in C programming
	static bool EdgeComparison(const FEdgeInfo& EdgeA, const FEdgeInfo& EdgeB);

private:



};