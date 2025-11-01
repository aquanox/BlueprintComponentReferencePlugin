// Copyright 2024, Aquanox.

#pragma once

#include "CoreMinimal.h"
#include "BlueprintComponentReference.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Info.h"
#include "Engine/EngineTypes.h"
#include "Engine/DataAsset.h"
#include "GameplayTagsManager.h"
#include "GameFramework/Character.h"
#include "BCRTestStruct.h"
#include "BCRTestActorComponent.h"

#if WITH_CACHED_COMPONENT_REFERENCE_TESTS
#include "CachedBlueprintComponentReference.h"
#endif

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

	UFUNCTION()
	static bool TestComponent(const UActorComponent* InComponent);
public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Components)
	USceneComponent* Default_Root = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Components)
	USceneComponent* Default_LevelOne = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Components)
	USceneComponent* Default_LevelTwo = nullptr;

	// no uproperty, never set, just to have pretty GET_MEMBER_NAME_CHECKED
	UActorComponent* NonExistingComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Components)
	UActorComponent* Default_LevelZero = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Components)
	USceneComponent* Construct_LevelOneNP = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Components)
	USceneComponent* Construct_LevelOne = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Components)
	UActorComponent* Construct_LevelZero = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Components)
	USceneComponent* Playtime_LevelOneNP = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Components)
	USceneComponent* Playtime_LevelOne;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Components)
	UActorComponent* Playtime_LevelZero = nullptr;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Test")
	UPrimaryDataAsset* TargetDA = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="Test|Engine")
	FComponentReference ComponentReference;

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

	// Simple component reference with member filter func. 
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="Test|Filter", meta=(ComponentFilter="TestComponent"))
	FBlueprintComponentReference ReferenceFilterUser;

	// Simple component reference with external user filter func. 
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="Test|Filter", meta=(ComponentFilter="BlueprintComponentReferenceTests.BCRTestActor.TestComponent"))
	FBlueprintComponentReference ReferenceFilterUserExternal;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Test|Containers", meta=(ShowBlueprint=false, AllowedClasses="/Script/Engine.SceneComponent"))
	TArray<FBlueprintComponentReference> ReferenceArray;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Test|Containers", meta=(ShowBlueprint=false, AllowedClasses="/Script/Engine.SceneComponent"))
	TMap<FName, FBlueprintComponentReference> ReferenceMap;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Test|Containers", meta=(ShowBlueprint=false, AllowedClasses="/Script/Engine.SceneComponent"))
	TSet<FBlueprintComponentReference> ReferenceSet;

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
	UChildActorComponent* LevelNope = nullptr;

};

// test cases with cached access
UCLASS(MinimalAPI, Blueprintable)
class ABCRCachedTestActor : public ACharacter
{
	GENERATED_BODY()
public:
	static const FName MeshPropertyName;
	
	ABCRCachedTestActor();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Test|Cached", meta=(AllowedClasses="/Script/Engine.SceneComponent"))
	FBlueprintComponentReference ReferenceSingle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Test|Cached", meta=(AllowedClasses="/Script/Engine.SceneComponent"))
	TArray<FBlueprintComponentReference> ReferenceArray;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Test|Cached", meta=(AllowedClasses="/Script/Engine.SceneComponent"))
	TMap<FName, FBlueprintComponentReference> ReferenceMap;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Test|Cached", meta=(AllowedClasses="/Script/Engine.SceneComponent"))
	TMap<FBlueprintComponentReference, FBCRTestStrustData> ReferenceMapKey;

#if WITH_CACHED_COMPONENT_REFERENCE_TESTS
	
	TCachedComponentReferenceSingle<USceneComponent> CachedReferenceSingle { this, &ReferenceSingle };
	
	TCachedComponentReferenceArray<USceneComponent> CachedReferenceArray { this, &ReferenceArray };
	
	TCachedComponentReferenceMapValue<USceneComponent, FName> CachedReferenceMap { this, &ReferenceMap };
	
	TCachedComponentReferenceMapKey<USceneComponent, FBCRTestStrustData> CachedReferenceMapKey { this, &ReferenceMapKey };

#endif
	
};
