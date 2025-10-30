// Copyright 2024, Aquanox.

#pragma once

#include "CoreMinimal.h"
#include "BlueprintComponentReference.h"
#include "Engine/DataAsset.h"
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
	UBCRTestDataAsset();

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
};
