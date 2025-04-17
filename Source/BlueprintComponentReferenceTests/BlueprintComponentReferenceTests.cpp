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
#include "Stats/StatsMisc.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlueprintComponentReferenceTests_Cached,
	"BlueprintComponentReference.Cached", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter | EAutomationTestFlags::HighPriority);

bool FBlueprintComponentReferenceTests_Cached::RunTest(FString const&)
{
	FTestWorldScope World;

	auto* TestActor = World->SpawnActor<ABCRCachedTestActor>();
	TestTrueExpr(TestActor != nullptr);
	TestActor->ReferenceSingle.Invalidate();
	TestActor->ReferenceArray.Empty();
	TestActor->ReferenceMap.Empty();
	TestActor->ReferenceMapKey.Empty();

	//======================================

	USceneComponent* ExpectedComp = TestActor->GetMesh();
	TestActor->ReferenceSingle = FBlueprintComponentReference::ForPath(ABCRCachedTestActor::MeshComponentName);

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
		auto* Comp = NewObject<UBCRTestSceneComponent>(TestActor);
		Comp->SampleName =  *FString::Printf(TEXT("MapKey%p"), Comp);
		Comp->SetupAttachment(TestActor->GetRootComponent());
		Comp->RegisterComponent();
		
		ExpectedComps.Add(Comp);

		UE_LOG(LogTemp, Log, TEXT("Make component. Ptr=%p Name=%s PathName=%s"), Comp, *Comp->GetName(), *Comp->GetPathName(TestActor));

		TestActor->ReferenceArray.Add(FBlueprintComponentReference::ForPath(Comp->GetFName()));

		ExpectedKeys.Add(Comp->SampleName);
		TestActor->ReferenceMap.Add(Comp->SampleName, FBlueprintComponentReference::ForPath(Comp->GetFName()));

		FBCRTestStrustData Data;
		Data.Sample = Comp->SampleName;
		TestActor->ReferenceMapKey.Add(FBlueprintComponentReference::ForPath(Comp->GetFName()), Data);
	}

	TestTrueExpr(ExpectedComps.Num() == TestActor->ReferenceArray.Num() );
	TestTrueExpr(TestActor == TestActor->CachedReferenceArray.GetBaseActorPtr() );
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

	//======================================

	TestTrueExpr(ExpectedKeys.Num() == TestActor->ReferenceMapKey.Num() );
	TestTrueExpr(TestActor == TestActor->CachedReferenceMapKey.GetBaseActor() );

	for (UActorComponent* ExpectedKey : ExpectedComps)
	{
		UBCRTestSceneComponent* InKey = Cast<UBCRTestSceneComponent>(ExpectedKey);
		UE_LOG(LogTemp, Log, TEXT("Searching key %p name=%s"), InKey, *InKey->GetName());
		FBCRTestStrustData* CachedBased = TestActor->CachedReferenceMapKey.Get(InKey);
		FBCRTestStrustData* Cached = TestActor->CachedReferenceMapKey.Get(TestActor, InKey);
		UE_LOG(LogTemp, Log, TEXT("Found data %s"), *Cached->Sample.ToString());
		TestTrueExpr(Cached == CachedBased);
		TestTrueExpr(InKey->SampleName == Cached->Sample);
		TestTrueExpr(InKey->SampleName == CachedBased->Sample);
	}

 	return true;
}

#define X_NAME_OF(x) TEXT(#x)
// single prop sequential resolve vs direct
template<int MaxNum>
struct PerfRunner_Single
{
	AActor* Actor;
	FBlueprintComponentReference Ref;

	TCachedComponentReference<USceneComponent, TObjectPtr> CachedStrong;
	
	TCachedComponentReference<USceneComponent, TWeakObjectPtr> CachedWeak;
	
	TCachedComponentReference<USceneComponent, TWeakObjectPtr> CachedWarm;
	
	PerfRunner_Single(AActor* InActor, const FBlueprintComponentReference& InRef)
		: Actor(InActor), CachedStrong(InActor, &Ref), CachedWeak(InActor, &Ref), CachedWarm(InActor, &Ref)
	{
		Ref = InRef;
	}
	
	FString GenerateDescription(const TCHAR* AccessType)
	{
		return FString::Printf(TEXT("PerfRunner_Single [%-10s] [%-10s] %-8d loops"), AccessType, *Ref.ToString(), MaxNum);
	}

