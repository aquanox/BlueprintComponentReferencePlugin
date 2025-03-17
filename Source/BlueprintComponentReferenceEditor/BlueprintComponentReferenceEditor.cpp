// Copyright 2024, Aquanox.

#include "BlueprintComponentReferenceEditor.h"
#include "BlueprintComponentReferenceCustomization.h"
#include "BlueprintComponentReferenceVarCustomization.h"
#include "BlueprintEditorModule.h"
#include "HAL/IConsoleManager.h"
#include "UnrealEdGlobals.h"
#include "Editor/EditorEngine.h"

IMPLEMENT_MODULE(FBCREditorModule, BlueprintComponentReferenceEditor);

DEFINE_LOG_CATEGORY(LogComponentReferenceEditor);

#if ALLOW_CONSOLE

static FAutoConsoleCommand BCR_DumpInstances(
	TEXT("BCR.DumpInstances"),
	TEXT("Dump active instance data"),
	FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& InArgs) {
		FBCREditorModule::GetReflectionHelper()->DebugDumpInstances(InArgs);
	})
);
static FAutoConsoleCommand BCR_DumpClasses(
	TEXT("BCR.DumpClasses"),
	TEXT("Dump active class data"),
	FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& InArgs) {
		FBCREditorModule::GetReflectionHelper()->DebugDumpClasses(InArgs);
	})
);
static FAutoConsoleCommand BCR_DumpContexts(
	TEXT("BCR.DumpContexts"),
	TEXT("Dump active contexts data"),
	FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& InArgs) {
		FBCREditorModule::GetReflectionHelper()->DebugDumpContexts(InArgs);
	})
);
static FAutoConsoleCommand BCR_ForceCleanup(
	TEXT("BCR.ForceCleanup"),
	TEXT("Force cleanup stale data"),
	FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& InArgs) {
		FBCREditorModule::GetReflectionHelper()->DebugForceCleanup();
	})
);
static FAutoConsoleCommand BCR_EnableLogging(
	TEXT("BCR.EnableLogging"),
	TEXT("Enable BCR debug logging"),
	FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& InArgs) {
		LogComponentReferenceEditor.SetVerbosity(ELogVerbosity::VeryVerbose);
		UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("Enabled BCR debug logging"));
	})
);

#endif

void FBCREditorModule::StartupModule()
{
	if (GIsEditor && !IsRunningCommandlet())
	{
		ClassHelper = MakeShared<FBlueprintComponentReferenceHelper>();

		PostEngineInitHandle = FCoreDelegates::OnPostEngineInit.AddRaw(this, &FBCREditorModule::OnPostEngineInit);
	}
}

void FBCREditorModule::OnPostEngineInit()
{
	FCoreDelegates::OnPostEngineInit.Remove(PostEngineInitHandle);

	OnReloadCompleteDelegateHandle = FCoreUObjectDelegates::ReloadCompleteDelegate.AddRaw(this, &FBCREditorModule::OnReloadComplete);
	OnReloadReinstancingCompleteDelegateHandle = FCoreUObjectDelegates::ReloadReinstancingCompleteDelegate.AddRaw(this, &FBCREditorModule::OnReinstancingComplete);
	OnModulesChangedDelegateHandle = FModuleManager::Get().OnModulesChanged().AddRaw(this, &FBCREditorModule::OnModulesChanged);
	OnBlueprintCompiledHandle = GEditor->OnBlueprintCompiled().AddRaw(this, &FBCREditorModule::OnBlueprintRecompile);

	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomPropertyTypeLayout(
		"BlueprintComponentReference",
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FBlueprintComponentReferenceCustomization::MakeInstance));

	FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::GetModuleChecked<FBlueprintEditorModule>("Kismet");
	VariableCustomizationHandle = BlueprintEditorModule.RegisterVariableCustomization(
		FProperty::StaticClass(),
		FOnGetVariableCustomizationInstance::CreateStatic(&FBlueprintComponentReferenceVarCustomization::MakeInstance));
}

void FBCREditorModule::ShutdownModule()
{
	if (GIsEditor && !IsRunningCommandlet())
	{
		FCoreDelegates::OnPostEngineInit.Remove(PostEngineInitHandle);
		FCoreUObjectDelegates::ReloadCompleteDelegate.Remove(OnReloadCompleteDelegateHandle);
		FCoreUObjectDelegates::ReloadReinstancingCompleteDelegate.Remove(OnReloadReinstancingCompleteDelegateHandle);
		FModuleManager::Get().OnModulesChanged().Remove(OnModulesChangedDelegateHandle);

		if (GEditor)
		{
			GEditor->OnBlueprintCompiled().Remove(OnBlueprintCompiledHandle);
		}

		if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
		{
			FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
			PropertyModule.UnregisterCustomPropertyTypeLayout("BlueprintComponentReference");
		}

		if (FModuleManager::Get().IsModuleLoaded("Kismet"))
		{
			FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::GetModuleChecked<FBlueprintEditorModule>("Kismet");
			BlueprintEditorModule.UnregisterVariableCustomization(FProperty::StaticClass(), VariableCustomizationHandle);
		}
	}
}

TSharedPtr<FBlueprintComponentReferenceHelper> FBCREditorModule::GetReflectionHelper()
{
	static const FName ModuleName("BlueprintComponentReferenceEditor");
	auto& Ref = FModuleManager::LoadModuleChecked<FBCREditorModule>(ModuleName).ClassHelper;
	if (!Ref.IsValid())
	{
		Ref =  MakeShared<FBlueprintComponentReferenceHelper>();
	}
	return Ref;
}

void FBCREditorModule::OnReloadComplete(EReloadCompleteReason ReloadCompleteReason)
{
	UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("OnReloadComplete"));
	if (ClassHelper)
	{
		ClassHelper->CleanupStaleData();
		ClassHelper->MarkBlueprintCacheDirty();
	}
}

void FBCREditorModule::OnReinstancingComplete()
{
	UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("OnReinstancingComplete"));
	if (ClassHelper)
	{
		ClassHelper->CleanupStaleData();
		//ClassHelper->MarkBlueprintCacheDirty();
	}
}

void FBCREditorModule::OnModulesChanged(FName Name, EModuleChangeReason ModuleChangeReason)
{
	UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("OnModulesChanged"));
	if (ClassHelper)
	{
		ClassHelper->CleanupStaleData();
		//ClassHelper->MarkBlueprintCacheDirty();
	}
}

void FBCREditorModule::OnBlueprintRecompile()
{
	UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("OnBlueprintRecompile"));
	if (ClassHelper)
	{
		ClassHelper->CleanupStaleData();
		ClassHelper->MarkBlueprintCacheDirty();
	}
}
