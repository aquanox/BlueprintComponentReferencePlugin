// Copyright 2024, Aquanox.

#pragma once

#include "CoreMinimal.h"
#include "BlueprintComponentReference.h"
#include "CachedBlueprintComponentReference.h"
#include "Engine/DataAsset.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "BCRTestDataAsset.generated.h"

/**
 *
 */
UCLASS(MinimalAPI, Blueprintable)
class UBCRTestDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, meta=(ActorClass="/Script/BlueprintComponentReferenceTests.BCRTestActor"), Category=Test)
    FBlueprintComponentReference ExternalRef;
    	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Test", meta=(ActorClass="/Script/BlueprintComponentReferenceTests.BCRTestActor", AllowedClasses="/Script/Engine.SceneComponent"))
	FBlueprintComponentReference ReferenceSingle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Test", meta=(ActorClass="/Script/BlueprintComponentReferenceTests.BCRTestActor", AllowedClasses="/Script/Engine.SceneComponent"))
	TArray<FBlueprintComponentReference> ReferenceArray;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Test", meta=(ActorClass="/Script/BlueprintComponentReferenceTests.BCRTestActor", AllowedClasses="/Script/Engine.SceneComponent"))
	TSet<FBlueprintComponentReference> ReferenceSet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Test", meta=(ActorClass="/Script/BlueprintComponentReferenceTests.BCRTestActor", AllowedClasses="/Script/Engine.SceneComponent"))
	TMap<FName, FBlueprintComponentReference> ReferenceMap;

	TCachedComponentReference<USceneComponent> CachedReferenceSingle { this, &ReferenceSingle };

	TCachedComponentReferenceArray<USceneComponent> CachedReferenceArray { this, &ReferenceArray };

	TCachedComponentReferenceMap<USceneComponent, decltype(ReferenceMap)::KeyType> CachedReferenceMap { this, &ReferenceMap };

	UBCRTestDataAsset();
	
	UFUNCTION(CallInEditor)
	void Foo();
};