	void Run()
	{
		{
			FScopeLogTime Scope(*GenerateDescription(TEXT("Direct")), nullptr, FConditionalScopeLogTime::ScopeLog_Milliseconds);
			for (int32 N = 0; N < MaxNum; ++N)
			{
				Ref.GetComponent(Actor);
			}
		}
		
		{
			FScopeLogTime Scope(*GenerateDescription(TEXT("Strong")), nullptr, FConditionalScopeLogTime::ScopeLog_Milliseconds);
			for (int32 N = 0; N < MaxNum; ++N)
			{
				CachedStrong.Get(Actor);
			}
		}
		
		{
			FScopeLogTime Scope(*GenerateDescription(TEXT("Weak")), nullptr, FConditionalScopeLogTime::ScopeLog_Milliseconds);
			for (int32 N = 0; N < MaxNum; ++N)
			{
				CachedWeak.Get(Actor);
			}
		}
		
		{
			CachedWarm.WarmupCache(Actor);
			
			FScopeLogTime Scope(*GenerateDescription(TEXT("WWarm")), nullptr, FConditionalScopeLogTime::ScopeLog_Milliseconds);
			for (int32 N = 0; N < MaxNum; ++N)
			{
				CachedWarm.Get(Actor);
			}
		}
	}
};

// array random access with resolve vs cached resolve
template<int32 NumEntries, int NumAccess>
struct PerfRunner_Array
{
	AActor* Actor;
	FBlueprintComponentReference Ref;
	TArray<FBlueprintComponentReference> RefArray;
	TArray<int32> RefAccessSequence;

	TCachedComponentReferenceArray<USceneComponent, TObjectPtr> CachedStrong;
	TCachedComponentReferenceArray<USceneComponent, TWeakObjectPtr> CachedWeak;
	TCachedComponentReferenceArray<USceneComponent, TWeakObjectPtr> CachedWarm;
	
	PerfRunner_Array(AActor* InActor, const FBlueprintComponentReference& InRef)
		: Actor(InActor), CachedStrong(InActor, &RefArray), CachedWeak(InActor, &RefArray), CachedWarm(InActor, &RefArray)
	{
		FRandomStream RandomStream( 0xC0FFEE );

		Ref = InRef;
		
		RefArray.SetNum(NumEntries);
		for (int32 Idx = 0; Idx < NumEntries; ++Idx)
		{
			RefArray[Idx] = InRef;
		}
		RefAccessSequence.SetNum(NumAccess);
		for (int32 Idx = 0; Idx < NumAccess; ++Idx)
        {
        	RefAccessSequence[Idx] = RandomStream.RandHelper(NumEntries);
        }
	}
	
	FString GenerateDescription(const TCHAR* AccessType)
	{
		return FString::Printf(TEXT("PerfRunner_Array [%-10s] [%-10s] %-8d access of %-8d"), AccessType, *Ref.ToString(), NumAccess, NumEntries);
	}
	
	void Run()
	{
		{
			FScopeLogTime Scope(*GenerateDescription(TEXT("Direct")), nullptr, FConditionalScopeLogTime::ScopeLog_Milliseconds);
			for (int32 Index : RefAccessSequence)
			{
				RefArray[Index].GetComponent(Actor);
			}
		}
		{
			FScopeLogTime Scope(*GenerateDescription(TEXT("Strong")), nullptr, FConditionalScopeLogTime::ScopeLog_Milliseconds);
			for (int32 Index : RefAccessSequence)
			{
				CachedStrong.Get(Actor, Index);
			}
		}
		{
			FScopeLogTime Scope(*GenerateDescription(TEXT("Weak")), nullptr, FConditionalScopeLogTime::ScopeLog_Milliseconds);
			for (int32 Index : RefAccessSequence)
			{
				CachedWeak.Get(Actor, Index);
			}
		}
		{
			CachedWarm.WarmupCache(Actor);
			
			FScopeLogTime Scope(*GenerateDescription(TEXT("WWarm")), nullptr, FConditionalScopeLogTime::ScopeLog_Milliseconds);
			for (int32 Index : RefAccessSequence)
			{
				CachedWarm.Get(Actor, Index);
			}
		}
	}
};
// random map key access vs loop
template<int32 NumComponents, int32 NumEntries, int NumAccess>
struct PerfRunner_MapKey
{
	AActor* Actor;
	FBlueprintComponentReference Ref;

	TMap<FBlueprintComponentReference, int32> RefMap;
	TCachedComponentReferenceMapKey<USceneComponent, int32> CachedMap;
	TCachedComponentReferenceMapKey<USceneComponent, int32> CachedMapWarm;

