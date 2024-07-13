// Copyright 2024, Aquanox.

#include "BCRTestActor.h"
#include "BlueprintComponentReference.h"
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, BlueprintComponentReferenceTests);

#if WITH_DEV_AUTOMATION_TESTS

struct FTestWorldScope
{
	FWorldContext WorldContext;
	UWorld* World = nullptr;
	UWorld* PrevWorld = nullptr;

	FTestWorldScope()
	{
		World = UWorld::CreateWorld(EWorldType::Game, false);

		WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(World);

		PrevWorld = &*GWorld;
		GWorld = World;
	}

	~FTestWorldScope()
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(true);
		GWorld = PrevWorld;
	}

	UWorld* operator->() const { return World; }
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlueprintComponentReferenceTests_Basic,
	"BlueprintComponentReference.Basic", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FBlueprintComponentReferenceTests_Basic::RunTest(FString const&)
{
	FTestWorldScope World;

	ABCRTestActor* TestActorS = World->SpawnActor<ABCRTestActor>();
	ABCRTestActor* TestActorD = GetMutableDefault<ABCRTestActor>();

	for(ABCRTestActor* TestActor : {  (ABCRTestActor*)nullptr, TestActorS, TestActorD })
	{
		UActorComponent* const RootComponent =
			TestActor ?  TestActor->GetRootComponent() : nullptr;
		UActorComponent* const LevelOneComponent =
			TestActor ?  TestActor->LevelOne  : nullptr;
		UActorComponent* const LevelZeroInstancedNPComponent =
			TestActor ? FindObject<UActorComponent>(TestActor, L"LevelZeroInstancedNPName") : nullptr;

		AddInfo(FString::Printf(TEXT("Using actor %s"), * GetNameSafe(TestActor)));

		FBlueprintComponentReference Reference;
		TestTrue("IsNull", Reference.IsNull());
		TestEqual("IsEqual", Reference.GetValueString(false), TEXT(""));
		TestEqual("IsEqual", Reference.GetValueString(true), TEXT(""));
		TestTrue("GetComponent2", Reference.GetComponent(TestActor) == nullptr);

		Reference.SetValueFromString(TEXT("LevelOne"));
		TestTrue("IsNull", !Reference.IsNull());
		TestEqual("IsEqual", Reference.GetValueString(false), TEXT("LevelOne"));
		TestEqual("IsEqual", Reference.GetValueString(true), TEXT("var:LevelOne"));
		TestTrue("GetComponent2", Reference.GetComponent(TestActor) == LevelOneComponent);

		Reference.SetValueFromString(TEXT("var:Root"));
		TestTrue("IsNull", !Reference.IsNull());
		TestEqual("IsEqual", Reference.GetValueString(false), TEXT("Root"));
		TestEqual("IsEqual", Reference.GetValueString(true), TEXT("var:Root"));
		TestTrue("GetComponent2", Reference.GetComponent(TestActor) == RootComponent);

		Reference.SetValueFromString(TEXT("path:LevelZeroInstancedNPName"));
		TestTrue("IsNull", !Reference.IsNull());
		TestEqual("IsEqual", Reference.GetValueString(false), TEXT("LevelZeroInstancedNPName"));
		TestEqual("IsEqual", Reference.GetValueString(true), TEXT("path:LevelZeroInstancedNPName"));
		TestTrue("GetComponent2", Reference.GetComponent(TestActor) == LevelZeroInstancedNPComponent);
	}

	return true;
}

#endif
