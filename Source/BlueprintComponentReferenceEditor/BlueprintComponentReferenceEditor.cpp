// Copyright 2024, Aquanox.

#include "BlueprintComponentReferenceEditor.h"
#include "BlueprintComponentReferenceCustomization.h"
#include "BlueprintComponentReferenceVarCustomization.h"
#include "BlueprintEditorModule.h"
#include "HAL/IConsoleManager.h"

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

		OnReloadCompleteDelegateHandle = FCoreUObjectDelegates::ReloadCompleteDelegate.AddRaw(this, &FBCREditorModule::OnReloadComplete);
		OnReloadAddedClassesDelegateHandle = FCoreUObjectDelegates::ReloadAddedClassesDelegate.AddRaw(this, &FBCREditorModule::OnReloadAddedClasses);
		OnReloadReinstancingCompleteDelegateHandle = FCoreUObjectDelegates::ReloadReinstancingCompleteDelegate.AddRaw(this, &FBCREditorModule::OnReinstancingComplete);
		OnModulesChangedDelegateHandle = FModuleManager::Get().OnModulesChanged().AddRaw(this, &FBCREditorModule::OnModulesChanged);
	}
}

void FBCREditorModule::OnPostEngineInit()
{
	FCoreDelegates::OnPostEngineInit.Remove(PostEngineInitHandle);

	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
	PropertyModule.RegisterCustomPropertyTypeLayout(
		FName("BlueprintComponentReference"),
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
		FCoreUObjectDelegates::ReloadAddedClassesDelegate.Remove(OnReloadAddedClassesDelegateHandle);
		FCoreUObjectDelegates::ReloadReinstancingCompleteDelegate.Remove(OnReloadReinstancingCompleteDelegateHandle);
		FModuleManager::Get().OnModulesChanged().Remove(OnModulesChangedDelegateHandle);

		if (FModuleManager::Get().IsModuleLoaded(TEXT("PropertyEditor")))
		{
			FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
			PropertyModule.UnregisterCustomPropertyTypeLayout(FName("BlueprintComponentReference"));
		}

		if (FModuleManager::Get().IsModuleLoaded(TEXT("Kismet")))
		{
			FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::GetModuleChecked<FBlueprintEditorModule>("Kismet");
			BlueprintEditorModule.UnregisterVariableCustomization(FProperty::StaticClass(), VariableCustomizationHandle);
		}
	}
}

TSharedPtr<FBlueprintComponentReferenceHelper> FBCREditorModule::GetReflectionHelper()
{
	static const FName ModuleName("BlueprintComponentReferenceEditor");
	return FModuleManager::LoadModuleChecked<FBCREditorModule>(ModuleName).ClassHelper;
}

void FBCREditorModule::OnReloadComplete(EReloadCompleteReason ReloadCompleteReason)
{
	UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("OnReloadComplete"));
	if (ClassHelper.IsValid())
	{
		ClassHelper->CleanupStaleData(true);
	}
}

void FBCREditorModule::OnReloadAddedClasses(const TArray<UClass*>& AddedClasses)
{
	UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("OnReloadAddedClasses"));
	if (ClassHelper.IsValid())
	{
		ClassHelper->CleanupStaleData(true);
	}
}

void FBCREditorModule::OnReinstancingComplete()
{
	UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("OnReinstancingComplete"));
	if (ClassHelper.IsValid())
	{
		ClassHelper->CleanupStaleData(true);
	}
}

void FBCREditorModule::OnModulesChanged(FName Name, EModuleChangeReason ModuleChangeReason)
{
	UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("OnModulesChanged"));
	if (ClassHelper.IsValid())
	{
		ClassHelper->CleanupStaleData(true);
	}
}
