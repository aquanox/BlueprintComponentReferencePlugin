// Copyright 2024, Aquanox.

#pragma once

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

class FBlueprintComponentReferenceHelper;

struct FBCREditorModule : public IModuleInterface
{
	static FBCREditorModule& Get();

	TSharedPtr<FBlueprintComponentReferenceHelper> GetClassHelper() const;

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void OnPostEngineInit();
private:
	TSharedPtr<FBlueprintComponentReferenceHelper> ClassHelper;

	FDelegateHandle VariableCustomizationHandle;
	FDelegateHandle PostEngineInitHandle;
};

DECLARE_LOG_CATEGORY_EXTERN(LogComponentReferenceEditor, Log, All);
