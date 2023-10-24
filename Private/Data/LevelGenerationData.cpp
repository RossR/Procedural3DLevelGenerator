// Fill out your copyright notice in the Description page of Project Settings.


#include "Data/LevelGenerationData.h"

void FMinimapInfo_Interactable::SetInteractableTransform(const FVector& Location, const FRotator& Rotation, const FVector& Scale3D, const float MinimapScale, AActor* MinimapActor)
{
	InteractableTransform = FTransform((MinimapActor->GetActorRotation() + Rotation), (MinimapActor->GetActorLocation() + (Location * MinimapScale)), Scale3D);
	/*InteractableTransform.SetLocation(MinimapActor->GetActorLocation() + (Location * MinimapScale));
	InteractableTransform.SetRotation(MinimapActor->GetActorQuat() + Rotation.Quaternion());
	InteractableTransform.SetScale3D(Scale3D);*/
}
