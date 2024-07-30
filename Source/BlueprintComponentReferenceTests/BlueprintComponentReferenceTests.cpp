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
		UActorComponent* const LevelOneConstructNPComponent =
			TestActor ? FindObject<UActorComponent>(TestActor, TEXT("LevelOneConstructNPName")) : nullptr;

		AddInfo(FString::Printf(TEXT("Using actor %s"), * GetNameSafe(TestActor)));

		{
			FBlueprintComponentReference Reference;
			TestTrue("Empty.IsNull", Reference.IsNull());
			TestEqual("Empty.IsEqual", Reference.GetValueString(false), TEXT(""));
			TestEqual("Empty.IsEqual2", Reference.GetValueString(true), TEXT(""));
			TestTrue("Empty.GetComponent", Reference.GetComponent(TestActor, false) == nullptr);
			TestTrue("Empty.GetComponent2", Reference.GetComponent(TestActor, true) == RootComponent);
		}

		{
			FBlueprintComponentReference Reference;
			Reference.SetValueFromString(TEXT("LevelOne"));

			TestTrue("Basic.IsEqual", Reference == FBlueprintComponentReference(EBlueprintComponentReferenceMode::Property, "LevelOne"));
			TestTrue("Basic.IsEqual2", Reference == FBlueprintComponentReference(TEXT("var:LevelOne")));
			TestTrue("Basic.IsEqual3", Reference != FBlueprintComponentReference(TEXT("LevelTwo")));
			TestTrue("Basic.IsNull", !Reference.IsNull());
			TestEqual("Basic.IsEqual", Reference.GetValueString(false), TEXT("LevelOne"));
			TestEqual("Basic.IsEqual2", Reference.GetValueString(true), TEXT("var:LevelOne"));
			TestTrue("Basic.GetComponent", Reference.GetComponent(TestActor, false) == LevelOneComponent);
			TestTrue("Basic.GetComponent2", Reference.GetComponent(TestActor, true) == LevelOneComponent);
		}

		{
			FBlueprintComponentReference Reference;
			Reference.SetValueFromString(TEXT("var:Root"));

			TestTrue("Full.IsEqual", Reference == FBlueprintComponentReference(EBlueprintComponentReferenceMode::Property, "Root"));
			TestTrue("Full.IsEqual2", Reference == FBlueprintComponentReference(TEXT("var:Root")));
			TestTrue("Full.IsEqual3", Reference != FBlueprintComponentReference(TEXT("path:Root")));
			TestTrue("Full.IsNull", !Reference.IsNull());
			TestEqual("Full.IsEqual", Reference.GetValueString(false), TEXT("Root"));
			TestEqual("Full.IsEqual2", Reference.GetValueString(true), TEXT("var:Root"));
			TestTrue("Full.GetComponent", Reference.GetComponent(TestActor, false) == RootComponent);
			TestTrue("Full.GetComponent2", Reference.GetComponent(TestActor, true) == RootComponent);
		}

		{
			FBlueprintComponentReference Reference;
			Reference.SetValueFromString(TEXT("path:LevelOneConstructNPName"));

			TestTrue("Path.IsEqual", Reference == FBlueprintComponentReference(EBlueprintComponentReferenceMode::Path, "LevelOneConstructNPName"));
			TestTrue("Path.IsEqual2", Reference != FBlueprintComponentReference(TEXT("LevelOneConstructNPName")));
			TestTrue("Path.IsEqual3", Reference == FBlueprintComponentReference(TEXT("path:LevelOneConstructNPName")));
			TestTrue("Path.IsNull", !Reference.IsNull());
			TestEqual("Path.IsEqual", Reference.GetValueString(false), TEXT("LevelOneConstructNPName"));
			TestEqual("Path.IsEqual2", Reference.GetValueString(true), TEXT("path:LevelOneConstructNPName"));
			TestTrue("Path.GetComponent", Reference.GetComponent(TestActor, false) == LevelOneConstructNPComponent);
		}

		{
			FBlueprintComponentReference Reference;

			TestTrue("None.IsEqual", Reference == FBlueprintComponentReference());
			TestTrue("None.IsNull", Reference.IsNull());
			TestTrue("None.GetComponent", Reference.GetComponent(TestActor, false) == nullptr);
			TestTrue("None.GetComponent2", Reference.GetComponent(TestActor, true) == RootComponent);
		}

		{
			FBlueprintComponentReference Reference;
			Reference.SetValueFromString(TEXT("DoesNotExist"));

			TestTrue("Bad.IsEqual", Reference == FBlueprintComponentReference(EBlueprintComponentReferenceMode::Property, "DoesNotExist"));
			TestTrue("Bad.IsEqual2", Reference == FBlueprintComponentReference(TEXT("DoesNotExist")));
			TestTrue("Bad.IsNull", !Reference.IsNull());
			TestTrue("Bad.GetComponent", Reference.GetComponent(TestActor, false) == nullptr);
			TestTrue("Bad.GetComponent2", Reference.GetComponent(TestActor, true) == RootComponent);
		}
	}

	return true;
}

#endif
