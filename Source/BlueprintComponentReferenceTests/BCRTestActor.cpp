// Copyright 2024, Aquanox.


#include "BCRTestActor.h"
#include "BCRTestActorComponent.h"
#include "Components/ChildActorComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogBlueprintComponentRef, Log, All);

// Sets default values
ABCRTestActor::ABCRTestActor()
{
	Root = CreateDefaultSubobject<UBCRTestSceneComponent>("Root");
	SetRootComponent(Root);
#if WITH_EDITORONLY_DATA
	Root->bVisualizeComponent = true;
#endif

	LevelOne = CreateDefaultSubobject<UBCRTestSceneComponent>("LevelOneName");
	LevelOne->SetupAttachment(Root);

	LevelTwo = CreateDefaultSubobject<UBCRTestSceneComponent>("LevelTwoName");
	LevelTwo->SetupAttachment(LevelOne);

	LevelZero = CreateDefaultSubobject<UBCRTestActorComponent>("ZeroName");

}

void ABCRTestActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// construct components

	LevelOneConstruct = NewObject<UBCRTestSceneComponent>(this, "LevelOneConstructName");
	LevelOneConstruct->SetupAttachment(LevelOne);
	LevelOneConstruct->RegisterComponent();
	AddInstanceComponent(LevelOneConstruct);

	LevelZeroConstruct = NewObject<UBCRTestActorComponent>(this, "LevelZeroConstructName");
	LevelZeroConstruct->RegisterComponent();
	AddInstanceComponent(LevelZeroConstruct);

	// instanced components without uproperty (weak ptr just to debug)

	LevelOneConstructNP = NewObject<UBCRTestSceneComponent>(this, "LevelOneConstructNPName");
	LevelOneConstructNP->SetupAttachment(LevelOne);
	LevelOneConstructNP->RegisterComponent();
	AddInstanceComponent(LevelOneConstructNP.Get());

	LevelZeroConstructNP = NewObject<UBCRTestActorComponent>(this, "LevelZeroConstructNPName");
	LevelZeroConstructNP->RegisterComponent();
	AddInstanceComponent(LevelZeroConstructNP.Get());
}

void ABCRTestActor::BeginPlay()
{
	Super::BeginPlay();

	// instanced components with prop

	LevelOneInstanced= NewObject<UBCRTestSceneComponent>(this, "LevelOneInstancedName");
	LevelOneInstanced->SetupAttachment(LevelOne);
	LevelOneInstanced->RegisterComponent();
	AddInstanceComponent(LevelOneInstanced);

	LevelZeroInstanced = NewObject<UBCRTestActorComponent>(this, "LevelZeroInstancedName");
	LevelZeroInstanced->RegisterComponent();
	AddInstanceComponent(LevelZeroInstanced);

	// instanced components without prop

	LevelOneInstancedNP = NewObject<UBCRTestSceneComponent>(this, "LevelOneInstancedNPName");
	LevelOneInstancedNP->SetupAttachment(LevelOne);
	LevelOneInstancedNP->RegisterComponent();
	AddInstanceComponent(LevelOneInstancedNP.Get());

	LevelZeroInstancedNP = NewObject<UBCRTestActorComponent>(this, "LevelZeroInstancedNPName");
	LevelZeroInstancedNP->RegisterComponent();
	AddInstanceComponent(LevelZeroInstancedNP.Get());
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
	LevelNope->SetChildActorClass(ABCRTestActor::StaticClass());
}
