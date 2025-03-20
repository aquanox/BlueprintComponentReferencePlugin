// Copyright 2024, Aquanox.

#include "BCRTestActor.h"
#include "BCRTestDataAsset.h"
#include "BCRTestActorComponent.h"
#include "BlueprintComponentReference.h"
#include "BlueprintComponentReferenceLibrary.h"
#include "BlueprintComponentReferenceMetadata.h"
#include "CachedBlueprintComponentReference.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"

#include "GameFramework/CharacterMovementComponent.h"

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

static FBlueprintComponentReference MakePathRef(FName InValue) { return FBlueprintComponentReference(EBlueprintComponentReferenceMode::Path, InValue); }
static FBlueprintComponentReference MakePropertyRef(FName InValue) { return FBlueprintComponentReference(EBlueprintComponentReferenceMode::Property, InValue); }

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
			TestTrue("Empty.IsNull", Reference.IsNull());
			TestEqual("Empty.ToString", Reference.ToString(), TEXT(""));
			TestTrue("Empty.GetComponent", Reference.GetComponent(TestActor) == nullptr);
		}

		{
			FBlueprintComponentReference Reference;
			Reference.ParseString(TEXT("Default_LevelOne"));

			TestTrue("Basic.IsEqual1", Reference == FBlueprintComponentReference(EBlueprintComponentReferenceMode::Property, "Default_LevelOne"));
			TestTrue("Basic.IsEqual2", Reference == FBlueprintComponentReference(TEXT("property:Default_LevelOne")));
			TestTrue("Basic.IsEqual3", Reference != FBlueprintComponentReference(TEXT("Default_LevelOne_f34t25tg2")));
			TestTrue("Basic.IsNull", !Reference.IsNull());
			TestEqual("Basic.ToString", Reference.ToString(), TEXT("property:Default_LevelOne"));
			TestTrue("Basic.GetComponent", Reference.GetComponent(TestActor) == LevelOneComponent);
		}

		{
			FBlueprintComponentReference Reference;
			Reference.ParseString(TEXT("property:Default_Root"));

			TestTrue("Full.IsEqual1", Reference == FBlueprintComponentReference(EBlueprintComponentReferenceMode::Property, "Default_Root"));
			TestTrue("Full.IsEqual2", Reference == FBlueprintComponentReference(TEXT("property:Default_Root")));
			TestTrue("Full.IsEqual3", Reference != FBlueprintComponentReference(TEXT("path:Default_Root")));
			TestTrue("Full.IsNull", !Reference.IsNull());
			TestEqual("Full.ToString", Reference.ToString(), TEXT("property:Default_Root"));
			TestTrue("Full.GetComponent", Reference.GetComponent(TestActor) == TestRootComponent);
		}

		{
			FBlueprintComponentReference Reference;
			Reference.ParseString(TEXT("path:Construct_LevelOne_SomeName"));

			TestTrue("Path.IsEqual1", Reference == FBlueprintComponentReference(EBlueprintComponentReferenceMode::Path, "Construct_LevelOne_SomeName"));
			TestTrue("Path.IsEqual2", Reference != FBlueprintComponentReference(TEXT("Construct_LevelOne_SomeName")));
			TestTrue("Path.IsEqual3", Reference == FBlueprintComponentReference(TEXT("path:Construct_LevelOne_SomeName")));
			TestTrue("Path.IsNull", !Reference.IsNull());
			TestEqual("Path.ToString", Reference.ToString(), TEXT("path:Construct_LevelOne_SomeName"));
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
	FBlueprintComponentReference TestBaseReference("property:Default_Root");
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlueprintComponentReferenceTests_Cached,
	"BlueprintComponentReference.Cached", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::HighPriority);

