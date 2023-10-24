// Fill out your copyright notice in the Description page of Project Settings.


#include "Data/FunctionLibraries/DelaunayTriangulationLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GeomTools.h"

#include "Data/FunctionLibraries/GuidueDevillersLibrary.h"

TArray<FQuarterEdge> UDelaunayTriangulationLibrary::GuibasStolfi(FIntVector GridSize, TArray<FIntVector> PointArray, bool bRemoveBoundaryTriangle)
{
    if (GridSize.IsZero()) { return TArray<FQuarterEdge>(); }

    TArray<FQuarterEdge*> TriangulationArray;
    TArray<FVector> AddedPointsArray;

    // Start with a single "infinitely large" triangle
    FVector BoundaryVertexLeft = { (-0.5f * (float)GridSize.X), ((float)-GridSize.Y), (0.f) };
    FVector BoundaryVertexRight = { (-0.5f * (float)GridSize.X), (2.f * (float)GridSize.Y), (0.f) };
    FVector BoundaryVertexTop = { (2.5f * (float)GridSize.X), (0.5f * (float)GridSize.Y), (0.f) };

    FQuarterEdge *BoundaryTriangle = MakeTriangle(BoundaryVertexLeft, BoundaryVertexRight, BoundaryVertexTop);
    
    TriangulationArray.Add(BoundaryTriangle);

    // Loop through your points, and add them into the triangulation one-by-one
    for (FIntVector ForPoint : PointArray)
    {
        FVector CurrentPoint = (FVector)ForPoint;
        AddedPointsArray.Add(CurrentPoint);

        // Start search from wherever the previous point was inserted
        FQuarterEdge* TriangleToCheckAB = TriangulationArray.Last();

        // Find the triangle containing this point
        FQuarterEdge* TriangleContainingPoint = nullptr;

        do
        {
            FQuarterEdge* TriangleToCheckBC = LNext(TriangleToCheckAB);
            FQuarterEdge* TriangleToCheckCA = LNext(TriangleToCheckBC);

            FQuarterEdge* ClockwiseEdge = IsPointInTriangle((FVector3f)CurrentPoint, TriangleToCheckAB);

            if (ClockwiseEdge == nullptr)
            {
                TriangleContainingPoint = TriangleToCheckAB;
                break;
            }
            else
            {
                TriangleToCheckAB = Previous(ClockwiseEdge);
            }
        } while (true);

        TArray<FQuarterEdge*> NewTrianglesArray;

        FQuarterEdge* TriangleContainingPointBA = SymmetricEdge(TriangleContainingPoint);

        FQuarterEdge* TriangleContainingPointBC = LNext(TriangleContainingPoint);
        FQuarterEdge* TriangleContainingPointCB = SymmetricEdge(TriangleContainingPointBC);

        FQuarterEdge* TriangleContainingPointCA = LNext(TriangleContainingPointBC);
        FQuarterEdge* TriangleContainingPointAC = SymmetricEdge(TriangleContainingPointCA);

        // if the new point intersects an existing edge, split that edge
        FQuarterEdge *IntersectedEdge = nullptr;

        if (IsPointOnEdge(CurrentPoint, TriangleContainingPoint))
        {
            IntersectedEdge = TriangleContainingPoint;
        }
        else if (IsPointOnEdge(CurrentPoint, TriangleContainingPointBC))
        {
            IntersectedEdge = TriangleContainingPointBC;
        }
        else if (IsPointOnEdge(CurrentPoint, TriangleContainingPointCA))
        {
            IntersectedEdge = TriangleContainingPointCA;
        }
        
        FQuarterEdge* AP = nullptr;
        FQuarterEdge* BP = nullptr;
        FQuarterEdge* CP = nullptr;
        FQuarterEdge* DP = nullptr;

        if (IntersectedEdge)
        {
            // BC, CA
            FQuarterEdge* BC = RNext(IntersectedEdge);
            FQuarterEdge* CA = RNext(BC);

            // BD, DA
            FQuarterEdge* BD = LNext(IntersectedEdge);
            FQuarterEdge* DA = LNext(BD);

            // Completely remove the intersected edge
            Sever(IntersectedEdge);
            TriangulationArray.Remove(IntersectedEdge);
            TriangulationArray.Remove(SymmetricEdge(IntersectedEdge));

            AP = MakeQuadEdge(Destination(CA), CurrentPoint);
            CP = MakeQuadEdge(Destination(BC), CurrentPoint);
            BP = MakeQuadEdge(BD->Data, CurrentPoint);
            DP = MakeQuadEdge(DA->Data, CurrentPoint);

            Splice(AP, LNext(DA));
            Splice(SymmetricEdge(AP), SymmetricEdge(DP));

            Splice(CP, LNext(SymmetricEdge(CA)));
            Splice(SymmetricEdge(CP), SymmetricEdge(AP));

            Splice(BP, LNext(SymmetricEdge(BC)));
            Splice(SymmetricEdge(BP), SymmetricEdge(CP));

            Splice(DP, LNext(BD));
            Splice(SymmetricEdge(DP), SymmetricEdge(CP));

            Splice(SymmetricEdge(CP), SymmetricEdge(DP));
        }
        else
        {
            // Split the triangle that contains it into three new triangles and them check the locally delaunay condition

            AP = MakeQuadEdge(Destination(TriangleContainingPointCA), CurrentPoint);
            BP = MakeQuadEdge(Destination(TriangleContainingPoint), CurrentPoint);
            CP = MakeQuadEdge(Destination(TriangleContainingPointBC), CurrentPoint);

            FQuarterEdge* PA = SymmetricEdge(AP);
            FQuarterEdge* PB = SymmetricEdge(BP);
            FQuarterEdge* PC = SymmetricEdge(CP);

            Splice(AP, LNext(TriangleContainingPointCA));
            Splice(SymmetricEdge(AP), SymmetricEdge(CP));

            Splice(BP, LNext(TriangleContainingPoint));
            Splice(SymmetricEdge(BP), SymmetricEdge(AP));

            Splice(CP, LNext(TriangleContainingPointBC));
            Splice(SymmetricEdge(CP), SymmetricEdge(BP));

            Splice(SymmetricEdge(BP), SymmetricEdge(CP));
        }

        TriangulationArray.Add(SymmetricEdge(AP));
        TriangulationArray.Add(SymmetricEdge(BP));
        TriangulationArray.Add(SymmetricEdge(CP));
        if (DP) { TriangulationArray.Add(SymmetricEdge(DP)); }

        //PA = nullptr;

        //for (FQuarterEdge CurrentTriangle : NewTrianglesArray)
        FQuarterEdge* StartingEdge = SymmetricEdge(AP);
        FQuarterEdge* CurrentEdge = SymmetricEdge(AP);
        FQuarterEdge* PreviousEdge = SymmetricEdge(AP);

        int GoForwardCount = 0;


        // You know that you started with a valid Delaunay triangulation, so you only have to look at the edges immediately surrounding the point that you added
        // the rest of the edges are already good
        do
        {
            if (CurrentEdge == StartingEdge && PreviousEdge == CurrentEdge->Next && GoForwardCount > 1) { break; }

            FQuarterEdge* CurrentTriangle = RNext(CurrentEdge);

            TArray<FTriangle> TriangleVertexArray;

            for (FQuarterEdge* CurrentEdgeInArray : TriangulationArray)
            {
                if (CurrentEdgeInArray == BoundaryTriangle) { continue; }
                else if (ContainsEdgeInTriangle(CurrentEdgeInArray, CurrentTriangle)) { continue; }

                FTriangle TriangleVertices;
                TriangleVertices.A = CurrentEdgeInArray->Data;
                TriangleVertices.B = RNext(CurrentEdgeInArray)->Data;
                TriangleVertices.C = RNext(RNext(CurrentEdgeInArray))->Data;

                // Don't add to the vertex array if it is already in it
                bool bAlreadyInVertexArray = false;

                for (FTriangle CurrentVertices : TriangleVertexArray)
                {
                    TArray<FVector> CurrentVertexArray
                    {
                        CurrentVertices.A,
                        CurrentVertices.B,
                        CurrentVertices.C
                    };

                    if (CurrentVertexArray.Contains(TriangleVertices.A) &&
                        CurrentVertexArray.Contains(TriangleVertices.B) &&
                        CurrentVertexArray.Contains(TriangleVertices.C))
                    {
                        bAlreadyInVertexArray = true;
                        break;
                    }
                }

                if (!bAlreadyInVertexArray)
                {
                    TriangleVertexArray.Add(TriangleVertices);
                }
            }

            // If a point on our boundary triangle is involved
            if (ContainsBoundaryPoint(BoundaryTriangle, CurrentTriangle))
            {
                // 1. If the edge in question is a boundary edge, do not flip it
                if (!IsEdgeABoundaryEdge(BoundaryTriangle, CurrentTriangle))
                {
                    bool bFlipBack = FlipEdgeOrFlipBack(BoundaryTriangle, TriangulationArray, TriangleVertexArray, CurrentEdge, CurrentTriangle);

                    if (bFlipBack)
                    {
                        // Flip back
                        Flip(CurrentTriangle);
                        Flip(CurrentTriangle);
                        Flip(CurrentTriangle);
                    }
                }

                // Go forward if CurrentTriangle hasn't changed
                if (CurrentTriangle == RNext(CurrentEdge))
                {
                    // Go Forward (clockwise)
                    PreviousEdge = CurrentEdge;
                    CurrentEdge = Previous(CurrentEdge);
                    GoForwardCount++;
                    continue;
                }

                bool bNewCurrentTriangleContainsBoundaryPoint = false;

                do
                {
                    CurrentTriangle = RNext(CurrentEdge);

                    bNewCurrentTriangleContainsBoundaryPoint = ContainsBoundaryPoint(BoundaryTriangle, CurrentTriangle);

                    if (bNewCurrentTriangleContainsBoundaryPoint)
                    {
                        if (!IsEdgeABoundaryEdge(BoundaryTriangle, CurrentTriangle))
                        {
                            TriangleVertexArray.Empty();

                            for (FQuarterEdge* CurrentEdgeInArray : TriangulationArray)
                            {
                                if (CurrentEdgeInArray == BoundaryTriangle) { continue; }
                                else if (ContainsEdgeInTriangle(CurrentEdgeInArray, CurrentTriangle)) { continue; }

                                FTriangle TriangleVertices;
                                TriangleVertices.A = CurrentEdgeInArray->Data;
                                TriangleVertices.B = RNext(CurrentEdgeInArray)->Data;
                                TriangleVertices.C = RNext(RNext(CurrentEdgeInArray))->Data;

                                // Don't add to the vertex array if it is already in it
                                bool bAlreadyInVertexArray = false;

                                for (FTriangle CurrentVertices : TriangleVertexArray)
                                {
                                    TArray<FVector> CurrentVertexArray
                                    {
                                        CurrentVertices.A,
                                        CurrentVertices.B,
                                        CurrentVertices.C
                                    };

                                    if (CurrentVertexArray.Contains(TriangleVertices.A) &&
                                        CurrentVertexArray.Contains(TriangleVertices.B) &&
                                        CurrentVertexArray.Contains(TriangleVertices.C))
                                    {
                                        bAlreadyInVertexArray = true;
                                        break;
                                    }
                                }

                                if (!bAlreadyInVertexArray)
                                {
                                    TriangleVertexArray.Add(TriangleVertices);
                                }
                            }

                            //Flip and check flip back
                            bool bFlipBack = FlipEdgeOrFlipBack(BoundaryTriangle, TriangulationArray, TriangleVertexArray, CurrentEdge, CurrentTriangle);

                            if (bFlipBack)
                            {
                                // Flip back
                                Flip(CurrentTriangle);
                                Flip(CurrentTriangle);
                                Flip(CurrentTriangle);
                                break;
                            }
                        }
                        else { break; }
                    }
                    else { break; }
                } while (true);
                
                if (bNewCurrentTriangleContainsBoundaryPoint)
                {
                    // Go Forward (clockwise)
                    PreviousEdge = CurrentEdge;
                    CurrentEdge = Previous(CurrentEdge);
                    GoForwardCount++;
                    continue;
                }
            }

            bool bIsLocallyDelaunay = true;

            for (FVector CurrentAddedPoint : AddedPointsArray)
            {
                if (!IsLocallyDelaunay(CurrentTriangle, CurrentAddedPoint))
                {
                    bIsLocallyDelaunay = false;
                    break;
                }
            }

            if (bIsLocallyDelaunay) 
            { 
                // Go Forward (clockwise)
                PreviousEdge = CurrentEdge;
                CurrentEdge = Previous(CurrentEdge);
                GoForwardCount++;
                continue;
            }

            // Flip any edges that are not locally Delaunay
            Flip(CurrentTriangle);

            // Go Backward (anti-clockwise)
            PreviousEdge = CurrentEdge;
            CurrentEdge = CurrentEdge->Next;
            GoForwardCount--;
            continue;

            // Then once you’re done checking and potentially flipping all of the “dirty” edges, you’re done! You have a valid Delaunay triangulation once again, 
            // and you’re ready for the next point.
        } while (true);
    }
    
    // Once you run out of points, just remove the infinitely large outer triangle – and the edges connected to it – and you have your final triangulation.

    TArray<FQuarterEdge>OutTriangulationArray;

    if (bRemoveBoundaryTriangle)
    {
        TriangulationArray.RemoveAt(0);

        TArray<FQuarterEdge*>EvaluatedEdgeArray;

        for (FQuarterEdge* CurrentTriangle : TriangulationArray)//int i = 0; i < TriangulationArray.Num(); i++)
        {
            TArray<FQuarterEdge*> TriangeEdgeArray
            {
                CurrentTriangle,
                RNext(CurrentTriangle),
                RNext(RNext(CurrentTriangle))
            };

            for (FQuarterEdge* CurrentEdge : TriangeEdgeArray)
            {
                if (ContainsBoundaryPoint(BoundaryTriangle, CurrentEdge))
                {
                    Sever(CurrentTriangle);
                    EvaluatedEdgeArray.Add(CurrentEdge);
                }
                else if (!EvaluatedEdgeArray.Contains(CurrentEdge) && !EvaluatedEdgeArray.Contains(SymmetricEdge(CurrentEdge)))
                {
                    OutTriangulationArray.Add(*CurrentEdge);
                    EvaluatedEdgeArray.Add(CurrentEdge);
                }
            }
        }
    }
    else
    {
        TArray<FQuarterEdge*>EvaluatedEdgeArray;

        for (FQuarterEdge* CurrentTriangle : TriangulationArray)
        {
            if (!EvaluatedEdgeArray.Contains(CurrentTriangle) && !EvaluatedEdgeArray.Contains(SymmetricEdge(CurrentTriangle)))
            {
                OutTriangulationArray.Add(*CurrentTriangle);
                EvaluatedEdgeArray.Add(CurrentTriangle);
            }
        }
    }

    return OutTriangulationArray;
}

