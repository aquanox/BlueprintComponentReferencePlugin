// Copyright 2024, Aquanox.


#include "BCRTestActor.h"
#include "BCRTestActorComponent.h"
#include "Components/ChildActorComponent.h"
#include "GameFramework/DefaultPawn.h"

DEFINE_LOG_CATEGORY_STATIC(LogBlueprintComponentRef, Log, All);

// Sets default values
ABCRTestActor::ABCRTestActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass(ACharacter::CharacterMovementComponentName, UBCRTestMovementComponent::StaticClass()))
{
	Default_Root = CreateDefaultSubobject<UBCRTestSceneComponent>("Default_Root");
	Default_Root->SetupAttachment(GetRootComponent());

	Default_LevelOne = CreateDefaultSubobject<UBCRTestSceneComponent>("Default_LevelOne");
	Default_LevelOne->SetupAttachment(Default_Root);

	Default_LevelTwo = CreateDefaultSubobject<UBCRTestSceneComponent>("Default_LevelTwo");
	Default_LevelTwo->SetupAttachment(Default_LevelOne);

	Default_LevelZero = CreateDefaultSubobject<UBCRTestActorComponent>("Default_LevelZero");

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
