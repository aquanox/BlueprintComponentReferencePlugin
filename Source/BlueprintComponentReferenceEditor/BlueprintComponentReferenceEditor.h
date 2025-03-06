// Copyright 2024, Aquanox.

#pragma once

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

class FBlueprintComponentReferenceHelper;
enum class EReloadCompleteReason;

struct FBCREditorModule : public IModuleInterface
{
	static TSharedPtr<FBlueprintComponentReferenceHelper> GetReflectionHelper();

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	virtual bool SupportsDynamicReloading() override { return false; }

private:
	void OnPostEngineInit();
	void OnReloadComplete(EReloadCompleteReason ReloadCompleteReason);
	void OnReloadAddedClasses(const TArray<UClass*>& AddedClasses);
	void OnReinstancingComplete();
	void OnModulesChanged(FName Name, EModuleChangeReason ModuleChangeReason);
private:
	TSharedPtr<FBlueprintComponentReferenceHelper> ClassHelper;

	FDelegateHandle VariableCustomizationHandle;
	FDelegateHandle PostEngineInitHandle;
	
	FDelegateHandle OnReloadCompleteDelegateHandle;
	FDelegateHandle OnReloadAddedClassesDelegateHandle;
	FDelegateHandle OnReloadReinstancingCompleteDelegateHandle;
	FDelegateHandle OnModulesChangedDelegateHandle;
};

DECLARE_LOG_CATEGORY_EXTERN(LogComponentReferenceEditor, Log, All);