	TArray<USceneComponent*> Components;
	TArray<USceneComponent*> ComponentsInUse;
	TArray<USceneComponent*> RefAccessSequence;
	
	PerfRunner_MapKey(AActor* InActor)
		: Actor(InActor), CachedMap(InActor, &RefMap), CachedMapWarm(InActor, &RefMap)
	{
		FRandomStream RandomStream( 0xC0FFEE );

		// setup components for test
		Components.Reserve(NumComponents + 10);
		InActor->GetComponents(UBCRTestSceneComponent::StaticClass(), Components);
		while (Components.Num() < NumComponents)
		{
			auto* Component = NewObject<UBCRTestSceneComponent>(InActor);
			Component->SetupAttachment(InActor->GetRootComponent());
			Component->RegisterComponent();
			Components.Add(Component);
		}

		// setup map contents
		ComponentsInUse.Reserve(NumEntries);
		while (RefMap.Num() < NumEntries)
		{
			int32 RIdx = RandomStream.RandHelper(Components.Num());
			USceneComponent* Target = Components[RIdx];

			RefMap.Add(FBlueprintComponentReference::ForPath(Target->GetFName()), RIdx);

			ComponentsInUse.AddUnique(Target);
		}
		
		// setup search sequence
		RefAccessSequence.Reserve(NumAccess);
		for (int32 Idx = 0; Idx < NumAccess; ++Idx)
		{
			RefAccessSequence.Add( ComponentsInUse[ RandomStream.RandHelper(ComponentsInUse.Num()) ]  );
		}
	}

	FString GenerateDescription(const TCHAR* AccessType)
	{
		return FString::Printf(TEXT("PerfRunner_MapKey [%-10s] [%-10s] %-8d access of %-8d/%-8d"), AccessType, *Ref.ToString(), NumAccess, NumEntries, NumComponents);
	}

	int DirectSearch(AActor* InActor, UActorComponent* InKey)
	{
		for (auto& RefToValue : RefMap)
		{
			if (RefToValue.Key.GetComponent(InActor) == InKey)
			{
				return RefToValue.Value;
			}
		}
		return -1;
	}
	
