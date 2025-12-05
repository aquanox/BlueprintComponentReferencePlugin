// Copyright 2024, Aquanox.

#include "BlueprintComponentReferenceTests.h"
#include "BCRTestActor.h"
#include "BCRTestDataAsset.h"
#include "BCRTestActorComponent.h"
#include "BlueprintComponentReference.h"
#include "BlueprintComponentReferenceLibrary.h"
#include "BlueprintComponentReferenceMetadata.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Stats/StatsMisc.h"
#include "Misc/AutomationTest.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, BlueprintComponentReferenceTests);

#if WITH_DEV_AUTOMATION_TESTS

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
			TestActor ?  TestActor->Default_Root : nullptr;
		UActorComponent* const LevelOneComponent =
			TestActor ?  TestActor->Default_LevelOne  : nullptr;
		UActorComponent* const LevelOneConstructNPComponent =
			TestActor ? FindObject<UActorComponent>(TestActor, TEXT("Construct_LevelOne_SomeName")) : nullptr;

		AddInfo(FString::Printf(TEXT("Using actor %s"), * GetNameSafe(TestActor)));

		{
			FBlueprintComponentReference Reference;
			TestTrueExpr(Reference.IsNull());
			TestTrueExpr(Reference.ToString() == TEXT(""));
			TestTrueExpr(Reference.GetComponent(TestActor) == nullptr);
		}

		{
			FName SampleName0 = "Sample";
			FName SampleName1 = "Sample";
			SampleName1.SetNumber(11);
			FName SampleName2 = "Sample";
			SampleName2.SetNumber(22);


			FBlueprintComponentReference Sample0 = FBlueprintComponentReference::ForPath(SampleName0);
			FBlueprintComponentReference Sample1 = FBlueprintComponentReference::ForPath(SampleName1);
			FBlueprintComponentReference Sample2 = FBlueprintComponentReference::ForPath(SampleName2);
			TestTrueExpr(Sample0 != Sample1);
			TestTrueExpr(Sample0 != Sample2);
			TestTrueExpr(Sample1 != Sample2);
			TestTrueExpr(Sample0.ToString() == TEXT("path:Sample"));
			TestTrueExpr(Sample1.ToString() == TEXT("path:Sample_10"));
			TestTrueExpr(Sample2.ToString() == TEXT("path:Sample_21"));
		}

		{
			FBlueprintComponentReference BasicReference;
			BasicReference.ParseString(TEXT("Default_LevelOne"));

			TestTrueExpr(BasicReference == FBlueprintComponentReference(EBlueprintComponentReferenceMode::Property, "Default_LevelOne"));
			TestTrueExpr(BasicReference == FBlueprintComponentReference(TEXT("property:Default_LevelOne")));
			TestTrueExpr(BasicReference != FBlueprintComponentReference(TEXT("Default_LevelOne_f34t25tg2")));
			TestTrueExpr(!BasicReference.IsNull());
			TestTrueExpr(BasicReference.ToString() == TEXT("property:Default_LevelOne"));
			TestTrueExpr(BasicReference.GetComponent(TestActor) == LevelOneComponent);
		}

		{
			FBlueprintComponentReference PropReference;
			PropReference.ParseString(TEXT("property:Default_Root"));

			TestTrueExpr(PropReference == FBlueprintComponentReference(EBlueprintComponentReferenceMode::Property, "Default_Root"));
			TestTrueExpr(PropReference == FBlueprintComponentReference(TEXT("property:Default_Root")));
			TestTrueExpr(PropReference != FBlueprintComponentReference(TEXT("path:Default_Root")));
			TestTrueExpr(!PropReference.IsNull());
			TestTrueExpr(PropReference.ToString() == TEXT("property:Default_Root"));
			TestTrueExpr(PropReference.GetComponent(TestActor) == TestRootComponent);
		}

		{
			FBlueprintComponentReference PathReference;
			PathReference.ParseString(TEXT("path:Construct_LevelOne_SomeName"));

			TestTrueExpr(PathReference == FBlueprintComponentReference(EBlueprintComponentReferenceMode::Path, "Construct_LevelOne_SomeName"));
			TestTrueExpr(PathReference != FBlueprintComponentReference(TEXT("Construct_LevelOne_SomeName")));
			TestTrueExpr(PathReference == FBlueprintComponentReference(TEXT("path:Construct_LevelOne_SomeName")));
			TestTrueExpr(!PathReference.IsNull());
			TestTrueExpr(PathReference.ToString() == TEXT("path:Construct_LevelOne_SomeName"));
			TestTrueExpr(PathReference.GetComponent(TestActor) == LevelOneConstructNPComponent);
		}

		{
			FBlueprintComponentReference BadReference;
			BadReference.ParseString(TEXT("DoesNotExist"));

			TestTrueExpr(BadReference == FBlueprintComponentReference(EBlueprintComponentReferenceMode::Property, "DoesNotExist"));
			TestTrueExpr(BadReference == FBlueprintComponentReference(TEXT("DoesNotExist")));
			TestTrueExpr(!BadReference.IsNull());
			TestTrueExpr(BadReference.GetComponent(TestActor) == nullptr);
		}
		{
			FBlueprintComponentReference RootReference;
			RootReference.ParseString(TEXT("property:RootComponent"));

			TestTrueExpr(!RootReference.IsNull());
			TestTrueExpr(RootReference.GetComponent(TestActor) == RootComponent);
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
	FBlueprintComponentReference TestBaseReference("property:Default_Root");
	FBlueprintComponentReference MeshPathReference(EBlueprintComponentReferenceMode::Path, ACharacter::MeshComponentName);
	FBlueprintComponentReference MeshVarReference(EBlueprintComponentReferenceMode::Property, TEXT("Mesh"));
	FBlueprintComponentReference BadReference("property:DoesNotExist");

	TestTrueExpr(UBlueprintComponentReferenceLibrary::IsNullComponentReference(NullReference));
	TestTrueExpr(!UBlueprintComponentReferenceLibrary::IsNullComponentReference(TestBaseReference));

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
	TestTrue("TestBase.GetReferencedComponent.Result", Result == TestActorReal->Default_Root);

	TestFalse("TestBase2.GetReferencedComponent", UBlueprintComponentReferenceLibrary::GetReferencedComponent(TestBaseReference, TestActorReal, USkeletalMeshComponent::StaticClass(), Result));
	TestTrue("TestBase2.GetReferencedComponent.Result", Result == nullptr);

	TestTrue("MeshPathReference.GetReferencedComponent", UBlueprintComponentReferenceLibrary::GetReferencedComponent(MeshPathReference, TestActorReal, nullptr, Result));
	TestTrue("MeshPathReference.GetReferencedComponent.Result", Result == TestActorReal->GetMesh());

	TestTrue("MeshPathReference2.GetReferencedComponent", UBlueprintComponentReferenceLibrary::GetReferencedComponent(MeshPathReference, TestActorReal, USkeletalMeshComponent::StaticClass(), Result));
	TestTrue("MeshPathReference2.GetReferencedComponent.Result", Result == TestActorReal->GetMesh());

	TestFalse("MeshPathReference3.GetReferencedComponent", UBlueprintComponentReferenceLibrary::GetReferencedComponent(MeshPathReference, TestActorReal, UStaticMeshComponent::StaticClass(), Result));
	TestTrue("MeshPathReference3.GetReferencedComponent.Result", Result == nullptr);

	TestTrue("MeshVarReference.GetReferencedComponent", UBlueprintComponentReferenceLibrary::GetReferencedComponent(MeshVarReference, TestActorReal, nullptr, Result));
	TestTrue("MeshVarReference.GetReferencedComponent.Result", Result == TestActorReal->GetMesh());

	TestTrue("MeshVarReference2.GetReferencedComponent", UBlueprintComponentReferenceLibrary::GetReferencedComponent(MeshVarReference, TestActorReal, USkeletalMeshComponent::StaticClass(),  Result));
	TestTrue("MeshVarReference2.GetReferencedComponent.Result", Result == TestActorReal->GetMesh());

	TestFalse("MeshVarReference3.GetReferencedComponent", UBlueprintComponentReferenceLibrary::GetReferencedComponent(MeshVarReference, TestActorReal, UStaticMeshComponent::StaticClass(),  Result));
	TestTrue("MeshVarReference3.GetReferencedComponent.Result", Result == nullptr);

	TestFalse("Bad.GetReferencedComponent", UBlueprintComponentReferenceLibrary::GetReferencedComponent(BadReference, TestActorReal, nullptr, Result));
	TestTrue("Bad.GetReferencedComponent.Result", Result == nullptr);

	return true;
}

#endif
