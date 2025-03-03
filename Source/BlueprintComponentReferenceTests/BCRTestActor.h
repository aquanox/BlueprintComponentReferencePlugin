// Copyright 2024, Aquanox.

#pragma once

#include "CoreMinimal.h"
#include "BlueprintComponentReference.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Info.h"
#include "Engine/EngineTypes.h"
#include "Engine/DataAsset.h"
#include "BCRTestActor.generated.h"

USTRUCT(BlueprintType)
struct FBCRTestStruct
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="Test")
	FBlueprintComponentReference Reference;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="Test")
	TArray<FBlueprintComponentReference> ReferenceArray;
};

USTRUCT()
struct FBCRTestReference : public FBlueprintComponentReference
{
	GENERATED_BODY()
	
};

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

	TWeakObjectPtr<USceneComponent> LevelOneConstructNP;
	TWeakObjectPtr<UActorComponent> LevelZeroConstructNP;

	TWeakObjectPtr<USceneComponent> LevelOneInstancedNP;
	TWeakObjectPtr<UActorComponent> LevelZeroInstancedNP;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Test")
	UPrimaryDataAsset* TargetDA;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Test|Engine")
	FBaseComponentReference BaseComponentReference;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="Test|Engine")
	FComponentReference ComponentReference;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="Test|Engine")
	FSoftComponentReference SoftComponentReference;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="Test|Base")
	FBlueprintComponentReference ReferenceA;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="Test|Base")
	FBCRTestReference ReferenceB;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Test|Metadata", NoClear, meta=(NoClear))
	FBlueprintComponentReference ReferenceNoClear;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Test|Metadata", meta=(NoNavigate))
	FBlueprintComponentReference ReferenceNoNavigate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Test|Metadata", meta=(NoPicker))
	FBlueprintComponentReference ReferenceNoPicker;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Test|Metadata", meta=(ShowNative=true, ShowBlueprint=false, ShowInstanced=false, ShowHidden=false))
	FBlueprintComponentReference ReferenceNativeOnly;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Test|Metadata", meta=(ShowNative=false, ShowBlueprint=true, ShowInstanced=false, ShowHidden=false))
	FBlueprintComponentReference ReferenceBlueprintOnly;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Test|Metadata", meta=(ShowNative=false, ShowBlueprint=false, ShowInstanced=true, ShowHidden=false))
	FBlueprintComponentReference ReferenceInstancedOnly;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Test|Metadata", meta=(ShowNative=false, ShowBlueprint=false, ShowInstanced=false, ShowHidden=true))
	FBlueprintComponentReference ReferencePathOnly;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Test|Containers", meta=(ShowBlueprint=false))
	TArray<FBlueprintComponentReference> ReferenceArray;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Test|Containers", meta=(ShowBlueprint=false))
	TMap<FName, FBlueprintComponentReference> ReferenceMap;
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Test|Containers", meta=(ShowBlueprint=false))
	//TSet<FBlueprintComponentReference> ReferenceSet;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Test|Struct")
	FBCRTestStruct StructTest;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Test|Struct")
	TArray<FBCRTestStruct> StructTestArray;
	
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