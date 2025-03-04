// Copyright 2024, Aquanox.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BCRTestActorComponent.generated.h"


UCLASS(MinimalAPI, meta=(BlueprintSpawnableComponent))
class UBCRTestActorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UBCRTestActorComponent();

};

UCLASS(MinimalAPI, meta=(BlueprintSpawnableComponent))
class UBCRTestSceneComponent : public USceneComponent
{
	GENERATED_BODY()

public:

};

UCLASS(MinimalAPI, meta=(BlueprintSpawnableComponent))
class UBCRTestMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:

};
