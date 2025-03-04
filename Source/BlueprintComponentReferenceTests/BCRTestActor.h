// Copyright 2024, Aquanox.

#pragma once

#include "CoreMinimal.h"
#include "BlueprintComponentReference.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Info.h"
#include "Engine/EngineTypes.h"
#include "Engine/DataAsset.h"

#include "GameFramework/Character.h"

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

UCLASS(MinimalAPI, Blueprintable, HideCategories=("ActorTick", "Cooking", "ComponentReplication", Physics, Activation, Rendering, Transform, Tags, "ComponentTick", "Collision", "Advanced", "Replication"))
class ABCRTestActor : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ABCRTestActor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UFUNCTION(CallInEditor)
	void DumpComponents();
public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Components)
	TObjectPtr<USceneComponent> TestBase;

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

	// Simple component reference. Defaults only
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="Test|Base")
	FBlueprintComponentReference ReferenceA;
	
	// Simple component reference. Only SceneComp allowed
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="Test|Filter", meta=(AllowedClasses="/Script/Engine.SceneComponent"))
	FBlueprintComponentReference ReferenceFilterA;

	// Simple component reference.  SceneComp disallowed
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="Test|Filter", meta=(DisallowedClasses="/Script/Engine.SceneComponent"))
	FBlueprintComponentReference ReferenceFilterB;

	// Hide clear button
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Test|Metadata", NoClear, meta=(NoClear))
	FBlueprintComponentReference ReferenceNoClear;

	// Hide navigate button
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Test|Metadata", meta=(NoNavigate))
	FBlueprintComponentReference ReferenceNoNavigate;

	// Hide navigate picker button and allow manual editing of members
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Test|Metadata", meta=(NoPicker))
	FBlueprintComponentReference ReferenceNoPicker;

	// Display only natively created components
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Test|Metadata", meta=(ShowNative=true, ShowBlueprint=false, ShowInstanced=false, ShowHidden=false))
	FBlueprintComponentReference ReferenceNativeOnly;
	// Display only blueprint created components
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Test|Metadata", meta=(ShowNative=false, ShowBlueprint=true, ShowInstanced=false, ShowHidden=false))
	FBlueprintComponentReference ReferenceBlueprintOnly;
	// Display only instanced created components. Instance only
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Test|Metadata", meta=(ShowNative=false, ShowBlueprint=false, ShowInstanced=true, ShowHidden=false))
	FBlueprintComponentReference ReferenceInstancedOnly;
	// Display only hidden components. Instance only
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Test|Metadata", meta=(ShowNative=false, ShowBlueprint=false, ShowInstanced=false, ShowHidden=true))
	FBlueprintComponentReference ReferencePathOnly;
	
	// Display only natively created components. No editor
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Test|Metadata", meta=(ShowNative=true, ShowBlueprint=false, ShowInstanced=false, ShowHidden=false, ShowEditor=false))
	FBlueprintComponentReference ReferenceNativeOnlyNoEditor;

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
