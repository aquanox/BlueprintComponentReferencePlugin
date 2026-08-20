// Copyright 2024, Aquanox.

#pragma once

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorDelegates.h"

class FBlueprintComponentReferenceCustomization;
class FBlueprintComponentReferenceHelper;
class FPropertyEditorModule;
class IPropertyTypeCustomization;
enum class EReloadCompleteReason;

class BLUEPRINTCOMPONENTREFERENCEEDITOR_API FBCREditorModule : public IModuleInterface
{
public:
	static bool IsAvailable();
	static FBCREditorModule& Get();

	static TSharedPtr<FBlueprintComponentReferenceHelper> GetReflectionHelper();

	/**
	 * A generic helper for registering component reference type with customization
	 *
	 * @tparam TReference Child of FBlueprintComponentReference type
	 * @tparam TCustomization Child of FBlueprintComponentReferenceCustomization type
	 * @tparam bExactType Should restrict to exact TReference type
	 */
	template<typename TReference, typename TCustomization = FBlueprintComponentReferenceCustomization, bool bExactType = true>
	static void RegisterComponentReferenceType()
	{
		using FIsSupportedStructFilter = TDelegate<bool(const UStruct*)>;

		const FName TypeName = TReference::StaticStruct()->GetFName();

		RegisterComponentRereferenceType(TypeName, FOnGetPropertyTypeCustomizationInstance::CreateLambda([]() -> TSharedRef<IPropertyTypeCustomization>
		{
			if constexpr (bExactType)
			{
				return MakeShared<TCustomization>(FIsSupportedStructFilter::CreateLambda([](const UScriptStruct* InType) -> bool
				{
					return InType == TReference::StaticStruct();
				}));
			}
			else
			{
				return MakeShared<TCustomization>();
			}
		}));
	}

	/**
	 * Register component reference type with customization.
	 *
	 * @param Name Struct name
	 * @param Provider Customization instance factory
	 */
	static void RegisterComponentRereferenceType(FName Name, FOnGetPropertyTypeCustomizationInstance Provider);

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	virtual bool SupportsDynamicReloading() override { return false; }

private:
	void OnPostEngineInit();
	void OnPostEngineInit_ProcessPendingRegs();

	void OnReloadComplete(EReloadCompleteReason ReloadCompleteReason);
	void OnReinstancingComplete();
	void OnModulesChanged(FName Name, EModuleChangeReason ModuleChangeReason);
	void OnBlueprintRecompile();
private:
	TSharedPtr<FBlueprintComponentReferenceHelper> ClassHelper;

	using FPendingRegistrationFn = TFunction<FName(FPropertyEditorModule&)>;
	TMap<FName, FOnGetPropertyTypeCustomizationInstance> PendingRegistrations;
	TArray<FName> RegisteredTypes;

	FDelegateHandle VariableCustomizationHandle;
	FDelegateHandle PostEngineInitHandle;
	bool bPostEngineInitComplete = false;

	FDelegateHandle OnReloadCompleteDelegateHandle;
	FDelegateHandle OnReloadReinstancingCompleteDelegateHandle;
	FDelegateHandle OnModulesChangedDelegateHandle;
	FDelegateHandle OnBlueprintCompiledHandle;
};

DECLARE_LOG_CATEGORY_EXTERN(LogComponentReferenceEditor, Log, All);
