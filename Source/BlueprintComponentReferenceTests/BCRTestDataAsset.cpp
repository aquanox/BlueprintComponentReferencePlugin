// Copyright 2024, Aquanox.


#include "BCRTestDataAsset.h"
#include "BCRTestActor.h"

UBCRTestDataAsset::UBCRTestDataAsset()
{
	ReferenceSingle = FBlueprintComponentReference::ForProperty(GET_MEMBER_NAME_CHECKED(ABCRTestActor, Default_LevelOne));
	
	ReferenceArray.Add(FBlueprintComponentReference::ForProperty(GET_MEMBER_NAME_CHECKED(ABCRTestActor, Default_Root)));
	ReferenceArray.Add(FBlueprintComponentReference::ForProperty(GET_MEMBER_NAME_CHECKED(ABCRTestActor, Default_LevelOne)));
	ReferenceArray.Add(FBlueprintComponentReference::ForProperty(GET_MEMBER_NAME_CHECKED(ABCRTestActor, Default_LevelTwo)));
	ReferenceArray.Add(FBlueprintComponentReference::ForProperty(GET_MEMBER_NAME_CHECKED(ABCRTestActor, NonExistingComponent)));
	ReferenceArray.Add(FBlueprintComponentReference::ForProperty(GET_MEMBER_NAME_CHECKED(ABCRTestActor, Default_LevelZero)));
	
	ReferenceSet.Add(FBlueprintComponentReference::ForProperty(GET_MEMBER_NAME_CHECKED(ABCRTestActor, Default_Root)));
	ReferenceSet.Add(FBlueprintComponentReference::ForProperty(GET_MEMBER_NAME_CHECKED(ABCRTestActor, Default_LevelOne)));
	ReferenceSet.Add(FBlueprintComponentReference::ForProperty(GET_MEMBER_NAME_CHECKED(ABCRTestActor, Default_LevelTwo)));
	ReferenceSet.Add(FBlueprintComponentReference::ForProperty(GET_MEMBER_NAME_CHECKED(ABCRTestActor, NonExistingComponent)));
	ReferenceSet.Add(FBlueprintComponentReference::ForProperty(GET_MEMBER_NAME_CHECKED(ABCRTestActor, Default_LevelZero)));
	
	ReferenceMap.Add("root", FBlueprintComponentReference::ForProperty(GET_MEMBER_NAME_CHECKED(ABCRTestActor, Default_Root)));
	ReferenceMap.Add("first", FBlueprintComponentReference::ForProperty(GET_MEMBER_NAME_CHECKED(ABCRTestActor, Default_LevelOne)));
	ReferenceMap.Add("second", FBlueprintComponentReference::ForProperty(GET_MEMBER_NAME_CHECKED(ABCRTestActor, Default_LevelTwo)));
	ReferenceMap.Add("bad", FBlueprintComponentReference::ForProperty(GET_MEMBER_NAME_CHECKED(ABCRTestActor, NonExistingComponent)));
	ReferenceMap.Add("bad2", FBlueprintComponentReference::ForProperty(GET_MEMBER_NAME_CHECKED(ABCRTestActor, Default_LevelZero)));
}

void UBCRTestDataAsset::Foo()
{

}
