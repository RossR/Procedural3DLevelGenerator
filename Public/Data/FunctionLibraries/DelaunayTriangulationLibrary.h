// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DelaunayTriangulationLibrary.generated.h"

// This blog was invaluable to the implementation of the 2D algorithm: https://ianthehenry.com/posts/delaunay/

/* Useful papers for Implementing the Bowyer-Watson algorithm in 3D space: 
    http://www.gdmc.nl/publications/2007/Computing_3D_Voronoi_Diagram.pdf
    https://people.eecs.berkeley.edu/~jrs/meshpapers/delnotes.pdf
*/

// Useful site for troubleshooting the tetrahedrons: https://www.geogebra.org/calculator

#define SMALL_NUM   0.00000001 // anything that avoids division overflow
// dot product (3D) which allows vector operations in arguments
#define dot(u,v)   ((u).X * (v).X + (u).Y * (v).Y + (u).Z * (v).Z)

/** Structure representing a quarter edge (quad-edge ref). */
USTRUCT(BlueprintType)
struct FQuarterEdge
{
    GENERATED_USTRUCT_BODY()

public:
    
    UPROPERTY(BlueprintReadWrite)
    FVector Data;

    // Points to the FQuarterEdge that has the same starting point as this FQuarterEdge & lies immediately anti-clockwise of this FQuarterEdge
    FQuarterEdge *Next;
    // Points to the FQuarterEdge that is anti-clockwise of this FQuarterEdge on the same FQuadEdge
    FQuarterEdge *Rot;

};

/** Structure representing the vertices of a triangle. */
USTRUCT(BlueprintType)
struct FTriangle
{
    GENERATED_USTRUCT_BODY()

public:

    UPROPERTY(BlueprintReadWrite)
    FVector A = FVector::ZeroVector;
    UPROPERTY(BlueprintReadWrite)
    FVector B = FVector::ZeroVector;
    UPROPERTY(BlueprintReadWrite)
    FVector C = FVector::ZeroVector;

};

/** Structure representing the verties of a tetrahedron. */
USTRUCT()
struct FTetrahedron
{
    GENERATED_USTRUCT_BODY()

public:

    FVector A = FVector::ZeroVector;
    FVector B = FVector::ZeroVector;
    FVector C = FVector::ZeroVector;
    FVector D = FVector::ZeroVector;

    /** Returns true if the edge is one of the edges that makes up the tetrahedron. */
    bool HasEdge(FVector InA, FVector InB)
    {
        if ((InA == A && (InB == B || InB == C || InB == D)) ||
            (InA == B && (InB == C || InB == D || InB == A)) ||
            (InA == C && (InB == D || InB == A || InB == B)) ||
            (InA == D && (InB == A || InB == B || InB == C)))
        {
            return true;
        }
        else { return false; }
    }

};

/** Structure that denotes a weighted edge. */
USTRUCT(BlueprintType)
struct FEdgeInfo
{
    GENERATED_USTRUCT_BODY()

public:

    UPROPERTY(VisibleAnywhere, Category = "Edge Info")
    FIntVector Origin = FIntVector::ZeroValue;

    UPROPERTY(VisibleAnywhere, Category = "Edge Info")
    FIntVector Destination = FIntVector::ZeroValue;

    UPROPERTY(VisibleAnywhere, Category = "Edge Info")
    float Weight = 0.f;

    FEdgeInfo(FIntVector InA = {0,0,0}, FIntVector InB = { 0,0,0 })
        : Origin(InA)
        , Destination(InB)
        , Weight(((FVector)InA - (FVector)InB).Length())
    {
    }

    bool operator==(const FEdgeInfo& Other) const
    {
        return (Origin == Other.Origin && Destination == Other.Destination) || (Origin == Other.Destination && Destination == Other.Origin);
    }

    friend uint32 GetTypeHash(const FEdgeInfo& Edge)
    {
        return HashCombine(GetTypeHash(Edge.Origin), GetTypeHash(Edge.Destination));
    }

};

