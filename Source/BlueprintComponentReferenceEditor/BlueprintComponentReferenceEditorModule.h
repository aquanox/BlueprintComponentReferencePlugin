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

private:
	TSharedPtr<FBlueprintComponentReferenceHelper> ClassHelper;
};

