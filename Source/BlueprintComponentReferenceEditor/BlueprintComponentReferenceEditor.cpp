// Copyright 2024, Aquanox.

#include "BlueprintComponentReferenceEditor.h"
#include "BlueprintComponentReferenceCustomization.h"
#include "BlueprintComponentReferenceVarCustomization.h"
#include "BlueprintEditorModule.h"

IMPLEMENT_MODULE(FBCREditorModule, BlueprintComponentReferenceEditor);

void FBCREditorModule::StartupModule()
{
	ClassHelper = MakeShared<FBlueprintComponentReferenceHelper>();

	PostEngineInitHandle = FCoreDelegates::OnPostEngineInit.AddRaw(this, &FBCREditorModule::OnPostEngineInit);
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
	FCoreDelegates::OnPostEngineInit.Remove(PostEngineInitHandle);

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

TSharedPtr<FBlueprintComponentReferenceHelper> FBCREditorModule::GetReflectionHelper()
{
	static const FName ModuleName("BlueprintComponentReferenceEditor");
	return FModuleManager::LoadModuleChecked<FBCREditorModule>(ModuleName).ClassHelper;
}
