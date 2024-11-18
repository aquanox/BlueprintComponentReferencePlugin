// Copyright 2024, Aquanox.

#pragma once

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

class FBlueprintComponentReferenceHelper;

struct FBCREditorModule : public IModuleInterface
{
	static TSharedPtr<FBlueprintComponentReferenceHelper> GetReflectionHelper();

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	virtual bool SupportsDynamicReloading() override { return false; }

	void OnPostEngineInit();
private:
	TSharedPtr<FBlueprintComponentReferenceHelper> ClassHelper;

	FDelegateHandle VariableCustomizationHandle;
	FDelegateHandle PostEngineInitHandle;
};

#if UE_BUILD_DEBUG
DECLARE_LOG_CATEGORY_EXTERN(LogComponentReferenceEditor, Log, All);
#else
DECLARE_LOG_CATEGORY_EXTERN(LogComponentReferenceEditor, Warning, All);
#endif
