// Copyright 2024, Aquanox.

#pragma once

#include "CoreMinimal.h"
#include "BlueprintComponentReference.h"
#include "GameFramework/Actor.h"
#include "Engine/EngineTypes.h"
#include "BCRTestActor.generated.h"

UCLASS(MinimalAPI, Blueprintable, HideCategories=("ActorTick", "Cooking", Activation, Rendering, Transform, Tags,"ComponentTick", "Collision", "Advanced", "Replication"))
class ABCRTestActor : public AInfo
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ABCRTestActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UFUNCTION(CallInEditor)
	void DumpComponents();
public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Components)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Components)
	TObjectPtr<USceneComponent> LevelOne;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Components)
	TObjectPtr<USceneComponent> LevelOneConstruct;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Components)
	TObjectPtr<USceneComponent> LevelOneInstanced;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Components)
	TObjectPtr<USceneComponent> LevelTwo;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Components)
	TObjectPtr<UActorComponent> LevelZero;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Components)
	TObjectPtr<UActorComponent> LevelZeroConstruct;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Components)
	TObjectPtr<UActorComponent> LevelZeroInstanced;


protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=TESTACTOR)
	UPrimaryDataAsset* TargetDA;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=TESTACTOR)
	FBaseComponentReference BaseComponentReference;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=TESTACTOR)
	FComponentReference ComponentReference;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=TESTACTOR)
	FSoftComponentReference SoftComponentReference;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category=TESTACTOR)
	FBlueprintComponentReference ReferenceA;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=TESTACTOR, NoClear, meta=(NoClear))
	FBlueprintComponentReference ReferenceNoClear;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=TESTACTOR, meta=(NoNavigate))
	FBlueprintComponentReference ReferenceNoNavigate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=TESTACTOR, meta=(NoPicker))
	FBlueprintComponentReference ReferenceNoPicker;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=TESTACTOR, meta=(ShowNative=true, ShowBlueprint=false, ShowInstanced=false, ShowPathOnly=false))
	FBlueprintComponentReference ReferenceNativeOnly;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=TESTACTOR, meta=(ShowNative=false, ShowBlueprint=true, ShowInstanced=false, ShowPathOnly=false))
	FBlueprintComponentReference ReferenceBlueprintOnly;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=TESTACTOR, meta=(ShowNative=false, ShowBlueprint=false, ShowInstanced=true, ShowPathOnly=false))
	FBlueprintComponentReference ReferenceInstancedOnly;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=TESTACTOR, meta=(ShowNative=false, ShowBlueprint=false, ShowInstanced=false, ShowPathOnly=true))
	FBlueprintComponentReference ReferencePathOnly;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=TESTACTOR, meta=(ShowBlueprint=false))
	TArray<FBlueprintComponentReference> ReferenceArray;

};

UCLASS(MinimalAPI, Blueprintable)
class ABCRTestActorWithChild : public ABCRTestActor
{
	GENERATED_BODY()
public:
	ABCRTestActorWithChild();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Components)
	TObjectPtr<UChildActorComponent> LevelNope;
};