// Copyright 2024, Aquanox.

#pragma once

#include "CoreMinimal.h"
#include "BlueprintComponentReference.h"
#include "CachedBlueprintComponentReference.h"
#include "Engine/DataAsset.h"
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
	
};

UCLASS(MinimalAPI)
class UBCRCachedTestDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Test|Metadata", meta=(ShowNative=false, ShowBlueprint=true, AllowedClasses="/Script/Engine.SceneComponent"))
	FBlueprintComponentReference ReferenceSingle;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Test|Metadata", meta=(ShowNative=false, ShowBlueprint=true, AllowedClasses="/Script/Engine.SceneComponent"))
	TArray<FBlueprintComponentReference> ReferenceArray;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Test|Metadata", meta=(ShowNative=false, ShowBlueprint=true, AllowedClasses="/Script/Engine.SceneComponent"))
	TMap<FGameplayTag, FBlueprintComponentReference> ReferenceMap;
	
	TCachedComponentReference<USceneComponent, &ThisClass::ReferenceSingle> CachedReferenceSingle { this };
	
	TCachedComponentReferenceArray<USceneComponent, &ThisClass::ReferenceArray> CachedReferenceArray { this };
	
	TCachedComponentReferenceMap<USceneComponent, decltype(ReferenceMap)::KeyType, &ThisClass::ReferenceMap> CachedReferenceMap { this };

	UFUNCTION(CallInEditor)
	void Foo();
};
