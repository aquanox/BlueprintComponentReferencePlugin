#pragma once

#include "BlueprintComponentReference.h"
#include "BCRTestStruct.generated.h"

USTRUCT(BlueprintType)
struct FBCRTestStruct
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="Test")
	FBlueprintComponentReference Reference;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="Test")
	TArray<FBlueprintComponentReference> ReferenceArray;
};
