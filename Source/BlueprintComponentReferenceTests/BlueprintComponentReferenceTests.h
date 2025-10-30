#pragma once

#include "Modules/ModuleManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/EngineVersionComparison.h"

struct FTestWorldScope
{
	FWorldContext WorldContext;
	UWorld* World = nullptr;
	UWorld* PrevWorld = nullptr;

	FTestWorldScope()
	{
		World = UWorld::CreateWorld(EWorldType::Game, false);

		WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(World);

		PrevWorld = &*GWorld;
		GWorld = World;
	}

	~FTestWorldScope()
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(true);
		GWorld = PrevWorld;
	}

	UWorld* operator->() const { return World; }
};