TArray<FEdgeInfo> UDelaunayTriangulationLibrary::DelaunayTetrahedralization(FIntVector GridSize, TArray<FIntVector> PointArray, bool bRemoveBoundaryTetrahedron)
{
    if (GridSize.IsZero()) { return TArray<FEdgeInfo>(); }

    /*if (PointArray.Num() > 3.f && FMath::PointsAreCoplanar((TArray<FVector>)PointArray))
    {
        TArray<FQuarterEdge> TriangulationArray = GuibasStolfi(GridSize, PointArray, bRemoveBoundaryTetrahedron);

        TSet<FEdgeInfo> EdgeSet;
        for (FQuarterEdge CurrentEdge : TriangulationArray)
        {
            EdgeSet.Add(FEdgeInfo((FIntVector)CurrentEdge.Data, (FIntVector)Destination(&CurrentEdge)));
        }

        return EdgeSet.Array();
    }*/

    // Make TetrahedronFaces Tetrahedron
    TArray<FTetrahedron*> TetrahedraArray;

    const float GridWidth = GridSize.X > GridSize.Y ? GridSize.X : GridSize.Y;

    FTetrahedron* BoundaryTetrahedron = new FTetrahedron
    {
        FVector{ (GridWidth * .5f),     (GridWidth * .5f),      (GridSize.Z * 4.1f) },
        FVector{ (GridWidth * -1.25f),  (GridWidth * -2.1f),    (GridSize.Z * -1.1f) },
        FVector{ (GridWidth * 4.f),     (GridWidth * .5f),      (GridSize.Z * -1.1f) },
        FVector{ (GridWidth * -1.25f),  (GridWidth * 3.1f),     (GridSize.Z * -1.1f) }
    };
    

    //FTetrahedron* BoundaryTetrahedron = GetSuperTetrahedron((TArray<FVector>)PointArray);
    TetrahedraArray.Add(BoundaryTetrahedron);

    BowyerWatson3D(TetrahedraArray, (TArray<FVector>)PointArray);

    TSet<FEdgeInfo>EdgeSet;
    TArray<FTetrahedron*> TetrahedraToEdgeArray;

    // Remove boundary Tetrahedron and any tetrahedra that are incident to a point of the boundary CurrentTetrahedron 
    if (bRemoveBoundaryTetrahedron)
    {
        TArray<FTetrahedron*> EvaluatedTetrahedraArray;
        TArray<FTetrahedron*> DiscardedTetrahedraArray;

        TSet<FVector> BoundaryVertices{ BoundaryTetrahedron->A, BoundaryTetrahedron->B, BoundaryTetrahedron->C, BoundaryTetrahedron->D };

        for (FTetrahedron* CurrentTetrahedron : TetrahedraArray)
        {
            TSet<FVector> CurrentTetrahedronVertices{ CurrentTetrahedron->A, CurrentTetrahedron->B, CurrentTetrahedron->C, CurrentTetrahedron->D };

            TArray<FVector> SharedVerticesWithBoundaryTetra = BoundaryVertices.Intersect(CurrentTetrahedronVertices).Array();

            if (SharedVerticesWithBoundaryTetra.IsEmpty() && !IsTetrahedronAlreadyInArray(EvaluatedTetrahedraArray, CurrentTetrahedron))
            {
                EvaluatedTetrahedraArray.Add(CurrentTetrahedron);
            }
        }
        TetrahedraToEdgeArray = EvaluatedTetrahedraArray;
    }
    else
    {
        TetrahedraToEdgeArray = TetrahedraArray;
    }

    if (AreAnyTetrahedraInArrayIntersecting(TetrahedraToEdgeArray))
    {
        UE_LOG(LogTemp, Warning, TEXT("UDelaunayTriangulationLibrary::DelaunayTetrahedralization Potentially intersecting tetrehedrons detected!"));
    }

    if (AreAnyTetrahedraInArrayCoplanar(TetrahedraToEdgeArray))
    {
        UE_LOG(LogTemp, Warning, TEXT("UDelaunayTriangulationLibrary::DelaunayTetrahedralization Potentially coplanar tetrahedrons detected!"));
    }

    const TSet<FIntVector> BoundaryTetrahedronVertices { (FIntVector)BoundaryTetrahedron->A, (FIntVector)BoundaryTetrahedron->B, (FIntVector)BoundaryTetrahedron->C, (FIntVector)BoundaryTetrahedron->D};

    for (FTetrahedron* CurrentTetrahedron : TetrahedraArray)
    {
        EdgeSet.Add(FEdgeInfo((FIntVector)CurrentTetrahedron->A, (FIntVector)CurrentTetrahedron->B));
        EdgeSet.Add(FEdgeInfo((FIntVector)CurrentTetrahedron->B, (FIntVector)CurrentTetrahedron->C));
        EdgeSet.Add(FEdgeInfo((FIntVector)CurrentTetrahedron->C, (FIntVector)CurrentTetrahedron->A));

        EdgeSet.Add(FEdgeInfo((FIntVector)CurrentTetrahedron->A, (FIntVector)CurrentTetrahedron->D));
        EdgeSet.Add(FEdgeInfo((FIntVector)CurrentTetrahedron->B, (FIntVector)CurrentTetrahedron->D));
        EdgeSet.Add(FEdgeInfo((FIntVector)CurrentTetrahedron->C, (FIntVector)CurrentTetrahedron->D));
    }

    // Remove edges connected to the boundary vertices
    for (FEdgeInfo CurrentEdge : EdgeSet.Array())
    {
        if (BoundaryTetrahedronVertices.Contains(CurrentEdge.Origin) || BoundaryTetrahedronVertices.Contains(CurrentEdge.Destination))
        {
            EdgeSet.Remove(CurrentEdge);
        }
    }

    

    // Remove duplicate edges
    TArray<FEdgeInfo> EdgeArray = EdgeSet.Array();

    for (FEdgeInfo CurrentEdge : EdgeSet.Array())
    {
        if (!EdgeSet.Contains(CurrentEdge)) { continue; }

        for (FEdgeInfo ComparisonEdge : EdgeArray)
        {
            if (CurrentEdge.Origin == ComparisonEdge.Origin && CurrentEdge.Destination == ComparisonEdge.Destination) { continue; }

            if (ComparisonEdge.Origin == CurrentEdge.Destination && ComparisonEdge.Destination == CurrentEdge.Origin)
            {
                EdgeSet.Remove(ComparisonEdge);
            }
        }
        EdgeArray = EdgeSet.Array();
    }

    return EdgeArray;
}

