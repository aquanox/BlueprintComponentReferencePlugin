// Copyright 2024, Aquanox.

#include "BlueprintComponentReferenceEditorModule.h"
#include "BlueprintComponentReferenceCustomization.h"

static const FName TypeName = TEXT("BlueprintComponentReference");


IMPLEMENT_MODULE(FBCREditorModule, BlueprintComponentReferenceEditor);

FBCREditorModule& FBCREditorModule::Get()
{
	return FModuleManager::GetModuleChecked<FBCREditorModule>("BlueprintComponentReferenceEditor");
}

void FBCREditorModule::StartupModule()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));

	PropertyModule.RegisterCustomPropertyTypeLayout(TypeName,
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(
				&FBlueprintComponentReferenceCustomization::MakeInstance));

	ClassHelper = MakeShared<FBlueprintComponentReferenceHelper>();
}

void FBCREditorModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded(TEXT("PropertyEditor")))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
		PropertyModule.UnregisterCustomPropertyTypeLayout(TypeName);
	}
}

TSharedPtr<FBlueprintComponentReferenceHelper> FBCREditorModule::GetClassHelper() const
{
	return ClassHelper;
}
