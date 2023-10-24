// Fill out your copyright notice in the Description page of Project Settings.


#include "Data/FunctionLibraries/KruskalMSTLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Runtime\MeshUtilitiesCommon\Public\DisjointSet.h"
//#include "DisjointSet.h"

TArray<FEdgeInfo> UKruskalMSTLibrary::GetMinimumSpanningTree(TArray<FIntVector> PointArray, TArray<FQuarterEdge> QuarterEdgeArray, TArray<FEdgeInfo>& DiscardedEdgesArray)
{
	const int TreeSize = PointArray.Num() - 1.f;

	TArray<FEdgeInfo> ConvertedEdgesArray;

	for (FQuarterEdge CurrentEdge : QuarterEdgeArray)
	{
		FEdgeInfo NewEdgeInfo{ (FIntVector)CurrentEdge.Data, (FIntVector)UDelaunayTriangulationLibrary::Destination(&CurrentEdge) };
		//{ (FIntVector)CurrentEdge.Data, (FIntVector)UDelaunayTriangulationLibrary::Destination(&CurrentEdge), (CurrentEdge.Data - UDelaunayTriangulationLibrary::Destination(&CurrentEdge)).Length()};
		ConvertedEdgesArray.Add(NewEdgeInfo);
	}

	if (ConvertedEdgesArray.IsEmpty()) { return TArray<FEdgeInfo>(); }

	TArray<FEdgeInfo> MinimumSpanningTree;
	//TArray<FEdgeInfo> DiscardedEdgeArray;

	// TotalPoints = Points in grid
	// EdgeCount = Edges connecting the points together

	
	// Step 1. Sort all edges in increasing order of their CurrentEdge weights.

	//ConvertedEdgesArray.Sort(FEdgeInfo::EdgeComparison);

	ConvertedEdgesArray.Sort([](const FEdgeInfo& EdgeA, const FEdgeInfo& EdgeB) -> bool {
		// sort by weight
		if (EdgeA.Weight < EdgeB.Weight) return true;
		if (EdgeA.Weight > EdgeB.Weight) return false;
		else { return false; }
		});

	FDisjointSet DisjointSet(PointArray.Num());

	do
	{
		// Step 2. Pick the smallest CurrentEdge.
		FEdgeInfo SmallestEdge = ConvertedEdgesArray[0];

		float EdgeWeight = SmallestEdge.Weight;
		int32 Origin;
		int32 Destination;

		PointArray.Find(SmallestEdge.Origin, Origin);
		PointArray.Find(SmallestEdge.Destination, Destination);

		// Step 3. Check if the new CurrentEdge creates a cycle or loop in a spanning tree.
		if (DisjointSet.Find(Origin) != DisjointSet.Find(Destination))
		{
			DisjointSet.Union(Origin, Destination);
			MinimumSpanningTree.Add(SmallestEdge);
			ConvertedEdgesArray.RemoveAt(0);
		}
		// Step 4. If it doesn’t form the cycle, then include that CurrentEdge in MST.Otherwise, discard it.
		else
		{
			DiscardedEdgesArray.Add(SmallestEdge);
			ConvertedEdgesArray.RemoveAt(0);
		}

		// Step 5. Repeat from step 2 until it includes | TotalPoints | -1 edges in MST.
		if (MinimumSpanningTree.Num() == TreeSize) { break; }
	} while (true);

	if (!ConvertedEdgesArray.IsEmpty())
	{
		for (FEdgeInfo CurrentEdge : ConvertedEdgesArray)
		{
			DiscardedEdgesArray.Add(CurrentEdge);
		}
	}

	return MinimumSpanningTree;
}