FQuarterEdge *UDelaunayTriangulationLibrary::MakeQuadEdge(FVector Start, FVector End)
{
    FQuarterEdge *StartEnd = new FQuarterEdge();
    FQuarterEdge *LeftRight = new FQuarterEdge();
    FQuarterEdge *EndStart = new FQuarterEdge();
    FQuarterEdge *RightLeft = new FQuarterEdge();

    StartEnd->Data = Start;
    EndStart->Data = End;

    StartEnd->Rot = LeftRight;
    LeftRight->Rot = EndStart;
    EndStart->Rot = RightLeft;
    RightLeft->Rot = StartEnd;

    // normal edges are on different vertices,
    // and initially they are the only edges out
    // of each vertex
    StartEnd->Next = StartEnd;
    EndStart->Next = EndStart;

    // but dual edges share the same face,
    // so they point to one another
    LeftRight->Next = RightLeft;
    RightLeft->Next = LeftRight;

    return StartEnd;
}

void UDelaunayTriangulationLibrary::Splice(FQuarterEdge *EdgeA, FQuarterEdge *EdgeB)
{
    SwapNexts(EdgeA->Next->Rot, EdgeB->Next->Rot);
    SwapNexts(EdgeA, EdgeB);
}

void UDelaunayTriangulationLibrary::SwapNexts(FQuarterEdge *EdgeA, FQuarterEdge *EdgeB)
{
    FQuarterEdge *FirstEdgeNext = EdgeA->Next;
    EdgeA->Next = EdgeB->Next;
    EdgeB->Next = FirstEdgeNext;
}