	void Run()
	{
		{
			FScopeLogTime Scope(*GenerateDescription(TEXT("Direct")), nullptr, FConditionalScopeLogTime::ScopeLog_Milliseconds);
			for (UActorComponent* Index : RefAccessSequence)
			{
				DirectSearch(Actor, Index);
			}
		}
		{
			FScopeLogTime Scope(*GenerateDescription(TEXT("Weak")), nullptr, FConditionalScopeLogTime::ScopeLog_Milliseconds);
			for (const USceneComponent* Index : RefAccessSequence)
			{
				CachedMap.Get(Actor, Index);
			}
		}
		{
			CachedMapWarm.WarmupCache(Actor);
			
			FScopeLogTime Scope(*GenerateDescription(TEXT("Warm")), nullptr, FConditionalScopeLogTime::ScopeLog_Milliseconds);
			for (const USceneComponent* Index : RefAccessSequence)
			{
				CachedMapWarm.Get(Actor, Index);
			}
		}
	}
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlueprintComponentReferenceTests_Perf,
	"BlueprintComponentReference.Perf", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FBlueprintComponentReferenceTests_Perf::RunTest(FString const&)
{
	FTestWorldScope World;

	auto* TestActor = World->SpawnActor<ABCRCachedTestActor>();
	TestTrueExpr(TestActor != nullptr);
	TestActor->ReferenceSingle.Invalidate();
	TestActor->ReferenceArray.Empty();
	TestActor->ReferenceMap.Empty();
	TestActor->ReferenceMapKey.Empty();

	const auto BY_PROPERTY = FBlueprintComponentReference::ForProperty(ABCRCachedTestActor::MeshPropertyName);
	const auto BY_PATH = FBlueprintComponentReference::ForPath(ABCRCachedTestActor::MeshComponentName);

	//======================================
	PerfRunner_Single<100>{ TestActor, BY_PROPERTY }.Run();
	PerfRunner_Single<1000>{ TestActor, BY_PROPERTY }.Run();
	PerfRunner_Single<10000>{ TestActor, BY_PROPERTY }.Run();
	PerfRunner_Single<100000>{ TestActor, BY_PROPERTY }.Run();
	PerfRunner_Single<1000000>{ TestActor, BY_PROPERTY }.Run();
	//======================================
	PerfRunner_Single<100>{ TestActor, BY_PATH }.Run();
	PerfRunner_Single<1000>{ TestActor, BY_PATH }.Run();
	PerfRunner_Single<10000>{ TestActor, BY_PATH }.Run();
	PerfRunner_Single<100000>{ TestActor, BY_PATH }.Run();
	PerfRunner_Single<1000000>{ TestActor, BY_PATH }.Run();
	//======================================
	PerfRunner_Array<1, 100> { TestActor, BY_PROPERTY }.Run();
	PerfRunner_Array<1, 1000> { TestActor, BY_PROPERTY }.Run();
	PerfRunner_Array<1, 10000> { TestActor, BY_PROPERTY }.Run();
	PerfRunner_Array<1, 100000> { TestActor, BY_PROPERTY }.Run();
	PerfRunner_Array<1, 1000000> { TestActor, BY_PROPERTY }.Run();

	PerfRunner_Array<10, 100> { TestActor, BY_PROPERTY }.Run();
	PerfRunner_Array<10, 1000> { TestActor, BY_PROPERTY }.Run();
	PerfRunner_Array<10, 10000> { TestActor, BY_PROPERTY }.Run();
	PerfRunner_Array<10, 100000> { TestActor, BY_PROPERTY }.Run();
	PerfRunner_Array<10, 1000000> { TestActor, BY_PROPERTY }.Run();

	PerfRunner_Array<100, 100> { TestActor, BY_PROPERTY }.Run();
	PerfRunner_Array<100, 1000> { TestActor, BY_PROPERTY }.Run();
	PerfRunner_Array<100, 10000> { TestActor, BY_PROPERTY }.Run();
	PerfRunner_Array<100, 100000> { TestActor, BY_PROPERTY }.Run();
	PerfRunner_Array<100, 1000000> { TestActor, BY_PROPERTY }.Run();
	//======================================
	PerfRunner_Array<1, 100> { TestActor, BY_PATH }.Run();
	PerfRunner_Array<1, 1000> { TestActor, BY_PATH }.Run();
	PerfRunner_Array<1, 10000> { TestActor, BY_PATH }.Run();
	PerfRunner_Array<1, 100000> { TestActor, BY_PATH }.Run();
	PerfRunner_Array<1, 1000000> { TestActor, BY_PATH }.Run();

	PerfRunner_Array<10, 100> { TestActor, BY_PATH }.Run();
	PerfRunner_Array<10, 1000> { TestActor, BY_PATH }.Run();
	PerfRunner_Array<10, 10000> { TestActor, BY_PATH }.Run();
	PerfRunner_Array<10, 100000> { TestActor, BY_PATH }.Run();
	PerfRunner_Array<10, 1000000> { TestActor, BY_PATH }.Run();

	PerfRunner_Array<100, 100> { TestActor, BY_PATH }.Run();
	PerfRunner_Array<100, 1000> { TestActor, BY_PATH }.Run();
	PerfRunner_Array<100, 10000> { TestActor, BY_PATH }.Run();
	PerfRunner_Array<100, 100000> { TestActor, BY_PATH }.Run();
	PerfRunner_Array<100, 1000000> { TestActor, BY_PATH }.Run();
	//======================================
	PerfRunner_MapKey<10, 1, 100> { TestActor }.Run();
	PerfRunner_MapKey<10, 1, 1000> { TestActor }.Run();
	PerfRunner_MapKey<10, 1, 10000> { TestActor }.Run();
	PerfRunner_MapKey<10, 1, 100000> { TestActor }.Run();
	PerfRunner_MapKey<10, 1, 1000000> { TestActor }.Run();
	
	PerfRunner_MapKey<100, 10, 100> { TestActor }.Run();
	PerfRunner_MapKey<100, 10, 1000> { TestActor }.Run();
	PerfRunner_MapKey<100, 10, 10000> { TestActor }.Run();
	PerfRunner_MapKey<100, 10, 100000> { TestActor }.Run();
	PerfRunner_MapKey<100, 10, 1000000> { TestActor }.Run();

	PerfRunner_MapKey<100, 50, 100> { TestActor }.Run();
	PerfRunner_MapKey<100, 50, 1000> { TestActor }.Run();
	PerfRunner_MapKey<100, 50, 10000> { TestActor }.Run();
	PerfRunner_MapKey<100, 50, 100000> { TestActor }.Run();
	PerfRunner_MapKey<100, 50, 1000000> { TestActor }.Run();
	
	return true;
}

#endif