bool FBlueprintComponentReferenceTests_Cached::RunTest(FString const&)
{
	FTestWorldScope World;

	auto* TestActor = World->SpawnActor<ABCRCachedTestActor>();
	TestTrueExpr(TestActor != nullptr);

	//======================================
	
	USceneComponent* ExpectedComp = TestActor->GetMesh();
	TestActor->ReferenceSingle = MakePathRef(ABCRCachedTestActor::MeshComponentName);

	TestTrueExpr(ExpectedComp != nullptr);
	TestTrueExpr(!TestActor->ReferenceSingle.IsNull());
	TestTrueExpr(TestActor->ReferenceSingle.GetComponent(TestActor) == ExpectedComp);
	TestTrueExpr(TestActor == TestActor->CachedReferenceSingle.GetBaseActor());
	TestTrueExpr(TestActor->CachedReferenceSingle.Get() == ExpectedComp);
	TestTrueExpr(TestActor->CachedReferenceSingle.Get(TestActor) == ExpectedComp);
	
	//======================================

	TArray<UBCRTestSceneComponent*> ExpectedComps;
	TArray<FName> ExpectedKeys;
	
	for (int i = 0; i < 4; ++i)
	{
		auto* Comp = Cast<UBCRTestSceneComponent>(TestActor->AddComponentByClass(UBCRTestSceneComponent::StaticClass(), true, FTransform::Identity, false));
		Comp->SampleName =  *FString::Printf(TEXT("MapKey%p"), Comp);

		ExpectedComps.Add(Comp);

		TestActor->AddInstanceComponent(Comp);

		TestActor->ReferenceArray.Add(MakePathRef(Comp->GetFName()));

		ExpectedKeys.Add(Comp->SampleName );
		TestActor->ReferenceMap.Add(Comp->SampleName , MakePathRef(Comp->GetFName()));
	}

	TestTrueExpr(ExpectedComps.Num() == TestActor->ReferenceArray.Num() );
	TestTrueExpr(TestActor == TestActor->CachedReferenceArray.GetBaseActor() );
	TestTrueExpr(ExpectedComps.Num() == TestActor->CachedReferenceArray.Num() );

	TestTrueExpr(TestActor->ReferenceArray[0].GetComponent(TestActor) == ExpectedComps[0]);
	TestTrueExpr(TestActor->ReferenceArray[1].GetComponent(TestActor) == ExpectedComps[1]);
	TestTrueExpr(TestActor->ReferenceArray[2].GetComponent(TestActor) == ExpectedComps[2]);
	TestTrueExpr(TestActor->ReferenceArray[3].GetComponent(TestActor) == ExpectedComps[3]);

	TestTrueExpr(TestActor->CachedReferenceArray.Get(TestActor, 0) == ExpectedComps[0]);
	TestTrueExpr(TestActor->CachedReferenceArray.Get(TestActor, 1) == ExpectedComps[1]);
	TestTrueExpr(TestActor->CachedReferenceArray.Get(TestActor, 2) == ExpectedComps[2]);
	TestTrueExpr(TestActor->CachedReferenceArray.Get(TestActor, 3) == ExpectedComps[3]);
	
	TestTrueExpr(TestActor->CachedReferenceArray.Get(0) == ExpectedComps[0]);
	TestTrueExpr(TestActor->CachedReferenceArray.Get(1) == ExpectedComps[1]);
	TestTrueExpr(TestActor->CachedReferenceArray.Get(2) == ExpectedComps[2]);
	TestTrueExpr(TestActor->CachedReferenceArray.Get(3) == ExpectedComps[3]);
	
	//======================================

	TestTrueExpr(ExpectedKeys.Num() == TestActor->ReferenceMap.Num() );
	TestTrueExpr(TestActor == TestActor->CachedReferenceMap.GetBaseActor() );

	for (FName ExpectedKey : ExpectedKeys)
	{
		FBlueprintComponentReference& Ref = TestActor->ReferenceMap.FindChecked(ExpectedKey);
		UActorComponent* Direct = Ref.GetComponent(TestActor);
		USceneComponent* CachedBased = TestActor->CachedReferenceMap.Get(ExpectedKey);
		USceneComponent* Cached = TestActor->CachedReferenceMap.Get(TestActor, ExpectedKey);
		TestTrueExpr(Direct != nullptr);
		TestTrueExpr(Cached != nullptr);
		TestTrueExpr(CachedBased != nullptr);
		TestTrueExpr(Cached == CachedBased);
		TestTrueExpr(Cached == Direct);
	}
	
 	return true;
}

#endif