FQuarterEdge *UDelaunayTriangulationLibrary::MakeTriangle(FVector PointA, FVector PointB, FVector PointC)
{
    FQuarterEdge *ab = MakeQuadEdge(PointA, PointB);
    FQuarterEdge *bc = MakeQuadEdge(PointB, PointC);
    FQuarterEdge *ca = MakeQuadEdge(PointC, PointA);

    Splice(SymmetricEdge(ab), bc);
    Splice(SymmetricEdge(bc), ca);
    Splice(SymmetricEdge(ca), ab);

    return ab;
}

void UDelaunayTriangulationLibrary::Sever(FQuarterEdge* FEdgeInfo)
{
    Splice(FEdgeInfo, Previous(FEdgeInfo));
    Splice(SymmetricEdge(FEdgeInfo), Previous(SymmetricEdge(FEdgeInfo)));
}

void UDelaunayTriangulationLibrary::Flip(FQuarterEdge* FEdgeInfo)
{
    FQuarterEdge *EdgeA = Previous(FEdgeInfo);
    FQuarterEdge *EdgeB = Previous(SymmetricEdge(FEdgeInfo));
    
    Splice(FEdgeInfo, EdgeA);
    Splice(SymmetricEdge(FEdgeInfo), EdgeB);
    Splice(FEdgeInfo, LNext(EdgeA));
    Splice(SymmetricEdge(FEdgeInfo), LNext(EdgeB));

    FEdgeInfo->Data= Destination(EdgeA);
    SymmetricEdge(FEdgeInfo)->Data = Destination(EdgeB);
}

bool UDelaunayTriangulationLibrary::IsLocallyDelaunay(FQuarterEdge *InTriangle, FVector Point)
{
    FQuarterEdge* AB = SymmetricEdge(InTriangle);
    FQuarterEdge* BC = LNext(AB);
    FQuarterEdge* CA = LNext(BC);

    const FVector3f PointA = (FVector3f)BC->Data;
    const FVector3f PointB = (FVector3f)AB->Data;
    const FVector3f PointC = (FVector3f)CA->Data;

    FMatrix InMatrix(   FPlane(PointA.X, PointB.X, PointC.X, Point.X),
                        FPlane(PointA.Y, PointB.Y, PointC.Y, Point.Y),
                        FPlane(((FMath::Square(PointA.X)) + (FMath::Square(PointA.Y))), ((FMath::Square(PointB.X)) + (FMath::Square(PointB.Y))), ((FMath::Square(PointC.X)) + (FMath::Square(PointC.Y))), ((FMath::Square(Point.X)) + (FMath::Square(Point.Y)))),
                        FPlane(1, 1, 1, 1));

    const float Determinant = InMatrix.Determinant();

    return Determinant <= 0.f;
}

FQuarterEdge *UDelaunayTriangulationLibrary::IsPointInTriangle(FVector3f Point, FQuarterEdge *InTriangle)
{
    const FVector3f PointA = (FVector3f)InTriangle->Data;
    const FVector3f PointB = (FVector3f)LNext(InTriangle)->Data;
    const FVector3f PointC = (FVector3f)LNext(LNext(InTriangle))->Data;

    // Cross product indicates which 'side' of the vector the point is on
    // If its on the same side as the remaining vert for all edges, then its inside.	
    if (FGeomTools::VectorsOnSameSide(PointB - PointA, Point - PointA, PointC - PointA, 0.f))
    {
        if (FGeomTools::VectorsOnSameSide(PointC - PointB, Point - PointB, PointA - PointB, 0.f))
        {
            if (FGeomTools::VectorsOnSameSide(PointA - PointC, Point - PointC, PointB - PointC, 0.f))
            {
                return nullptr;
            }
            else
            {
                return LNext(LNext(InTriangle));
            }
        }
        else
        {
            return LNext(InTriangle);
        }
    }
    else
    {
        return InTriangle;
    }

    return InTriangle;
}

bool UDelaunayTriangulationLibrary::ContainsBoundaryPoint(FQuarterEdge *BoundaryTriangle, FQuarterEdge *Triangle)
{
    if (Triangle->Data == BoundaryTriangle->Data ||
        Triangle->Data == SymmetricEdge(BoundaryTriangle)->Next->Data ||
        Triangle->Data == SymmetricEdge(SymmetricEdge(BoundaryTriangle)->Next)->Next->Data ||
        Destination(Triangle) == BoundaryTriangle->Data ||
        Destination(Triangle) == SymmetricEdge(BoundaryTriangle)->Next->Data ||
        Destination(Triangle) == SymmetricEdge(SymmetricEdge(BoundaryTriangle)->Next)->Next->Data)
    {
        return true;
    }

    return false;
}

bool UDelaunayTriangulationLibrary::IsPointConnectedToBoundary(FQuarterEdge* BoundaryTriangle, FQuarterEdge* InCurrentEdge)
{
    TArray<FVector> BoundaryPoints
    {
        BoundaryTriangle->Data,
        RNext(BoundaryTriangle)->Data,
        RNext(RNext(BoundaryTriangle))->Data
    };

    FQuarterEdge* StartingEdge = InCurrentEdge;
    FQuarterEdge* CurrentEdge = InCurrentEdge;
    FQuarterEdge* PreviousEdge = InCurrentEdge;

    do
    {
        if (CurrentEdge == StartingEdge && PreviousEdge == CurrentEdge->Next) { break; }

        if (BoundaryPoints.Contains(Destination(CurrentEdge)))
        {
            return true;
        }

        // Go to next point (clockwise)
        PreviousEdge = CurrentEdge;
        CurrentEdge = Previous(CurrentEdge);

    } while (true);

    return false;
}

bool UDelaunayTriangulationLibrary::IsEdgeABoundaryEdge(FQuarterEdge *BoundaryTriangle, FQuarterEdge *InEdge)
{
    FQuarterEdge *BC = RNext(BoundaryTriangle);
    FQuarterEdge *CA = RNext(RNext(BoundaryTriangle));

    if ((InEdge->Data == BoundaryTriangle->Data && Destination(InEdge)== Destination(BoundaryTriangle)) ||
        (InEdge->Data == BC->Data && Destination(InEdge) == Destination(BC)) ||
        (InEdge->Data == CA->Data && Destination(InEdge) == Destination(CA)) ||
        (InEdge->Data == SymmetricEdge(BoundaryTriangle)->Data && Destination(InEdge) == Destination(SymmetricEdge(BoundaryTriangle))) ||
        (InEdge->Data == SymmetricEdge(BC)->Data && Destination(InEdge) == Destination(SymmetricEdge(BC))) ||
        (InEdge->Data == SymmetricEdge(CA)->Data && Destination(InEdge) == Destination(SymmetricEdge(CA))) )
    {
        return true;
    }
    return false;
}

