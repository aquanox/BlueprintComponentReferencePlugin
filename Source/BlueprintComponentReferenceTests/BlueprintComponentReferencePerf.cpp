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
#include "CachedBlueprintComponentReference.h"

#if WITH_CACHED_COMPONENT_REFERENCE_TESTS && WITH_DEV_AUTOMATION_TESTS

void EnsureTemplatesCompile(ABCRCachedTestActor* Target)
{
	check(!"This is just to ensure template code compiles during autobuild");
	
	AActor* PtrToActor = nullptr;
	USceneComponent* PtrToComponent = nullptr;
	FName None("SuperKey");

	Target->CachedReferenceSingle.Get();
	Target->CachedReferenceSingle.Get(PtrToActor);
	Target->CachedReferenceSingle.Invalidate();
	Target->CachedReferenceSingle.Get(PtrToActor);
	Target->CachedReferenceArray.Get(0);
	Target->CachedReferenceArray.Get(PtrToActor, 0);
	Target->CachedReferenceArray.Num();
	Target->CachedReferenceArray.IsEmpty();
	Target->CachedReferenceArray.Invalidate();
	Target->CachedReferenceArray.Get(PtrToActor, 0);
	Target->CachedReferenceMap.Get(None);
	Target->CachedReferenceMap.Get(PtrToActor, None);
	Target->CachedReferenceMap.Invalidate();
	Target->CachedReferenceMap.Get(PtrToActor, NAME_None);
	Target->CachedReferenceMapKey.Get(PtrToComponent);
	Target->CachedReferenceMapKey.Get(PtrToActor, PtrToComponent);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlueprintComponentReferenceTests_Cached,
	"BlueprintComponentReference.Cached", EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter | EAutomationTestFlags::HighPriority);

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

	TCachedComponentReference<USceneComponent, BCRDetails::TRawPointerFuncs> CachedStrong;
	
	TCachedComponentReference<USceneComponent, BCRDetails::TWeakPointerFuncs> CachedWeak;
	
	TCachedComponentReference<USceneComponent, BCRDetails::TWeakPointerFuncs> CachedWarm;
	
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
			CachedWarm.Get(Actor);
			
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

	TCachedComponentReferenceArray<USceneComponent, BCRDetails::TRawPointerFuncs> CachedStrong;
	TCachedComponentReferenceArray<USceneComponent, BCRDetails::TWeakPointerFuncs> CachedWeak;
	TCachedComponentReferenceArray<USceneComponent, BCRDetails::TWeakPointerFuncs> CachedWarm;
	
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
			CachedWarm.GetAll(Actor);
			
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
	TCachedComponentReferenceMapKey<UActorComponent, int32> CachedMap;
	TCachedComponentReferenceMapKey<UActorComponent, int32> CachedMapWarm;

	TArray<UActorComponent*> Components;
	TArray<UActorComponent*> ComponentsInUse;
	TArray<UActorComponent*> RefAccessSequence;
	
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
			UActorComponent* Target = Components[RIdx];

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
			for (const UActorComponent* Index : RefAccessSequence)
			{
				CachedMap.Get(Actor, Index);
			}
		}
		{
			CachedMapWarm.GetAll(Actor);
			
			FScopeLogTime Scope(*GenerateDescription(TEXT("Warm")), nullptr, FConditionalScopeLogTime::ScopeLog_Milliseconds);
			for (const UActorComponent* Index : RefAccessSequence)
			{
				CachedMapWarm.Get(Actor, Index);
			}
		}
	}
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlueprintComponentReferenceTests_Perf,
	"BlueprintComponentReference.Perf", EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter);

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

#endif // WITH_CACHED_COMPONENT_REFERENCE_TESTS