UCLASS()
class PROJECTSCIFI_API UDelaunayTriangulationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

    // Returns the edge that is rotated anti-clockwise from the given edge around the same face.
    static FQuarterEdge *Rotate(FQuarterEdge *FEdgeInfo) {
        return FEdgeInfo->Rot;
    }

    // Returns the edge that is symmetric the the given edge (same edge but in the opposite direction).
    static FQuarterEdge *SymmetricEdge(FQuarterEdge *FEdgeInfo) {
        return FEdgeInfo->Rot->Rot;
    }

    // Returns the edge that is rotated clockwise from the given edge around the same face.
    static FQuarterEdge *ReverseRotate(FQuarterEdge *FEdgeInfo) {
        return FEdgeInfo->Rot->Rot->Rot;
    }

    // Go to the previous edge that shares the same point (navigate clockwise)
    static FQuarterEdge *Previous(FQuarterEdge *FEdgeInfo) {
        return FEdgeInfo->Rot->Next->Rot;
    }

    // Navigate around the triangle to the left of the current edge
    static FQuarterEdge *LNext(FQuarterEdge *FEdgeInfo) {
        return ReverseRotate(FEdgeInfo)->Next->Rot;
    }

    // Navigate around the triangle to the right of the current edge
    static FQuarterEdge* RNext(FQuarterEdge* FEdgeInfo) {
        return SymmetricEdge(FEdgeInfo)->Next;
    }

    // Returns the data of the given edge.
    static FVector Destination(FQuarterEdge *FEdgeInfo) {
        return SymmetricEdge(FEdgeInfo)->Data;
    }

public:

    /// <summary>
    /// Delaunay Triangulation using the Guibas and Stolfi algorithm.
    /// </summary>
    /// <param name="GridSize"> The size of each grid space that the points sit on. </param>
    /// <param name="PointArray"> List of points to use in the Delaunay Triangulation. </param>
    /// <param name="bRemoveBoundaryTriangle"> If true, edges that touch the boundary triangle will be removed from the returned array. </param>
    /// <returns> List of edges made by the Delaunay Triangulation. </returns>
    UFUNCTION(BlueprintCallable, Category = "Triangulation")
    static TArray<FQuarterEdge> GuibasStolfi(FIntVector GridSize, TArray<FIntVector> PointArray, bool bRemoveBoundaryTriangle = true);

    /// <summary>
    /// Delaunay Tetrahedralization using the Bowyer-Watson algorithm.
    /// </summary>
    /// <param name="GridSize"> The size of each grid space that the points sit on. </param>
    /// <param name="PointArray"> List of points to use in the Delaunay Triangulation. </param>
    /// <param name="bRemoveBoundaryTetrahedron"> If true, edges that touch the boundary tetrahedron will be removed from the returned array. </param>
    /// <returns> List of edges made by the Delaunay Tetrahedralization. </returns>
    UFUNCTION(BlueprintCallable, Category = "Triangulation")
    static TArray<FEdgeInfo> DelaunayTetrahedralization(FIntVector GridSize, TArray<FIntVector> PointArray, bool bRemoveBoundaryTetrahedron = true);

    /** Returns a quarter edge made from the provided vectors. */
    static FQuarterEdge *MakeQuadEdge(FVector Start, FVector End);

    /** Updates the triangulation by replacing adjacent triangles with new triangles while maintaining the Delaunay property. */
    static void Splice(FQuarterEdge *EdgeA, FQuarterEdge *EdgeB);

    /** Internal function of Splice, swaps the Nexts of the provided quarter edges. */
    static void SwapNexts(FQuarterEdge *EdgeA, FQuarterEdge *EdgeB);

    /** Returns one edge of a newly created triangle using the quarter edge structure. */
    static FQuarterEdge *MakeTriangle(FVector PointA, FVector PointB, FVector PointC);

    /** Removes the provided edge from the triangulation. */
    static void Sever(FQuarterEdge* FEdgeInfo);

    /** Flips the diagonal edge of a quadrangle. */
    static void Flip(FQuarterEdge* FEdgeInfo);

    /** Returns true if the point does not lie inside the circumcircle of the triangle vertices. */
    static bool IsLocallyDelaunay(FQuarterEdge *InTriangle, FVector Point);

    /** Returns nullptr if the point is inside the triangle, else it returns the edge of the triangle that the point is not to the left of. */
    static FQuarterEdge *IsPointInTriangle(FVector3f Point, FQuarterEdge *InTriangle);