bool UDelaunayTriangulationLibrary::FlipEdgeOrFlipBack(FQuarterEdge* BoundaryTriangle, TArray<FQuarterEdge*> TriangulationArray, TArray<FTriangle> TriangleVertexArray, FQuarterEdge* CurrentEdge, FQuarterEdge* CurrentTriangle)
{
    FQuarterEdge* TestBStartingEdge = SymmetricEdge(CurrentEdge)->Next;
    FQuarterEdge* TestBCurrentEdge = TestBStartingEdge;
    int EdgeCounterB = 1;

    // 2. If the edge contains a boundary vertex, flip it, unless doing so would create an inside-out triangle.
    Flip(CurrentTriangle);

    FQuarterEdge* TestAStartingEdge = CurrentEdge;
    FQuarterEdge* TestACurrentEdge = CurrentEdge;
    int EdgeCounterA = 1;

    // Count edges surrounding the flipped edge's Data point
    do
    {
        TestACurrentEdge = TestACurrentEdge->Next;

        if (TestACurrentEdge != TestAStartingEdge)
        {
            EdgeCounterA++;
        }
        else
        {
            // Flip back if point only has two connected edges (and is not boundary points)
            if (EdgeCounterA <= 2.f) { return true; }
            break;
        }
    } while (true);

    // Count edges surround the previous destination of the flipped edge
    do
    {
        TestBCurrentEdge = TestBCurrentEdge->Next;

        if (TestBCurrentEdge != TestBStartingEdge)
        {
            EdgeCounterB++;
        }
        else
        {
            // Flip back if point only has two connected edges (and is not boundary points)
            if (EdgeCounterB <= 2.f) { return true; }
            break;
        }
    } while (true);

    // Flip back if edge completely overlaps an already existing edge
    for (FQuarterEdge* CurrentEdgeInArray : TriangulationArray)
    {
        if (CurrentEdgeInArray == CurrentTriangle) { continue; }
        else if (SymmetricEdge(CurrentEdgeInArray) == CurrentTriangle) { continue; }
        else if ((CurrentEdgeInArray->Data == CurrentTriangle->Data && Destination(CurrentEdgeInArray) == Destination(CurrentTriangle)) ||
            (CurrentEdgeInArray->Data == Destination(CurrentTriangle) && Destination(CurrentEdgeInArray) == CurrentTriangle->Data))
        {
            return true;
        }
    }

    if (IsTriangleInsideOut(TriangleVertexArray, CurrentTriangle))
    {
        return true;
    }

    // 3. If an edge separates our newly inserted point from a boundary vertex, do not flip it.
    if (!IsPointConnectedToBoundary(BoundaryTriangle, CurrentEdge))
    {
        return true;
    }

    return false;
}

bool UDelaunayTriangulationLibrary::IsTriangleInsideOut(TArray<FTriangle> TriangleArray, FQuarterEdge* InTriangle)
{
    for (FTriangle CurrentTriangle : TriangleArray)
    {
        if (DoTrianglesIntersect(InTriangle, CurrentTriangle))
        {
            return true;
        }
    }

    return false;
}

