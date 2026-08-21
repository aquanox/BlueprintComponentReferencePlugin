#pragma once

#include "CoreMinimal.h"
#include "BlueprintComponentReference.h"

#include "BlueprintComponentReferenceExtras.generated.h"

#define WITH_BCR_EXTRAS 1

/**
 * This header contains miscellaneous types with BCR
 */
class USceneComponent;
class UPrimitiveComponent;
class UMeshComponent;

/**
 * A deriviate of FBlueprintComponentReference allowing only SceneComponents by default
 */
USTRUCT(meta=(AllowedClasses="/Script/Engine.SceneComponent"))
struct FSceneComponentReference : public FBlueprintComponentReference
{
	GENERATED_BODY()
};

/**
 * A deriviate of FBlueprintComponentReference allowing only PrimitiveComponent by default
 */
USTRUCT(meta=(AllowedClasses="/Script/Engine.PrimitiveComponent"))
struct FPrimitiveComponentReference : public FBlueprintComponentReference
{
	GENERATED_BODY()
};

/**
 * A deriviate of FBlueprintComponentReference allowing only MeshComponent by default
 */
USTRUCT(meta=(AllowedClasses="/Script/Engine.MeshComponent"))
struct FMeshComponentReference : public FBlueprintComponentReference
{
	GENERATED_BODY()
};

/**
 * A deriviate of FBlueprintComponentReference allowing only MeshComponent by default
 * and providing a socket selector
 */
USTRUCT(meta=(AllowedClasses="/Script/Engine.MeshComponent"))
struct FMeshSocketReference : public FBlueprintComponentReference
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category=Component, meta=(DisplayAfter="Value"))
	FName SocketName;
};
