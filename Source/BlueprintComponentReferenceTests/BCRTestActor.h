// Copyright 2024, Aquanox.

#pragma once

#include "CoreMinimal.h"
#include "BlueprintComponentReference.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Info.h"
#include "Engine/EngineTypes.h"
#include "Engine/DataAsset.h"
#include "UObject/StrongObjectPtr.h"
#include "GameplayTagsManager.h"
#include "GameplayTagContainer.h"
#include "GameplayTagAssetInterface.h"
#include "CachedBlueprintComponentReference.h"
#include "GameFramework/Character.h"
#include "BCRTestStruct.h"
#include "BCRTestActorComponent.h"

#include "BCRTestActor.generated.h"

// basic examples
UCLASS(MinimalAPI, Blueprintable, HideCategories=("ActorTick", "Cooking", "ComponentReplication", Physics, Activation, Rendering, Transform, Tags, "ComponentTick", "Collision", "Advanced", "Replication"))
class ABCRTestActor : public ACharacter
{
	GENERATED_BODY()

public:
	ABCRTestActor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UFUNCTION(CallInEditor)
	void DumpComponents();
public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Components)
	TObjectPtr<USceneComponent> Default_Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Components)
	TObjectPtr<USceneComponent> Default_LevelOne;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Components)
	TObjectPtr<USceneComponent> Default_LevelTwo;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Components)
	TObjectPtr<UActorComponent> Default_LevelZero;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Components)
	TObjectPtr<USceneComponent> Construct_LevelOneNP;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Components)
	TObjectPtr<USceneComponent> Construct_LevelOne;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Components)
	TObjectPtr<UActorComponent> Construct_LevelZero;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Components)
	TObjectPtr<USceneComponent> Playtime_LevelOneNP;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Components)
	TObjectPtr<USceneComponent> Playtime_LevelOne;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Components)
	TObjectPtr<UActorComponent> Playtime_LevelZero;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Test")
	TObjectPtr<UPrimaryDataAsset> TargetDA;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Test|Engine")
	FBaseComponentReference BaseComponentReference;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="Test|Engine")
	FComponentReference ComponentReference;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="Test|Engine")
	FSoftComponentReference SoftComponentReference;

	// Simple component reference. Defaults only
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="Test|Base")
	FBlueprintComponentReference ReferenceVar;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="Test|Base")
	FBlueprintComponentReference ReferencePath;

	// Reference to nonexistent var 
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="Test|Base")
	FBlueprintComponentReference ReferenceBadVar;
	// Reference to nonexistent path
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="Test|Base")
	FBlueprintComponentReference ReferenceBadPath;
	// Reference to existing component that does not match filter conditions
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="Test|Base", meta=(AllowedClasses="/Script/Engine.MovementComponent"))
	FBlueprintComponentReference ReferenceBadValue;
	
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
	// Display components without properties assigned. Instance only
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
	// unsupported, can reference only component, not instanced things within spawned actor
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Components)
	TObjectPtr<UChildActorComponent> LevelNope;

};

// test cases with cached access
UCLASS(MinimalAPI)
class ABCRCachedTestActor : public ACharacter
{
	GENERATED_BODY()

public:
	ABCRCachedTestActor() = default;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Test|Metadata", meta=(ShowNative=false, ShowBlueprint=true, AllowedClasses="/Script/Engine.SceneComponent"))
	FBlueprintComponentReference ReferenceSingle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Test|Metadata", meta=(ShowNative=false, ShowBlueprint=true, AllowedClasses="/Script/Engine.SceneComponent"))
	TArray<FBlueprintComponentReference> ReferenceArray;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Test|Metadata", meta=(ShowNative=false, ShowBlueprint=true, AllowedClasses="/Script/Engine.SceneComponent"))
	TMap<FName, FBlueprintComponentReference> ReferenceMap;

	TCachedComponentReference<USceneComponent> CachedReferenceSingle { this, &ReferenceSingle };

	TCachedComponentReferenceArray<USceneComponent> CachedReferenceArray { this, &ReferenceArray };

	TCachedComponentReferenceMap<USceneComponent, decltype(ReferenceMap)::KeyType> CachedReferenceMap { this, &ReferenceMap };

	UFUNCTION(CallInEditor)
	void Foo();
};