bool UDelaunayTriangulationLibrary::DoTrianglesIntersect(FQuarterEdge* TriangleA, FTriangle TriangleB)
{
    const TArray<FVector> TriangleAPoints{ 
        TriangleA->Data, 
        RNext(TriangleA)->Data, 
        RNext(RNext(TriangleA))->Data };

    const TArray<FVector> TriangleBPoints{
    TriangleB.A,
    TriangleB.B,
    TriangleB.C };

    // If both triangles have the same 3 vertices then it is the same triangle
    if (TriangleBPoints.Contains(TriangleAPoints[0]) && TriangleBPoints.Contains(TriangleAPoints[1]) && TriangleBPoints.Contains(TriangleAPoints[2]))
    {
        return false;
    }

     FMatrix IntersectionMatrixA(  
        FPlane(TriangleBPoints[0].X, TriangleBPoints[1].X, TriangleBPoints[2].X, TriangleAPoints[0].X),
        FPlane(TriangleBPoints[0].Y, TriangleBPoints[1].Y, TriangleBPoints[2].Y, TriangleAPoints[0].Y),
        FPlane(TriangleBPoints[0].Z, TriangleBPoints[1].Z, TriangleBPoints[2].Z, TriangleAPoints[0].Z),
        FPlane(1, 1, 1, 1));

    FMatrix IntersectionMatrixB(
        FPlane(TriangleBPoints[0].X, TriangleBPoints[1].X, TriangleBPoints[2].X, TriangleAPoints[1].X),
        FPlane(TriangleBPoints[0].Y, TriangleBPoints[1].Y, TriangleBPoints[2].Y, TriangleAPoints[1].Y),
        FPlane(TriangleBPoints[0].Z, TriangleBPoints[1].Z, TriangleBPoints[2].Z, TriangleAPoints[1].Z),
        FPlane(1, 1, 1, 1));

    FMatrix IntersectionMatrixC(
        FPlane(TriangleBPoints[0].X, TriangleBPoints[1].X, TriangleBPoints[2].X, TriangleAPoints[2].X),
        FPlane(TriangleBPoints[0].Y, TriangleBPoints[1].Y, TriangleBPoints[2].Y, TriangleAPoints[2].Y),
        FPlane(TriangleBPoints[0].Z, TriangleBPoints[1].Z, TriangleBPoints[2].Z, TriangleAPoints[2].Z),
        FPlane(1, 1, 1, 1));

    const float DeterminantA = IntersectionMatrixA.Determinant();
    const float DeterminantB = IntersectionMatrixB.Determinant();
    const float DeterminantC = IntersectionMatrixC.Determinant();

    int TestResult = 0;

    // If the signs of these determinants are the same and nonzero, there can be no intersection, and the test ends.
    if (DeterminantA == DeterminantB && DeterminantB == DeterminantC && DeterminantA != 0)
    {
        return false;
    }

    // If all are zero, the triangles are coplanar, and a separate test is performed to handle this case. 
    if (DeterminantA == DeterminantB && DeterminantB == DeterminantC && DeterminantA == 0)
    {
        TestResult = UGuidueDevillersLibrary::tri_tri_overlap_test_2d(FVector2D(TriangleAPoints[0].X, TriangleAPoints[0].Y), FVector2D(TriangleAPoints[1].X, TriangleAPoints[1].Y), FVector2D(TriangleAPoints[2].X, TriangleAPoints[2].Y), FVector2D(TriangleBPoints[0].X, TriangleBPoints[0].Y), FVector2D(TriangleBPoints[1].X, TriangleBPoints[1].Y), FVector2D(TriangleBPoints[2].X, TriangleBPoints[2].Y));
    } 

    // If both triangles share two points, and the remaining points are both clockwise/anti-clockwise of AB then they are intersecting
    
    if (TestResult > 0.f)
    {
        bool bShareTwoPoints = false;

        FVector SharedPointA;
        FVector SharedPointB;

        FVector DifferentPointA;
        FVector DifferentPointB;

        // A & B
        if (TriangleBPoints.Contains(TriangleAPoints[0]) && TriangleBPoints.Contains(TriangleAPoints[1]) && !TriangleBPoints.Contains(TriangleAPoints[2]))
        {
            bShareTwoPoints = true;

            SharedPointA = TriangleAPoints[0];
            SharedPointB = TriangleAPoints[1];

            DifferentPointA = TriangleAPoints[2];

            for (FVector DifferentPoint : TriangleBPoints)
            {
                if (DifferentPoint != TriangleAPoints[0] && DifferentPoint != TriangleAPoints[1])
                {
                    DifferentPointB = DifferentPoint;
                    break;
                }
            }
        }

        // B & C
        if (!TriangleBPoints.Contains(TriangleAPoints[0]) && TriangleBPoints.Contains(TriangleAPoints[1]) && TriangleBPoints.Contains(TriangleAPoints[2]))
        {
            bShareTwoPoints = true;

            SharedPointA = TriangleAPoints[1];
            SharedPointB = TriangleAPoints[2];

            DifferentPointA = TriangleAPoints[0];

            for (FVector DifferentPoint : TriangleBPoints)
            {
                if (DifferentPoint != TriangleAPoints[1] && DifferentPoint != TriangleAPoints[2])
                {
                    DifferentPointB = DifferentPoint;
                    break;
                }
            }
        }

        // C & A
        if (TriangleBPoints.Contains(TriangleAPoints[0]) && !TriangleBPoints.Contains(TriangleAPoints[1]) && TriangleBPoints.Contains(TriangleAPoints[2]))
        {
            bShareTwoPoints = true;

            SharedPointA = TriangleAPoints[0];
            SharedPointB = TriangleAPoints[2];

            DifferentPointA = TriangleAPoints[1];

            for (FVector DifferentPoint : TriangleBPoints)
            {
                if (DifferentPoint != TriangleAPoints[0] && DifferentPoint != TriangleAPoints[2])
                {
                    DifferentPointB = DifferentPoint;
                    break;
                }
            }
        }

        if (bShareTwoPoints)
        {
            if (FGeomTools::VectorsOnSameSide((FVector3f)(SharedPointB - SharedPointA), (FVector3f)(DifferentPointA - SharedPointA), (FVector3f)(DifferentPointB - SharedPointA), 0.f))
            {
                return true;
            }

            return false;
        }

        bool bShareOnePoint = false;

        TArray<FVector> TriangleAEdge;
        TArray<FVector> TriangleBEdge;
        
        // A
        if (TriangleBPoints.Contains(TriangleAPoints[0]) && !TriangleBPoints.Contains(TriangleAPoints[1]) && !TriangleBPoints.Contains(TriangleAPoints[2]))
        {
            bShareOnePoint = true;
            SharedPointA = TriangleAPoints[0];

            TriangleAEdge = 
            {
                TriangleAPoints[1],
                TriangleAPoints[2]
            };

            for (FVector DifferentPoint : TriangleBPoints)
            {
                if (DifferentPoint != TriangleAPoints[0])
                {
                    TriangleBEdge.Add(DifferentPoint);
                }
            }
        }
        // B
        if (!TriangleBPoints.Contains(TriangleAPoints[0]) && TriangleBPoints.Contains(TriangleAPoints[1]) && !TriangleBPoints.Contains(TriangleAPoints[2]))
        {
            bShareOnePoint = true;
            SharedPointA = TriangleAPoints[1];

            TriangleAEdge =
            {
                TriangleAPoints[0],
                TriangleAPoints[2]
            };

            for (FVector DifferentPoint : TriangleBPoints)
            {
                if (DifferentPoint != TriangleAPoints[1])
                {
                    TriangleBEdge.Add(DifferentPoint);
                }
            }
        }
        // C
        if (!TriangleBPoints.Contains(TriangleAPoints[0]) && !TriangleBPoints.Contains(TriangleAPoints[1]) && TriangleBPoints.Contains(TriangleAPoints[2]))
        {
            bShareOnePoint = true;
            SharedPointA = TriangleAPoints[2];

            TriangleAEdge =
            {
                TriangleAPoints[0],
                TriangleAPoints[1]
            };

            for (FVector DifferentPoint : TriangleBPoints)
            {
                if (DifferentPoint != TriangleAPoints[2])
                {
                    TriangleBEdge.Add(DifferentPoint);
                }
            }
        }

        if (bShareOnePoint)
        {
            FVector IntersectionPoint = FVector::ZeroVector;
            FVector TriangleNormal = FVector::ZeroVector;

            if (TriangleAEdge.Num() < 2.f)
            {
                UE_LOG(LogTemp, Error, TEXT("UDelaunayTriangulationLibrary::DoTrianglesIntersect TriangleAEdge does not have 2 entries."));
            }
            else if (FMath::SegmentIntersection2D(TriangleAEdge[0], TriangleAEdge[1], TriangleBPoints[0], TriangleBPoints[1], IntersectionPoint))
            {
                return true;
            }
            else if (FMath::SegmentIntersection2D(TriangleAEdge[0], TriangleAEdge[1], TriangleBPoints[1], TriangleBPoints[2], IntersectionPoint))
            {
                return true;
            }
            else if (FMath::SegmentIntersection2D(TriangleAEdge[0], TriangleAEdge[1], TriangleBPoints[2], TriangleBPoints[0], IntersectionPoint))
            {
                return true;
            }

            if (TriangleBEdge.Num() < 2.f)
            {
                UE_LOG(LogTemp, Error, TEXT("UDelaunayTriangulationLibrary::DoTrianglesIntersect TriangleBEdge does not have 2 entries."));
            }
            // Test TriangleA's isolated edge with TriangleB
            else if (FMath::SegmentIntersection2D(TriangleBEdge[0], TriangleBEdge[1], TriangleAPoints[0], TriangleAPoints[1], IntersectionPoint))
            {
                return true;
            }
            else if (FMath::SegmentIntersection2D(TriangleBEdge[0], TriangleBEdge[1], TriangleAPoints[1], TriangleAPoints[2], IntersectionPoint))
            {
                return true;
            }
            else if (FMath::SegmentIntersection2D(TriangleBEdge[0], TriangleBEdge[1], TriangleAPoints[2], TriangleAPoints[0], IntersectionPoint))
            {
                return true;
            }
            else { return false; }
        }
    }
    
    return TestResult > 0.f;
}

bool UDelaunayTriangulationLibrary::ContainsEdgeInTriangle(FQuarterEdge* InTriangle, FQuarterEdge* InEdge)
{
    FQuarterEdge* BC = RNext(InTriangle);
    FQuarterEdge* CA = RNext(RNext(InTriangle));

    TArray<FQuarterEdge*> TriangleEdges
    {
        InTriangle,
        SymmetricEdge(InTriangle),
        BC,
        SymmetricEdge(BC),
        CA,
        SymmetricEdge(CA),
    };

    if (TriangleEdges.Contains(InEdge))
    {
        return true;
    }

    return false;
}

