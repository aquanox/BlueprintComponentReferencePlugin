// Copyright 2024, Aquanox.

#pragma once

#include "BlueprintComponentReference.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BlueprintComponentReferenceUtils.generated.h"

/**
 * Various helper functions to interact with blueprint components
 */
UCLASS(MinimalAPI)
class UBlueprintComponentReferenceUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable, meta=( DefaultToSelf="Actor"))
	static UActorComponent* ResolveComponentReference(const FBlueprintComponentReference& Reference, AActor* Actor);

	UFUNCTION(BlueprintCallable, meta=(DefaultToSelf="Actor", DeterminesOutputType="Class", DynamicOutputParam="Component"))
	static UPARAM(DisplayName="Success") void TryResolveComponentReference(const FBlueprintComponentReference& Reference, AActor* Actor, TSubclassOf<UActorComponent> Class, UActorComponent*& Component);

	UFUNCTION(BlueprintCallable)
	static void SetComponentReference_FromName(UPARAM(Ref) FBlueprintComponentReference& Reference, AActor* Actor, FName PropertyName);

	UFUNCTION(BlueprintCallable)
	static void SetComponentReference_FromComponent(UPARAM(Ref) FBlueprintComponentReference& Reference,UActorComponent* Component);
};

/**
 * Internal struct for blueprint property configuration
 */
USTRUCT(NotBlueprintable, NotBlueprintType, meta=(HiddenByDefault))
struct FBlueprintComponentReferenceExtras
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category=MetaData)
	bool bShowNative = true;
	UPROPERTY(EditAnywhere, Category=MetaData)
	bool bShowBlueprint = true;
	UPROPERTY(EditAnywhere, Category=MetaData)
	bool bShowInstanced = false;
	UPROPERTY(EditAnywhere, Category=MetaData)
	bool bShowPathOnly = false;

	//UPROPERTY(EditAnywhere, Category=MetaData)
	//bool bAllowPicker	= true;
	//UPROPERTY(EditAnywhere, Category=MetaData)
	//bool bAllowNavigate = true;
	//UPROPERTY(EditAnywhere, Category=MetaData)
	//bool bAllowClear	= true;

	UPROPERTY(EditAnywhere, Category=MetaData, meta=(DisplayThumbnail=false, NoBrowse, NoCreate))
	TArray<TSoftClassPtr<UActorComponent>>	AllowedClasses;
	UPROPERTY(EditAnywhere, Category=MetaData, meta=(DisplayThumbnail=false, NoBrowse, NoCreate))
	TArray<TSoftClassPtr<UActorComponent>>	DisallowedClasses;
	//UPROPERTY(EditAnywhere, Category=MetaData, meta=(DisplayThumbnail=false, NoBrowse, NoCreate))
	//TArray<TSoftClassPtr<UInterface>>		RequiredInterfaces;
};
