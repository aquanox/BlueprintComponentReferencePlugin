// Copyright 2024, Aquanox.

#include "BlueprintComponentReferenceEditor.h"
#include "BlueprintComponentReferenceCustomization.h"
#include "BlueprintComponentReferenceCustomizationExtras.h"
#include "MetadataCore/MetadataEditorVarCustomization.h"
#include "BlueprintEditorModule.h"
#include "HAL/IConsoleManager.h"
#include "UnrealEdGlobals.h"
#include "Misc/EngineVersionComparison.h"
#include "Editor/EditorEngine.h"
#include "Context/ComponentPickerContext.h"
#include "Context/ComponentPickerContextFactory.h"

IMPLEMENT_MODULE(FBCREditorModule, BlueprintComponentReferenceEditor);

DEFINE_LOG_CATEGORY(LogComponentReferenceEditor);

namespace
{
	static const FName BCRModuleName("BlueprintComponentReferenceEditor");

	using FContextFactoryImpl = FLegacyContextFactory;
}

static FAutoConsoleCommand BCR_EnableLogging(
	TEXT("BCR.EnableLogging"),
	TEXT("Enable BCR debug logging"),
	FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& InArgs) {
		LogComponentReferenceEditor.SetVerbosity(ELogVerbosity::VeryVerbose);
		UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("Enabled BCR debug logging"));
	})
);

void FBCREditorModule::StartupModule()
{
	if (GIsEditor && !IsRunningCommandlet())
	{
		ContextFactory = MakeShared<FContextFactoryImpl>();

#if UE_VERSION_OLDER_THAN(5, 8, 0)
		PostEngineInitHandle = FCoreDelegates::OnPostEngineInit.AddRaw(this, &FBCREditorModule::OnPostEngineInit);
#else
		PostEngineInitHandle = FCoreDelegates::GetOnPostEngineInit().AddRaw(this, &FBCREditorModule::OnPostEngineInit);
#endif
	}
}

void FBCREditorModule::OnPostEngineInit()
{
#if UE_VERSION_OLDER_THAN(5, 8, 0)
	FCoreDelegates::OnPostEngineInit.Remove(PostEngineInitHandle);
#else
	FCoreDelegates::GetOnPostEngineInit().Remove(PostEngineInitHandle);
#endif

#if !UE_VERSION_OLDER_THAN(5, 0, 0)
	OnReloadCompleteDelegateHandle = FCoreUObjectDelegates::ReloadCompleteDelegate.AddRaw(this, &FBCREditorModule::OnReloadComplete);
	OnReloadReinstancingCompleteDelegateHandle = FCoreUObjectDelegates::ReloadReinstancingCompleteDelegate.AddRaw(this, &FBCREditorModule::OnReinstancingComplete);
#endif
	OnModulesChangedDelegateHandle = FModuleManager::Get().OnModulesChanged().AddRaw(this, &FBCREditorModule::OnModulesChanged);
	OnBlueprintCompiledHandle = GEditor->OnBlueprintCompiled().AddRaw(this, &FBCREditorModule::OnBlueprintRecompile);

	FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::GetModuleChecked<FBlueprintEditorModule>("Kismet");
#if UE_VERSION_OLDER_THAN(5, 0, 0)
	BlueprintEditorModule.RegisterVariableCustomization(
		FProperty::StaticClass(),
		FOnGetVariableCustomizationInstance::CreateStatic(&FMetadataEditorVarCustomization::MakeInstance));
#else
	VariableCustomizationHandle = BlueprintEditorModule.RegisterVariableCustomization(
		FProperty::StaticClass(),
		FOnGetVariableCustomizationInstance::CreateStatic(&FMetadataEditorVarCustomization::MakeInstance));
#endif

	// register default types before resetting PostEngineInitHandle
	RegisterComponentReferenceType<FBlueprintComponentReference, FBlueprintComponentReferenceCustomization, false>();

#if defined(WITH_BCR_EXTRAS) && WITH_BCR_EXTRAS
	// register extras
	RegisterComponentReferenceType<FSceneComponentReference, FBlueprintComponentReferenceCustomization, false>();
	RegisterComponentReferenceType<FPrimitiveComponentReference, FBlueprintComponentReferenceCustomization, false>();
	RegisterComponentReferenceType<FMeshComponentReference, FBlueprintComponentReferenceCustomization, false>();
	RegisterComponentReferenceType<FMeshSocketReference, FMeshSocketReferenceCustomization, false>();
#endif

	// process pending layout registrations
	OnPostEngineInit_ProcessPendingRegs();

	//
	bPostEngineInitComplete = true;
}