bool UDelaunayTriangulationLibrary::IsPointOnEdge(FVector Point, FQuarterEdge* FEdgeInfo)
{
    // Share direction

    FVector EdgeDirection = Destination(FEdgeInfo) - FEdgeInfo->Data;
    EdgeDirection.Normalize();

    FVector PointDirection = Destination(FEdgeInfo) - Point;
    PointDirection.Normalize();

    if (EdgeDirection.Equals(PointDirection, 0.000000000000001f))
    {
        // point exists between A & B
        const float EdgeLength = (Destination(FEdgeInfo) - FEdgeInfo->Data).Length();
        const float PointLengthA = (Destination(FEdgeInfo) - Point).Length();
        const float PointLengthB = (Point - FEdgeInfo->Data).Length();

        if (FMath::IsNearlyEqual(EdgeLength, (PointLengthA + PointLengthB), .000001f))
        {
            return true;
        }
    }
   
    return false;
}

FTetrahedron* UDelaunayTriangulationLibrary::Neighbour(TArray<FTetrahedron*>& TetrahedraArray, TSet<FVector> InTetrahedron, TSet<FVector> SharedTriangleVertices)
{
    for (FTetrahedron* CurrentTetrahedron : TetrahedraArray)
    {
        TSet<FVector> CurrentTetraVertices{ CurrentTetrahedron->A, CurrentTetrahedron->B, CurrentTetrahedron->C, CurrentTetrahedron->D };

        TSet<FVector> DifferentVertices = SharedTriangleVertices.Difference(CurrentTetraVertices);

        if (InTetrahedron.Includes(CurrentTetraVertices)) { continue; }
        
        if (CurrentTetraVertices.Includes(SharedTriangleVertices))
        {
            return CurrentTetrahedron;
        }

        if (DifferentVertices.Num() == 0.f)
        {
            return CurrentTetrahedron;
        }
    }

    return nullptr;
}

int UDelaunayTriangulationLibrary::InSphere(FVector A, FVector B, FVector C, FVector D, FVector P)
{
    // Column then row
    double Matrix[10][10];

    TArray<FVector>MatrixVectors{ A, B, C, D, P };

    for (int i = 0; i < MatrixVectors.Num(); i++)
    {
        Matrix[i][0] = MatrixVectors[i].X;
        Matrix[i][1] = MatrixVectors[i].Y;
        Matrix[i][2] = MatrixVectors[i].Z;
        Matrix[i][3] = FMath::Square(MatrixVectors[i].X) + FMath::Square(MatrixVectors[i].Y) + FMath::Square(MatrixVectors[i].Z);
        Matrix[i][4] = 1.f;
    }

    const float Determinant = GetDeterminant(Matrix, 5);

    return Determinant;
}

float UDelaunayTriangulationLibrary::GetDeterminant(double Matrix[10][10], int MatrixSize)
{
    float Determinant = 0;
    double SubMatrix[10][10];
    if (MatrixSize == 2)
        return ((Matrix[0][0] * Matrix[1][1]) - (Matrix[1][0] * Matrix[0][1]));
    else {
        for (int x = 0; x < MatrixSize; x++) {
            int subi = 0;
            for (int i = 1; i < MatrixSize; i++) {
                int subj = 0;
                for (int j = 0; j < MatrixSize; j++) {
                    if (j == x)
                        continue;
                    SubMatrix[subi][subj] = Matrix[i][j];
                    subj++;
                }
                subi++;
            }
            Determinant = Determinant + (pow(-1, x) * Matrix[0][x] * GetDeterminant(SubMatrix, MatrixSize - 1));
        }
    }
    return Determinant;
}

void UDelaunayTriangulationLibrary::BowyerWatson3D(TArray<FTetrahedron*>& TetrahedraArray, TArray<FVector> PointArray)
{
    if (PointArray.IsEmpty()) { return; }

    const FTetrahedron* BoundaryTetrahedron = TetrahedraArray[0];
    const TSet<FVector> BoundaryTetrahedronVertices{ BoundaryTetrahedron->A, BoundaryTetrahedron->B, BoundaryTetrahedron->C, BoundaryTetrahedron->D };

    TArray<FVector>InsertedPointsArray;
    // Add the remaining points one by one and refine the tetrahedralization
    for (const FVector& Point : PointArray)
    {
        InsertedPointsArray.Add(Point);

        TArray<FTetrahedron*> CavityList;
        // Create an empty cavity list
        // for each tetrahedron in tetrahedron list :
        for (FTetrahedron* Tetrahedron : TetrahedraArray)
        {
            // if point is inside tetrahedron's circumsphere:
            if (InSphere(Tetrahedron->A, Tetrahedron->B, Tetrahedron->C, Tetrahedron->D, Point) > 0.f)
            {
                // Add tetrahedron to the cavity list
                CavityList.Add(Tetrahedron);
            }
        }

        // Create a new list for the boundary faces
        TArray<FTriangle> BoundaryFaces;
        
        // for each tetrahedron in cavity list :
        for (FTetrahedron* CavityTetrahedron : CavityList)
        {
            TSet<FVector> CavityTetrahedronVertices {CavityTetrahedron->A, CavityTetrahedron->B, CavityTetrahedron->C, CavityTetrahedron->D};

            TArray<FTriangle> TetrahedronFaces;
            TetrahedronFaces.Add(FTriangle{ CavityTetrahedron->B, CavityTetrahedron->A, CavityTetrahedron->C });
            TetrahedronFaces.Add(FTriangle{ CavityTetrahedron->C, CavityTetrahedron->A, CavityTetrahedron->D });
            TetrahedronFaces.Add(FTriangle{ CavityTetrahedron->D, CavityTetrahedron->A, CavityTetrahedron->B });
            TetrahedronFaces.Add(FTriangle{ CavityTetrahedron->D, CavityTetrahedron->B, CavityTetrahedron->C });

            // for each face in tetrahedron :
            for (FTriangle CurrentFace : TetrahedronFaces)
            {
                // if face is not shared by any other tetrahedron in cavity list :
                if (!Neighbour(CavityList, CavityTetrahedronVertices, TSet<FVector>{CurrentFace.A, CurrentFace.B, CurrentFace.C}))
                {
                    // Add face to the boundary faces list
                    BoundaryFaces.Add(CurrentFace);
                }
            }
        }

        //  Remove all tetrahedra in the cavity list from the tetrahedron list
        for (FTetrahedron* CavityTetrahedron : CavityList)
        {
            RemoveTetrahedronFromArray(TetrahedraArray, CavityTetrahedron);
        }

        // for each face in the boundary faces list :
        for (FTriangle CurrentFace : BoundaryFaces)
        {
            // Create a new tetrahedron using the face and the new point
            FTetrahedron* NewTetrahedron = new FTetrahedron{Point, CurrentFace.A, CurrentFace.B, CurrentFace.C};

            // Add the new tetrahedron to the tetrahedron list
            if (!IsTetrahedronAlreadyInArray(TetrahedraArray, NewTetrahedron))
            {
                TetrahedraArray.Add(NewTetrahedron);
            }
        }
    }
}

bool UDelaunayTriangulationLibrary::IsTetrahedronAlreadyInArray(TArray<FTetrahedron*> TetrahedraArray, FTetrahedron* InTetrahedron)
{
    TSet<FVector> TetrahedronVertices{ InTetrahedron->A, InTetrahedron->B, InTetrahedron->C, InTetrahedron->D };

    for (FTetrahedron* CurrentTetra : TetrahedraArray)
    {
        if (CurrentTetra == InTetrahedron) { continue; }

        TSet<FVector> CurrentTetraVertices{ CurrentTetra->A,CurrentTetra->B, CurrentTetra->C, CurrentTetra->D };

        if (TetrahedronVertices.Includes(CurrentTetraVertices))
        {
            return true;
        }
    }

    return false;
}

