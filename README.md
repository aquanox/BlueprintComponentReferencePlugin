[![GitHub release](https://img.shields.io/github/release/aquanox/BlueprintComponentReferencePlugin.svg)](https://github.com/aquanox/BlueprintComponentReferencePlugin/releases)
[![GitHub license](https://img.shields.io/github/license/aquanox/BlueprintComponentReferencePlugin)](https://github.com/aquanox/BlueprintComponentReferencePlugin/blob/main/LICENSE)
[![GitHub forks](https://img.shields.io/github/forks/aquanox/BlueprintComponentReferencePlugin)](https://github.com/aquanox/BlueprintComponentReferencePlugin/network)
[![GitHub stars](https://img.shields.io/github/stars/aquanox/BlueprintComponentReferencePlugin)](https://github.com/aquanox/BlueprintComponentReferencePlugin/stargazers)
![UE5](https://img.shields.io/badge/UE5-5.0+-lightgrey)

## Blueprint Component Reference Plugin for Unreal Engine

Blueprint Component Reference Plugin provides a struct and set of accessors that allow referencing actor components from blueprint editor details view with a component picker, it automatically identifies context and builds list of components for selection menu. 

![](Images/BCR-Quick.png)

## Features

Plugin provides component picker in following use cases:
 * Actor class member UPROPERTY 
 * Struct member UPROPERTY within Actor class or blueprint.
 * Local variable in an Actor Blueprint.
 * Function local variable in an Actor Blueprint.
 * Generic member UPROPERTY (if ActorClass meta specifier is used).
 * Generic Blueprint variable (if ActorClass meta specifier is used).
 * Array in one of above
 * Map value in one of above
 
## Component Selection Customizations

Component reference property display can be customized with UPROPERTY meta specifiers.
 
Component Filtering:
 * `AllowedClasses` - List of classes for component filter
 * `DisallowedClasses` - List of classes for component filter
 * `ShowNative` - Include native (created with CreateDefaultSubobject) components in picker. Default = True.
 * `ShowBlueprint` - Include blueprint (added in Blueprint Editor) components in picker. Default = True.
 * `ShowInstanced` - Include instanced components with assosicated property in picker. Default = False.
 * `ShowHidden` - Include any components without associated property in picker. Default = False.
 * `ShowEditor` - Include editor-only components in picker. Default = True.

Item Display: 
 * `NoClear` - Hide 'Clear' button. Default = False.
 * `NoNavigate` - Hide 'Navigate to' button. Default = False.
 * `NoPicker` - Disable component picker. Default = False.

Out-of-Actor Use:
* `ActorClass` - Class to use for component selection dropdown if context detection is not possible.

Blueprint variables supported via variable editor extension.

![](Images/BCR-Variable.png)
 
 
## Unreal Engine Versions

Plugin is compatible with 5.0+, ue5-main and can be modified to work with other engine versions.

## License

Blueprint Component Reference Plugin is available under the MIT license. See the LICENSE file for more info.

## Examples

```c++

UCLASS()
class ABCRTestActor : public AInfo
{
	GENERATED_BODY()
public:
    /* Simplest reference to any component within current class */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FBlueprintComponentReference SimpleReference;
    
    /* Simplest reference to any native component within current class */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ShowNative=true, ShowBlueprint=false))
    FBlueprintComponentReference NativeOnlyReference;
    
    /* Simplest reference to any component within current class */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowedClasses="/Script/Engine.SceneComponent"))
    FBlueprintComponentReference SimpleSceneCompReference;
};

UCLASS()
class UBCRTestDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
    /* Make a reference to any component within a BCRTestActor class */
    UPROPERTY(EditAnywhere, meta=(ActorClass="/Script/BlueprintComponentReferenceTests.BCRTestActor"))
    FBlueprintComponentReference ExternalRef;
};

void AMyActor::Foo()
{
    if (auto* Component = Reference.GetComponent<UBCRActorComponent>(this))
    {
        Component->Activate();
    }
}

```

Code examples can be found in `BCRTestActor.h`

Editor View:

![](Images/BCR-Large.png)

Details View:

![](Images/BCR-Quick.png)

Graph Nodes:

![](Images/BCR-Nodes.png)

Example use:

![](Images/BCR-Hello.png)


