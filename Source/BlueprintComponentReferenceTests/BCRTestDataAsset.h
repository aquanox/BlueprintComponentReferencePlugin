// Copyright 2024, Aquanox.

#pragma once

#include "CoreMinimal.h"
#include "BlueprintComponentReference.h"
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
