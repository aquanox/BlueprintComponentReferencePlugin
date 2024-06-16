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