TArray<FEdgeInfo> UKruskalMSTLibrary::GetMinimumSpanningTreeV2(TArray<FIntVector> PointArray, TArray<FEdgeInfo> EdgeArray, TArray<FEdgeInfo>& DiscardedEdgesArray)
{
	const int TreeSize = PointArray.Num() - 1.f;


	if (EdgeArray.IsEmpty()) { return TArray<FEdgeInfo>(); }

	TArray<FEdgeInfo> MinimumSpanningTree;

	EdgeArray.Sort([](const FEdgeInfo& EdgeA, const FEdgeInfo& EdgeB) -> bool {
		// Step 1. Sort the edges by weight
		if (EdgeA.Weight < EdgeB.Weight) return true;
		if (EdgeA.Weight > EdgeB.Weight) return false;
		else { return false; }
		});

	FDisjointSet DisjointSet(PointArray.Num());

	do
	{
		// Step 2. Pick the smallest CurrentEdge.
		FEdgeInfo SmallestEdge = EdgeArray[0];

		float EdgeWeight = SmallestEdge.Weight;
		int32 Origin;
		int32 Destination;

		PointArray.Find(SmallestEdge.Origin, Origin);
		PointArray.Find(SmallestEdge.Destination, Destination);

		// Step 3. Check if the new CurrentEdge creates a cycle or loop in a spanning tree.
		if (DisjointSet.Find(Origin) != DisjointSet.Find(Destination))
		{
			DisjointSet.Union(Origin, Destination);
			MinimumSpanningTree.Add(SmallestEdge);
			EdgeArray.RemoveAt(0);
		}
		// Step 4. If it doesn’t form the cycle, then include that CurrentEdge in MST.Otherwise, discard it.
		else
		{
			DiscardedEdgesArray.Add(SmallestEdge);
			EdgeArray.RemoveAt(0);
		}

		// Step 5. Repeat from step 2 until it includes | TotalPoints | -1 edges in MST.
		if (MinimumSpanningTree.Num() == TreeSize) { break; }
	} while (true);

	if (!EdgeArray.IsEmpty())
	{
		for (FEdgeInfo CurrentEdge : EdgeArray)
		{
			DiscardedEdgesArray.Add(CurrentEdge);
		}
	}

	return MinimumSpanningTree;
}

bool UKruskalMSTLibrary::EdgeComparison(const FEdgeInfo& EdgeA, const FEdgeInfo& EdgeB)
{
	return EdgeA.Weight > EdgeB.Weight;
	//FEdgeInfo* a1 = (FEdgeInfo*)EdgeA;

	//FEdgeInfo* b1 = (FEdgeInfo*)EdgeB;

	//return a1->Weight > b1->Weight;
}

void UKruskalMSTLibrary::DrawMST(TArray<FEdgeInfo> EdgeArray, UObject* WorldRef, int TileSize, FLinearColor TraceColour)
{
	if (!WorldRef) { return; }

	for (FEdgeInfo CurrentEdge : EdgeArray)
	{
		FVector Start = (FVector)CurrentEdge.Origin * TileSize;
		FVector End = (FVector)CurrentEdge.Destination * TileSize;

		TArray<FHitResult> HitResults;

		UKismetSystemLibrary::LineTraceMulti(WorldRef, Start + FVector(0.f, 0.f, 1000.f), End + FVector(0.f, 0.f, 1000.f), ETraceTypeQuery::TraceTypeQuery1, false, TArray<AActor*>(), EDrawDebugTrace::Persistent, HitResults, false, TraceColour);
	}
}

void UKruskalMSTLibrary::RandomlyAddEdgesToMST(TArray<FEdgeInfo>& MinimumSpanningTree, TArray<FEdgeInfo> DiscardedEdgesArray, FRandomStream Stream, float AddToMSTChance)
{
	if (DiscardedEdgesArray.IsEmpty()) { return; }

	for (FEdgeInfo CurrentEdge : DiscardedEdgesArray)
	{
		float RandomChanceResult = UKismetMathLibrary::RandomFloatInRangeFromStream(0.f, 1.f, Stream);

		if (RandomChanceResult <= AddToMSTChance)
		{
			MinimumSpanningTree.Add(CurrentEdge);
		}
	}
	
	return;
}
