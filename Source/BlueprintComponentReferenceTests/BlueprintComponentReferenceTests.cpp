// Copyright 2024, Aquanox.

#include "BCRTestActor.h"
#include "BlueprintComponentReference.h"
#include "BlueprintComponentReferenceLibrary.h"

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/SkeletalMeshComponent.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlueprintComponentReferenceTests_Core,
	"BlueprintComponentReference.Core", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::HighPriority);

bool FBlueprintComponentReferenceTests_Core::RunTest(FString const&)
{
	FTestWorldScope World;

	ABCRTestActor* const TestActorNull = nullptr;
	ABCRTestActor* const TestActorS = World->SpawnActor<ABCRTestActor>();
	ABCRTestActor* const TestActorD = GetMutableDefault<ABCRTestActor>();

	for(ABCRTestActor* TestActor : {  TestActorNull, TestActorS, TestActorD })
	{
		UActorComponent* const RootComponent =
			TestActor ?  TestActor->GetRootComponent() : nullptr;
		UActorComponent* const TestRootComponent =
			TestActor ?  TestActor->TestBase : nullptr;
		UActorComponent* const LevelOneComponent =
			TestActor ?  TestActor->LevelOne  : nullptr;
		UActorComponent* const LevelOneConstructNPComponent =
			TestActor ? FindObject<UActorComponent>(TestActor, TEXT("LevelOneConstructNPName")) : nullptr;

		AddInfo(FString::Printf(TEXT("Using actor %s"), * GetNameSafe(TestActor)));

		{
			FBlueprintComponentReference Reference;
			TestTrue("Empty.IsNull", Reference.IsNull());
			TestEqual("Empty.ToString", Reference.ToString(), TEXT(""));
			TestTrue("Empty.GetComponent", Reference.GetComponent(TestActor) == nullptr);
		}

		{
			FBlueprintComponentReference Reference;
			Reference.ParseString(TEXT("LevelOne"));

			TestTrue("Basic.IsEqual1", Reference == FBlueprintComponentReference(EBlueprintComponentReferenceMode::Property, "LevelOne"));
			TestTrue("Basic.IsEqual2", Reference == FBlueprintComponentReference(TEXT("property:LevelOne")));
			TestTrue("Basic.IsEqual3", Reference != FBlueprintComponentReference(TEXT("LevelTwo")));
			TestTrue("Basic.IsNull", !Reference.IsNull());
			TestEqual("Basic.ToString", Reference.ToString(), TEXT("property:LevelOne"));
			TestTrue("Basic.GetComponent", Reference.GetComponent(TestActor) == LevelOneComponent);
		}

		{
			FBlueprintComponentReference Reference;
			Reference.ParseString(TEXT("property:TestBase"));

			TestTrue("Full.IsEqual1", Reference == FBlueprintComponentReference(EBlueprintComponentReferenceMode::Property, "TestBase"));
			TestTrue("Full.IsEqual2", Reference == FBlueprintComponentReference(TEXT("property:TestBase")));
			TestTrue("Full.IsEqual3", Reference != FBlueprintComponentReference(TEXT("path:TestBase")));
			TestTrue("Full.IsNull", !Reference.IsNull());
			TestEqual("Full.ToString", Reference.ToString(), TEXT("property:TestBase"));
			TestTrue("Full.GetComponent", Reference.GetComponent(TestActor) == TestRootComponent);
		}

		{
			FBlueprintComponentReference Reference;
			Reference.ParseString(TEXT("path:LevelOneConstructNPName"));

			TestTrue("Path.IsEqual1", Reference == FBlueprintComponentReference(EBlueprintComponentReferenceMode::Path, "LevelOneConstructNPName"));
			TestTrue("Path.IsEqual2", Reference != FBlueprintComponentReference(TEXT("LevelOneConstructNPName")));
			TestTrue("Path.IsEqual3", Reference == FBlueprintComponentReference(TEXT("path:LevelOneConstructNPName")));
			TestTrue("Path.IsNull", !Reference.IsNull());
			TestEqual("Path.ToString", Reference.ToString(), TEXT("path:LevelOneConstructNPName"));
			TestTrue("Path.GetComponent", Reference.GetComponent(TestActor) == LevelOneConstructNPComponent);
		}

		{
			FBlueprintComponentReference Reference;
			Reference.ParseString(TEXT("DoesNotExist"));

			TestTrue("Bad.IsEqual", Reference == FBlueprintComponentReference(EBlueprintComponentReferenceMode::Property, "DoesNotExist"));
			TestTrue("Bad.IsEqual2", Reference == FBlueprintComponentReference(TEXT("DoesNotExist")));
			TestTrue("Bad.IsNull", !Reference.IsNull());
			TestTrue("Bad.GetComponent", Reference.GetComponent(TestActor) == nullptr);
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlueprintComponentReferenceTests_Library,
	"BlueprintComponentReference.Library", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::HighPriority);

bool FBlueprintComponentReferenceTests_Library::RunTest(FString const&)
{
	FTestWorldScope World;

	ABCRTestActor* const TestActorNull = nullptr;
	ABCRTestActor* const TestActorReal = World->SpawnActor<ABCRTestActor>();
	ABCRTestActor* const TestActorDefault = GetMutableDefault<ABCRTestActor>();
	
	FBlueprintComponentReference NullReference;
	FBlueprintComponentReference TestBaseReference("property:TestBase");
	FBlueprintComponentReference MeshPathReference(EBlueprintComponentReferenceMode::Path, ACharacter::MeshComponentName);
	FBlueprintComponentReference MeshVarReference(EBlueprintComponentReferenceMode::Property, TEXT("Mesh"));
	FBlueprintComponentReference BadReference("property:DoesNotExist");
	
	TestTrue("IsNullComponentReference.1", UBlueprintComponentReferenceLibrary::IsNullComponentReference(NullReference));
	TestFalse("IsNullComponentReference.2", UBlueprintComponentReferenceLibrary::IsNullComponentReference(TestBaseReference));

	FBlueprintComponentReference CopyReference(TestBaseReference);
	TestTrue("InvalidateComponentReference.1", !CopyReference.IsNull());
	UBlueprintComponentReferenceLibrary::InvalidateComponentReference(CopyReference);
	TestTrue("InvalidateComponentReference.2", CopyReference.IsNull());

	UActorComponent* Result = nullptr;

	// TryGetReferencedComponent and GetReferencedComponent same
	TestFalse("InvalidThings.GetReferencedComponent", UBlueprintComponentReferenceLibrary::GetReferencedComponent(BadReference, TestActorNull, UActorComponent::StaticClass(), Result));
	TestTrue("InvalidThings.GetReferencedComponent.Result", Result == nullptr);
	
	TestFalse("Null.GetReferencedComponent", UBlueprintComponentReferenceLibrary::GetReferencedComponent(NullReference, TestActorReal, nullptr, Result));
	TestTrue("Null.GetReferencedComponent.Result", Result == nullptr);

	TestTrue("TestBase.GetReferencedComponent", UBlueprintComponentReferenceLibrary::GetReferencedComponent(TestBaseReference, TestActorReal, nullptr, Result));
	TestTrue("TestBase.GetReferencedComponent.Result", Result == TestActorReal->TestBase);

	TestTrue("MeshPathReference.GetReferencedComponent", UBlueprintComponentReferenceLibrary::GetReferencedComponent(MeshPathReference, TestActorReal, nullptr, Result));
	TestTrue("MeshPathReference.GetReferencedComponent.Result", Result == TestActorReal->GetMesh());
	
	TestTrue("MeshVarReference.GetReferencedComponent", UBlueprintComponentReferenceLibrary::GetReferencedComponent(MeshVarReference, TestActorReal, nullptr, Result));
	TestTrue("MeshVarReference.GetReferencedComponent.Result", Result == TestActorReal->GetMesh());

	TestFalse("Bad.GetReferencedComponent", UBlueprintComponentReferenceLibrary::GetReferencedComponent(BadReference, TestActorReal, nullptr, Result));
	TestTrue("Bad.GetReferencedComponent.Result", Result == nullptr);
	
	return true;
}

#endif