void FBCREditorModule::RegisterComponentRereferenceType(FName Name, FOnGetPropertyTypeCustomizationInstance Provider)
{
	// External calls to Register may be invoked from early loading phases before EngineInit, so need to defer the tasks
	// after engine is initialized it is safe to register type right away
	if (GIsEditor && !IsRunningCommandlet())
	{
		FBCREditorModule& Module = FBCREditorModule::Get();
		Module.PendingRegistrations.Emplace(Name, MoveTemp(Provider));

		if (Module.bPostEngineInitComplete)
		{ // init already happened, process right away
			Module.OnPostEngineInit_ProcessPendingRegs();
		}
	}
}

void FBCREditorModule::OnPostEngineInit_ProcessPendingRegs()
{
	if (PendingRegistrations.Num())
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		for (const auto& Pair : PendingRegistrations)
		{
			PropertyModule.RegisterCustomPropertyTypeLayout(Pair.Key, Pair.Value);
			RegisteredTypes.Add(Pair.Key);
		}
		PendingRegistrations.Empty();
		PropertyModule.NotifyCustomizationModuleChanged();
	}
}

void FBCREditorModule::ShutdownModule()
{
	if (GIsEditor && !IsRunningCommandlet())
	{
		bPostEngineInitComplete = false;

#if UE_VERSION_OLDER_THAN(5, 8, 0)
		FCoreDelegates::OnPostEngineInit.Remove(PostEngineInitHandle);
#else
		FCoreDelegates::GetOnPostEngineInit().Remove(PostEngineInitHandle);
#endif

#if !UE_VERSION_OLDER_THAN(5, 0, 0)
		FCoreUObjectDelegates::ReloadCompleteDelegate.Remove(OnReloadCompleteDelegateHandle);
		FCoreUObjectDelegates::ReloadReinstancingCompleteDelegate.Remove(OnReloadReinstancingCompleteDelegateHandle);
#endif
		FModuleManager::Get().OnModulesChanged().Remove(OnModulesChangedDelegateHandle);

		if (GEditor)
		{
			GEditor->OnBlueprintCompiled().Remove(OnBlueprintCompiledHandle);
		}

		if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
		{
			FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
			//PropertyModule.UnregisterCustomPropertyTypeLayout("BlueprintComponentReference");
			for (const FName& TypeName : RegisteredTypes)
			{
				PropertyModule.UnregisterCustomPropertyTypeLayout(TypeName);
			}
			RegisteredTypes.Empty();
		}

		if (FModuleManager::Get().IsModuleLoaded("Kismet"))
		{
			FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::GetModuleChecked<FBlueprintEditorModule>("Kismet");
#if UE_VERSION_OLDER_THAN(5, 0, 0)
			BlueprintEditorModule.UnregisterVariableCustomization(FProperty::StaticClass());
#else
			BlueprintEditorModule.UnregisterVariableCustomization(FProperty::StaticClass(), VariableCustomizationHandle);
#endif
		}
	}
}

bool FBCREditorModule::IsAvailable()
{
	return FModuleManager::Get().IsModuleLoaded(BCRModuleName);
}

FBCREditorModule& FBCREditorModule::Get()
{
	return FModuleManager::GetModuleChecked<FBCREditorModule>(BCRModuleName);
}

TSharedRef<FComponentPickerContextFactory> FBCREditorModule::GetContextFactory()
{
	auto& BuilderPtr = FBCREditorModule::Get().ContextFactory;
	if (!BuilderPtr.IsValid())
	{
		BuilderPtr =  MakeShared<FContextFactoryImpl>();
	}
	return BuilderPtr.ToSharedRef();
}

void FBCREditorModule::OnReloadComplete(EReloadCompleteReason ReloadCompleteReason)
{
	UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("OnReloadComplete"));
	if (ContextFactory)
	{
		ContextFactory->ValidateCache(true);
	}
}

void FBCREditorModule::OnReinstancingComplete()
{
	UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("OnReinstancingComplete"));
	if (ContextFactory)
	{
		ContextFactory->ValidateCache(false);
	}
}

void FBCREditorModule::OnModulesChanged(FName Name, EModuleChangeReason ModuleChangeReason)
{
	UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("OnModulesChanged"));
	if (ContextFactory)
	{
		ContextFactory->ValidateCache(false);
	}
}

void FBCREditorModule::OnBlueprintRecompile()
{
	UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("OnBlueprintRecompile"));
	if (ContextFactory)
	{
		ContextFactory->ValidateCache(true);
	}
}
