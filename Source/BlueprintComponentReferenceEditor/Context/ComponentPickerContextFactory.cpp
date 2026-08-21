#include "ComponentPickerContextFactory.h"

#include "HAL/IConsoleManager.h"
#include "BlueprintComponentReferenceEditor.h"

static FAutoConsoleCommand BCR_DumpInstances(
	TEXT("BCR.DumpInstances"),
	TEXT("Dump active instance data"),
	FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& InArgs) {
		FBCREditorModule::GetContextFactory()->HandleCommand("DebugDumpInstances", InArgs);
	})
);
static FAutoConsoleCommand BCR_DumpClasses(
	TEXT("BCR.DumpClasses"),
	TEXT("Dump active class data"),
	FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& InArgs) {
		FBCREditorModule::GetContextFactory()->HandleCommand("DebugDumpClasses", InArgs);
	})
);
static FAutoConsoleCommand BCR_DumpContexts(
	TEXT("BCR.DumpContexts"),
	TEXT("Dump active contexts data"),
	FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& InArgs) {
		FBCREditorModule::GetContextFactory()->HandleCommand("DebugDumpContexts", InArgs);
	})
);
static FAutoConsoleCommand BCR_ForceCleanup(
	TEXT("BCR.ForceCleanup"),
	TEXT("Force cleanup stale data"),
	FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& InArgs) {
		FBCREditorModule::GetContextFactory()->HandleCommand("DebugForceCleanup", InArgs);
	})
);