protected:

    /** Returns true if the triangle shares a vertice with the boundary triangle. */
    static bool ContainsBoundaryPoint(FQuarterEdge* BoundaryTriangle, FQuarterEdge* Triangle);

    /** Returns true if the point of the current edge is connected through an edge to the boundary triangle.  */
    static bool IsPointConnectedToBoundary(FQuarterEdge* BoundaryTriangle, FQuarterEdge* InCurrentEdge);

    /** Returns true if the edge is one of the edges of the boundary triangle. */
    static bool IsEdgeABoundaryEdge(FQuarterEdge* BoundaryTriangle, FQuarterEdge* InEdge);
    
    /** Flips the edge of CurrentTriangle, and returns true if the edge flip should be reverted. */
    static bool FlipEdgeOrFlipBack(FQuarterEdge* BoundaryTriangle, TArray<FQuarterEdge*> TriangulationArray, TArray<FTriangle> TriangleVertexArray, FQuarterEdge* CurrentEdge, FQuarterEdge* CurrentTriangle);
    
    /** Returns true if Triangle is inside another triangle of the triangulation. */
    static bool IsTriangleInsideOut(TArray<FTriangle> TriangleArray, FQuarterEdge* Triangle);

    /** Returns true if the both triangles are intersecting each other. */
    static bool DoTrianglesIntersect(FQuarterEdge *TriangleA, FTriangle TriangleB);

    /** Returns true if the triangle contains the edge. */
    static bool ContainsEdgeInTriangle(FQuarterEdge* InTriangle, FQuarterEdge* InEdge);

    /** Returns true if the point exists somewhere along the edge. */
    static bool IsPointOnEdge(FVector Point, FQuarterEdge *FEdgeInfo);

/*
*   3D DELAUNAY TRIANGULATION
*/
    // Return the tetrahedron that shares the same triangle (face)
    static FTetrahedron* Neighbour(TArray<FTetrahedron*>& TetrahedraArray, TSet<FVector> InTetrahedron, TSet<FVector> SharedTriangleVertices);

    /*
    Returns the following:
    A positive value if P is inside the circumsphere of ABCD. 
    A negative value if P is outside the circumsphere of ABCD.
    0 if P lies on the circumsphere of ABCD.
    */
    static int InSphere(FVector A, FVector B, FVector C, FVector D, FVector P);

    /** Returns the determinant of the matrix. */
    static float GetDeterminant(double Matrix[10][10], int MatrixSize);

    /// <summary>
    /// Delaunay Tetrahedralization using the Bowyer-Watson algorithm. Includes the boundary tetrahedron.
    /// </summary>
    /// <param name="TetrahedraArray"> List of tetrahedra containing only the boundary tetrahedron, returns the results of the Delaunay Tetrahedralization. </param>
    /// <param name="PointArray"> List of points to use in the Delaunay Triangulation. </param>
    static void BowyerWatson3D(TArray<FTetrahedron*>& TetrahedraArray, TArray<FVector> PointArray);

    /** Returns true if the tetrahedron is in the array. */
    static bool IsTetrahedronAlreadyInArray(TArray<FTetrahedron*> TetrahedraArray, FTetrahedron* InTetrahedron);

    /** Returns true if the tetrahedron exists in the array and has been removed successfully. */
    static bool RemoveTetrahedronFromArray(TArray<FTetrahedron*>& TetrahedraArray, FTetrahedron* InTetrahedron);

    /** Returns true if any of the tetrahedra in the tetrahedralization are intersecting. */
    static bool AreAnyTetrahedraInArrayIntersecting(TArray<FTetrahedron*> TetrahedraArray);

    /** Returns true if the both tetrahedra intersect each other. */
    static bool DoTetrahedraIntersect(FTetrahedron* TetrahedronA, FTetrahedron* TetrahedronB);

    /** Returns a list of all the faces (triangles) that make up the tetrahedron. */
    static TArray<FTriangle> GetTetrahedronFaces(FTetrahedron* Tetrahedron);

    /** Returns one of the vertices of the tetrahedron, determined by the index provided. */
    static FVector GetVertexByIndex(FTetrahedron* Tetrahedron, int32 Index);

    /** Returns true if both triangles intersect in 3D space. */
    static bool TriTriIntersect3D(FTriangle TriangleA, FTriangle TriangleB);

    /** Returns true if any tetrahedra in the array are coplanar. */
    static bool AreAnyTetrahedraInArrayCoplanar(TArray<FTetrahedron*> TetrahedraArray);

};