bool UDelaunayTriangulationLibrary::RemoveTetrahedronFromArray(TArray<FTetrahedron*>& TetrahedraArray, FTetrahedron* InTetrahedron)
{
    TSet<FVector> TetrahedronVertices{ InTetrahedron->A, InTetrahedron->B, InTetrahedron->C, InTetrahedron->D };

    TArray<FTetrahedron*> DeleteList;

    for (FTetrahedron* CurrentTetra : TetrahedraArray)
    {
        TSet<FVector> CurrentTetraVertices{ CurrentTetra->A,CurrentTetra->B, CurrentTetra->C, CurrentTetra->D };

        const TSet<FVector> SharedVertices = CurrentTetraVertices.Intersect(TetrahedronVertices);

        if (SharedVertices.Num() == 4.f)
        {
            DeleteList.Add(CurrentTetra);
        }
    }

    if (!DeleteList.IsEmpty())
    {
        for (FTetrahedron* TetraToDelete : DeleteList)
        {
            TetrahedraArray.Remove(TetraToDelete);
        }
        return true;
    }
    
    return false;
}

bool UDelaunayTriangulationLibrary::AreAnyTetrahedraInArrayIntersecting(TArray<FTetrahedron*> TetrahedraArray)
{
    int32 NumTetrahedra = TetrahedraArray.Num();

    bool bIsThereIntersection = false;

    // Check for pairwise intersection of tetrahedra
    for (int32 i = 0; i < NumTetrahedra - 1; ++i)
    {
        FTetrahedron* TetrahedronA = TetrahedraArray[i];

        for (int32 j = i + 1; j < NumTetrahedra; ++j)
        {
            FTetrahedron* TetrahedronB = TetrahedraArray[j];

            // Check if any edges of TetrahedronA intersect with TetrahedronB
            if (DoTetrahedraIntersect(TetrahedronA, TetrahedronB))
            {
                bIsThereIntersection = true; // Tetrahedra intersect
            }
        }
    }

    if (bIsThereIntersection) { return true; }

    return false; // No intersecting tetrahedra found
}

bool UDelaunayTriangulationLibrary::DoTetrahedraIntersect(FTetrahedron* TetrahedronA, FTetrahedron* TetrahedronB)
{
    TArray<FTriangle>TetrahedronAFaces = GetTetrahedronFaces(TetrahedronA);
    TArray<FTriangle>TetrahedronBFaces = GetTetrahedronFaces(TetrahedronA);

    for (FTriangle CurrentTriangleFromA : TetrahedronAFaces)
    {
        for (FTriangle CurrentTriangleFromB : TetrahedronBFaces)
        {
            if (TriTriIntersect3D(CurrentTriangleFromA, CurrentTriangleFromB))
            {
                return true;
            }
        }
    }

    return false;
}

TArray<FTriangle> UDelaunayTriangulationLibrary::GetTetrahedronFaces(FTetrahedron* Tetrahedron)
{
    TArray<FTriangle> Faces;

    // Define the vertex indices for each face
    const TArray<int32> FaceIndices = { 0, 1, 2, 0, 2, 3, 0, 3, 1, 1, 3, 2 };

    // Iterate over the face indices and create the corresponding faces
    for (int32 i = 0; i < FaceIndices.Num(); i += 3)
    {
        FTriangle Face;
        Face.A = GetVertexByIndex(Tetrahedron, FaceIndices[i]);
        Face.B = GetVertexByIndex(Tetrahedron, FaceIndices[i + 1]);
        Face.C = GetVertexByIndex(Tetrahedron, FaceIndices[i + 2]);

        Faces.Add(Face);
    }

    return Faces;
}

FVector UDelaunayTriangulationLibrary::GetVertexByIndex(FTetrahedron* Tetrahedron, int32 Index)
{
    switch (Index)
    {
    case 0: return Tetrahedron->A;
    case 1: return Tetrahedron->B;
    case 2: return Tetrahedron->C;
    case 3: return Tetrahedron->D;
    default: return FVector::ZeroVector;
    }
}

bool UDelaunayTriangulationLibrary::TriTriIntersect3D(FTriangle TriangleA, FTriangle TriangleB)
{
    const TSet<FVector> TriangleAVertices{ TriangleA.A, TriangleA.B, TriangleA.C };
    const TSet<FVector> TriangleBVertices{ TriangleB.A, TriangleB.B, TriangleB.C };

    // return false if they are the same triangle
    if (TriangleAVertices.Includes(TriangleBVertices)) { return false; }

    const TSet<FVector> SharedVertices = TriangleAVertices.Intersect(TriangleBVertices);
    
    // If no vertex are shared
    if (SharedVertices.IsEmpty())
    {
        if (UGuidueDevillersLibrary::tri_tri_overlap_test_3d(TriangleA.A, TriangleA.B, TriangleA.C, TriangleB.A, TriangleB.B, TriangleB.C))
        {
            return true;
        }
    }

    // If one vertex is shared
    if (SharedVertices.Num() == 1.f)
    {
        FVector IntersectPoint = FVector::ZeroVector;
        FVector TriangleNormal = FVector::ZeroVector;

        TArray<FVector> UniqueAVertices = TriangleAVertices.Difference(SharedVertices).Array();
        FVector SegmentStart = UniqueAVertices[0];
        FVector SegmentEnd = UniqueAVertices[1];

        if (FMath::SegmentTriangleIntersection(SegmentStart, SegmentEnd, TriangleB.A, TriangleB.B, TriangleB.C, IntersectPoint, TriangleNormal))
        {
            return true;
        }
        
        IntersectPoint = FVector::ZeroVector;
        TriangleNormal = FVector::ZeroVector;

        TArray<FVector> UniqueBVertices = TriangleBVertices.Difference(SharedVertices).Array();
        SegmentStart = UniqueBVertices[0];
        SegmentEnd = UniqueBVertices[1];
        
        if (FMath::SegmentTriangleIntersection(SegmentStart, SegmentEnd, TriangleB.A, TriangleB.B, TriangleB.C, IntersectPoint, TriangleNormal))
        {
            return true;
        }
    }

    // If the triangles share an edge (two vertices)
    if (SharedVertices.Num() == 2.f)
    {
        // Return true if the triangles are coplanar

        TArray<FVector> Points;
        Points.Append(SharedVertices.Array());
        Points.Append(TriangleAVertices.Difference(SharedVertices).Array());
        Points.Append(TriangleBVertices.Difference(SharedVertices).Array());
        if (FMath::PointsAreCoplanar(Points))
        {
            return true;
        }
    }
   
    return false;
}

bool UDelaunayTriangulationLibrary::AreAnyTetrahedraInArrayCoplanar(TArray<FTetrahedron*> TetrahedraArray)
{
    TArray<FTetrahedron*> CoplanarTetrahedronArray;
    for (FTetrahedron* CurrentTetrahedron: TetrahedraArray)
    {
        TArray<FVector> TetrahedronVertices{ CurrentTetrahedron->A, CurrentTetrahedron->B, CurrentTetrahedron->C, CurrentTetrahedron->D };

        if (FMath::PointsAreCoplanar(TetrahedronVertices))
        {
            CoplanarTetrahedronArray.Add(CurrentTetrahedron);
        }
    }

    if (!CoplanarTetrahedronArray.IsEmpty())
    {
        return true;
    }

    return false;
}