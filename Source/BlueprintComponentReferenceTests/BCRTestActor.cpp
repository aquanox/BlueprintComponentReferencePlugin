// Copyright 2024, Aquanox.


#include "BCRTestActor.h"
#include "BCRTestActorComponent.h"
#include "Components/ChildActorComponent.h"
#include "GameFramework/DefaultPawn.h"

DEFINE_LOG_CATEGORY_STATIC(LogBlueprintComponentRef, Log, All);

const FName ABCRCachedTestActor::MeshPropertyName = TEXT("Mesh");

// Sets default values
ABCRTestActor::ABCRTestActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass(ACharacter::CharacterMovementComponentName, UBCRTestMovementComponent::StaticClass()))
{
	bIsEditorOnlyActor = true;
	
	Default_Root = CreateDefaultSubobject<UBCRTestSceneComponent>("Default_Root");
	Default_Root->SetupAttachment(GetRootComponent());

	Default_LevelOne = CreateDefaultSubobject<UBCRTestSceneComponent>("Default_LevelOne");
	Default_LevelOne->SetupAttachment(Default_Root);

	Default_LevelTwo = CreateDefaultSubobject<UBCRTestSceneComponent>("Default_LevelTwo");
	Default_LevelTwo->SetupAttachment(Default_LevelOne);

	Default_LevelZero = CreateDefaultSubobject<UBCRTestActorComponent>("Default_LevelZero");

	ReferenceVar = FBlueprintComponentReference::ForProperty(GET_MEMBER_NAME_CHECKED(ThisClass, Default_Root));
	ReferencePath = FBlueprintComponentReference::ForPath(FName("Default_LevelZero"));
	ReferenceBadVar = FBlueprintComponentReference::ForProperty(GET_MEMBER_NAME_CHECKED(ThisClass, NonExistingComponent));
	ReferenceBadPath = FBlueprintComponentReference::ForPath(FName("Non_Existent_Path"));
	ReferenceBadValue = FBlueprintComponentReference::ForProperty(GET_MEMBER_NAME_CHECKED(ThisClass, Default_LevelTwo));

	ReferenceArray.Empty();
	ReferenceArray.Add(FBlueprintComponentReference::ForProperty(GET_MEMBER_NAME_CHECKED(ThisClass, Default_LevelOne)));
	ReferenceArray.Add(FBlueprintComponentReference::ForProperty(GET_MEMBER_NAME_CHECKED(ThisClass, Default_LevelTwo)));
	ReferenceArray.Add(FBlueprintComponentReference::ForProperty(GET_MEMBER_NAME_CHECKED(ThisClass, NonExistingComponent)));
	ReferenceArray.Add(FBlueprintComponentReference::ForProperty(GET_MEMBER_NAME_CHECKED(ThisClass, Default_LevelZero)));
	
	ReferenceSet.Empty();
	ReferenceSet.Add(FBlueprintComponentReference::ForProperty(GET_MEMBER_NAME_CHECKED(ThisClass, Default_LevelOne)));
	ReferenceSet.Add(FBlueprintComponentReference::ForProperty(GET_MEMBER_NAME_CHECKED(ThisClass, Default_LevelTwo)));
	ReferenceSet.Add(FBlueprintComponentReference::ForProperty(GET_MEMBER_NAME_CHECKED(ThisClass, NonExistingComponent)));
	ReferenceSet.Add(FBlueprintComponentReference::ForProperty(GET_MEMBER_NAME_CHECKED(ThisClass, Default_LevelZero)));
	
	ReferenceMap.Empty();
	ReferenceMap.Add("one", FBlueprintComponentReference::ForProperty(GET_MEMBER_NAME_CHECKED(ThisClass, Default_LevelOne)));
	ReferenceMap.Add("two", FBlueprintComponentReference::ForProperty(GET_MEMBER_NAME_CHECKED(ThisClass, Default_LevelTwo)));
	ReferenceMap.Add("bad", FBlueprintComponentReference::ForProperty(GET_MEMBER_NAME_CHECKED(ThisClass, NonExistingComponent)));
	ReferenceMap.Add("zero", FBlueprintComponentReference::ForProperty(GET_MEMBER_NAME_CHECKED(ThisClass, Default_LevelZero)));
}

void ABCRTestActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	Construct_LevelOneNP= NewObject<UBCRTestSceneComponent>(this, "Construct_LevelOne_SomeName");
	Construct_LevelOneNP->SetupAttachment(Default_LevelOne);
	Construct_LevelOneNP->RegisterComponent();
	// no addinstancedcomp

	Construct_LevelOne = NewObject<UBCRTestSceneComponent>(this, "Construct_LevelOne");
	Construct_LevelOne->SetupAttachment(Default_LevelOne);
	Construct_LevelOne->RegisterComponent();
	AddInstanceComponent(Construct_LevelOne);

	Construct_LevelZero = NewObject<UBCRTestActorComponent>(this, "Construct_LevelZero");
	Construct_LevelZero->RegisterComponent();
	AddInstanceComponent(Construct_LevelZero);

}

void ABCRTestActor::BeginPlay()
{
	Super::BeginPlay();

	Playtime_LevelOneNP= NewObject<UBCRTestSceneComponent>(this, "Playtime_LevelOne_SomeName");
	Playtime_LevelOneNP->SetupAttachment(Default_LevelOne);
	Playtime_LevelOneNP->RegisterComponent();
	// no addinstancedcomp

	Playtime_LevelOne= NewObject<UBCRTestSceneComponent>(this, "Playtime_LevelOne");
	Playtime_LevelOne->SetupAttachment(Default_LevelOne);
	Playtime_LevelOne->RegisterComponent();
	AddInstanceComponent(Playtime_LevelOne);

	Playtime_LevelZero = NewObject<UBCRTestActorComponent>(this, "Playtime_LevelZero");
	Playtime_LevelZero->RegisterComponent();
	AddInstanceComponent(Playtime_LevelZero);

}

void ABCRTestActor::DumpComponents()
{
	for (UActorComponent* Component : GetComponents())
	{
		auto Class = Component->GetClass()->GetName();
		auto Name = Component->GetName();
		auto Path = Component->GetPathName(this);

		UE_LOG(LogBlueprintComponentRef, Log, TEXT("Found component: %p Class=%s Name=%s Path=%s"), this, *Class, *Name, *Path);
	}
}

ABCRTestActorWithChild::ABCRTestActorWithChild()
{
	LevelNope = CreateDefaultSubobject<UChildActorComponent>("LevelNope");
	LevelNope->SetChildActorClass(ADefaultPawn::StaticClass());
}

ABCRCachedTestActor::ABCRCachedTestActor()
{
	ReferenceSingle = FBlueprintComponentReference::ForPath(ACharacter::MeshComponentName);
	
	ReferenceArray.Add(FBlueprintComponentReference::ForPath(ACharacter::MeshComponentName));
	
	ReferenceMap.Add("property", FBlueprintComponentReference::ForProperty("Mesh"));
	ReferenceMap.Add("path", FBlueprintComponentReference::ForPath(ACharacter::MeshComponentName));
}

void ABCRCachedTestActor::TryCompileTemplates()
{
	check(!"This is just to ensure template code compiles during autobuild");
	
	AActor* PtrToActor = nullptr;
	USceneComponent* PtrToComponent = nullptr;
	FName None("SuperKey");

	CachedReferenceSingle.Get();
	CachedReferenceSingle.Get(PtrToActor);
	CachedReferenceSingle.InvalidateCache();
	
	CachedReferenceSingleRaw.Get();
	CachedReferenceSingleRaw.Get(PtrToActor);

	CachedReferenceArray.Get(0);
	CachedReferenceArray.Get(PtrToActor, 0);
	CachedReferenceArray.Num();
	CachedReferenceArray.IsEmpty();
	CachedReferenceArray.InvalidateCache();
	
	CachedReferenceArrayRaw.Get(0);
	CachedReferenceArrayRaw.Get(PtrToActor, 0);

	CachedReferenceMap.Get(None);
	CachedReferenceMap.Get(PtrToActor, None);
	CachedReferenceMap.Num();
	CachedReferenceMap.IsEmpty();
	CachedReferenceMap.InvalidateCache();
	
	CachedReferenceMapRaw.Get("root");
	CachedReferenceMapRaw.Get(PtrToActor, "root");
	
	CachedReferenceMapKey.Get(PtrToComponent);
	CachedReferenceMapKey.Get(PtrToActor, PtrToComponent);

	FReferenceCollector& Collector = *(FReferenceCollector*)nullptr;
	CachedReferenceSingleRaw.AddReferencedObjects(Collector, PtrToActor);
	CachedReferenceArrayRaw.AddReferencedObjects(Collector, PtrToActor);
	CachedReferenceMapRaw.AddReferencedObjects(Collector, PtrToActor);
	CachedReferenceMapKey.AddReferencedObjects(Collector, PtrToActor);
}
