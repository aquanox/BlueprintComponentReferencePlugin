#pragma once

#include "Misc/EngineVersionComparison.h"
#include "Brushes/SlateNoResource.h"

#if UE_VERSION_OLDER_THAN(5, 0, 0)
#include "EditorStyleSet.h"
using FSlateStyleHelper = FEditorStyle;
#else
#include "Styling/AppStyle.h"
#include "UObject/ObjectPtr.h"
using FSlateStyleHelper = FAppStyle;
#endif
