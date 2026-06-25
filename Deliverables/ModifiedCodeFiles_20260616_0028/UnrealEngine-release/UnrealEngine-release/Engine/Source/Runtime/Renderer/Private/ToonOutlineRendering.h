// Toon outline mesh pass (EMeshPass::ToonOutline).
// Draws inverted-hull outlines for opaque/masked materials using the Toon shading model (MSM_Toon).
// Runs after opaque lighting has been composited into scene color and before volumetric fog,
// writing depth so fog and translucency sort correctly against the outline shell.

#pragma once

#include "CoreMinimal.h"

class FRDGBuilder;
class FViewInfo;
struct FMinimalSceneTextures;

/** Returns true when the toon outline pass is globally enabled (r.Toon.Outline). */
bool IsToonOutlinePassEnabled();

/** Renders the toon outline pass for all views into scene color + scene depth. */
void RenderToonOutlinePass(
	FRDGBuilder& GraphBuilder,
	TArrayView<FViewInfo> Views,
	const FMinimalSceneTextures& SceneTextures);
