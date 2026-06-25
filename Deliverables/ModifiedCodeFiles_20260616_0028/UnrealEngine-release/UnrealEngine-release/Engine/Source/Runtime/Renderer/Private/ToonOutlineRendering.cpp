// Toon outline mesh pass implementation. See ToonOutlineRendering.h.
// Wiring follows FCustomDepthPassMeshProcessor (processor/registration) and
// RenderLightMapDensities (pass uniform buffer + RDG raster pass).

#include "ToonOutlineRendering.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "MeshPassProcessor.h"
#include "MeshPassProcessor.inl"
#include "MeshMaterialShader.h"
#include "Materials/Material.h"
#include "MaterialDomain.h"
#include "SceneRendering.h"
#include "ScenePrivate.h"
#include "SceneTextures.h"
#include "PSOPrecache.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"

static TAutoConsoleVariable<int32> CVarToonOutline(
	TEXT("r.Toon.Outline"),
	1,
	TEXT("Enables the toon outline mesh pass for materials using the Toon shading model.\n")
	TEXT(" 0: off\n")
	TEXT(" 1: on (default)"),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarToonOutlineWidth(
	TEXT("r.Toon.Outline.Width"),
	3.0f,
	TEXT("Toon outline width in pixels (screen-space constant). Scaled per primitive by CustomPrimitiveData[0]."),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarToonOutlineColorMul(
	TEXT("r.Toon.Outline.ColorMul"),
	0.2f,
	TEXT("Multiplier applied to the material BaseColor to derive the outline color (lower = darker outline)."),
	ECVF_RenderThreadSafe);

DECLARE_GPU_DRAWCALL_STAT_NAMED(ToonOutline, TEXT("Toon Outline"));

bool IsToonOutlinePassEnabled()
{
	return CVarToonOutline.GetValueOnRenderThread() != 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Pass uniform buffer

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FToonOutlineUniformParameters, )
	SHADER_PARAMETER(float, OutlineWidth)
	SHADER_PARAMETER(FVector3f, OutlineColorMul)
END_GLOBAL_SHADER_PARAMETER_STRUCT()

IMPLEMENT_STATIC_UNIFORM_BUFFER_STRUCT(FToonOutlineUniformParameters, "ToonOutlinePass", SceneTextures);

static TRDGUniformBufferRef<FToonOutlineUniformParameters> CreateToonOutlineUniformBuffer(FRDGBuilder& GraphBuilder)
{
	auto* UniformBufferParameters = GraphBuilder.AllocParameters<FToonOutlineUniformParameters>();
	UniformBufferParameters->OutlineWidth = FMath::Max(CVarToonOutlineWidth.GetValueOnRenderThread(), 0.0f);
	const float ColorMul = FMath::Clamp(CVarToonOutlineColorMul.GetValueOnRenderThread(), 0.0f, 1.0f);
	UniformBufferParameters->OutlineColorMul = FVector3f(ColorMul, ColorMul, ColorMul);
	return GraphBuilder.CreateUniformBuffer(UniformBufferParameters);
}

/////////////////////////////////////////////////////////////////////////////////////////
// Shaders

static bool ShouldCompileToonOutlineShaders(const FMeshMaterialShaderPermutationParameters& Parameters)
{
	return Parameters.MaterialParameters.ShadingModels.HasShadingModel(MSM_Toon)
		&& (IsOpaqueBlendMode(Parameters.MaterialParameters) || IsMaskedBlendMode(Parameters.MaterialParameters))
		&& Parameters.MaterialParameters.MaterialDomain == MD_Surface
		&& !Parameters.VertexFactoryType->SupportsNaniteRendering()
		&& IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
}

class FToonOutlineVS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FToonOutlineVS, MeshMaterial);

protected:
	FToonOutlineVS() = default;
	FToonOutlineVS(const FMeshMaterialShaderType::CompiledShaderInitializerType& Initializer)
		: FMeshMaterialShader(Initializer)
	{}

public:
	static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters)
	{
		return ShouldCompileToonOutlineShaders(Parameters);
	}
};

class FToonOutlinePS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FToonOutlinePS, MeshMaterial);

public:
	FToonOutlinePS() = default;
	FToonOutlinePS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FMeshMaterialShader(Initializer)
	{}

	static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters)
	{
		return ShouldCompileToonOutlineShaders(Parameters);
	}

	static void ModifyCompilationEnvironment(const FMaterialShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FMeshMaterialShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("SCENE_TEXTURES_DISABLED"), 1u);
	}
};

IMPLEMENT_MATERIAL_SHADER_TYPE(, FToonOutlineVS, TEXT("/Engine/Private/ToonOutline.usf"), TEXT("MainVS"), SF_Vertex);
IMPLEMENT_MATERIAL_SHADER_TYPE(, FToonOutlinePS, TEXT("/Engine/Private/ToonOutline.usf"), TEXT("MainPS"), SF_Pixel);

/////////////////////////////////////////////////////////////////////////////////////////
// Mesh pass processor

class FToonOutlineMeshProcessor : public FSceneRenderingAllocatorObject<FToonOutlineMeshProcessor>, public FMeshPassProcessor
{
public:
	FToonOutlineMeshProcessor(const FScene* Scene, ERHIFeatureLevel::Type FeatureLevel, const FSceneView* InViewIfDynamicMeshCommand, FMeshPassDrawListContext* InDrawListContext);

	virtual void AddMeshBatch(const FMeshBatch& RESTRICT MeshBatch, uint64 BatchElementMask, const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy, int32 StaticMeshId = -1) override final;
	virtual void CollectPSOInitializers(const FSceneTexturesConfig& SceneTexturesConfig, const FMaterial& Material, const FPSOPrecacheVertexFactoryData& VertexFactoryData, const FPSOPrecacheParams& PreCacheParams, TArray<FPSOPrecacheData>& PSOInitializers) override final;

private:
	static bool ShouldDraw(const FMaterial& Material)
	{
		return Material.GetShadingModels().HasShadingModel(MSM_Toon)
			&& (IsOpaqueBlendMode(Material) || IsMaskedBlendMode(Material))
			&& Material.GetMaterialDomain() == MD_Surface;
	}

	bool Process(
		const FMeshBatch& MeshBatch,
		uint64 BatchElementMask,
		int32 StaticMeshId,
		const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy,
		const FMaterialRenderProxy& RESTRICT MaterialRenderProxy,
		const FMaterial& RESTRICT MaterialResource,
		ERasterizerFillMode MeshFillMode,
		ERasterizerCullMode MeshCullMode);

	FMeshPassProcessorRenderState PassDrawRenderState;
};

FToonOutlineMeshProcessor::FToonOutlineMeshProcessor(const FScene* Scene, ERHIFeatureLevel::Type FeatureLevel, const FSceneView* InViewIfDynamicMeshCommand, FMeshPassDrawListContext* InDrawListContext)
	: FMeshPassProcessor(EMeshPass::ToonOutline, Scene, FeatureLevel, InViewIfDynamicMeshCommand, InDrawListContext)
{
	PassDrawRenderState.SetBlendState(TStaticBlendState<>::GetRHI());
	// Test against and write scene depth: interior shell pixels fail against the original mesh,
	// leaving only the silhouette ring; writing depth keeps fog/translucency sorting correct.
	PassDrawRenderState.SetDepthStencilState(TStaticDepthStencilState<true, CF_DepthNearOrEqual>::GetRHI());
}

void FToonOutlineMeshProcessor::AddMeshBatch(const FMeshBatch& RESTRICT MeshBatch, uint64 BatchElementMask, const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy, int32 StaticMeshId)
{
	if (!MeshBatch.bUseForMaterial)
	{
		return;
	}

	const FMaterialRenderProxy* MaterialRenderProxy = MeshBatch.MaterialRenderProxy;
	while (MaterialRenderProxy)
	{
		const FMaterial* Material = MaterialRenderProxy->GetMaterialNoFallback(FeatureLevel);
		if (Material)
		{
			if (!ShouldDraw(*Material))
			{
				return; // Fallbacks won't be toon either; skip the batch entirely.
			}

			const FMeshDrawingPolicyOverrideSettings OverrideSettings = ComputeMeshOverrideSettings(MeshBatch);
			const ERasterizerFillMode MeshFillMode = ComputeMeshFillMode(*Material, OverrideSettings);
			const ERasterizerCullMode MeshCullMode = ComputeMeshCullMode(*Material, OverrideSettings);

			// Inverted hull: draw the back faces of the expanded shell.
			if (Process(MeshBatch, BatchElementMask, StaticMeshId, PrimitiveSceneProxy, *MaterialRenderProxy, *Material, MeshFillMode, InverseCullMode(MeshCullMode)))
			{
				break;
			}
		}

		MaterialRenderProxy = MaterialRenderProxy->GetFallback(FeatureLevel);
	}
}

bool FToonOutlineMeshProcessor::Process(
	const FMeshBatch& RESTRICT MeshBatch,
	uint64 BatchElementMask,
	int32 StaticMeshId,
	const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy,
	const FMaterialRenderProxy& RESTRICT MaterialRenderProxy,
	const FMaterial& RESTRICT MaterialResource,
	ERasterizerFillMode MeshFillMode,
	ERasterizerCullMode MeshCullMode)
{
	const FVertexFactory* VertexFactory = MeshBatch.VertexFactory;

	TMeshProcessorShaders<FToonOutlineVS, FToonOutlinePS> PassShaders;

	FMaterialShaderTypes ShaderTypes;
	ShaderTypes.AddShaderType<FToonOutlineVS>();
	ShaderTypes.AddShaderType<FToonOutlinePS>();

	FMaterialShaders Shaders;
	if (!MaterialResource.TryGetShaders(ShaderTypes, VertexFactory->GetType(), Shaders))
	{
		return false;
	}

	Shaders.TryGetVertexShader(PassShaders.VertexShader);
	Shaders.TryGetPixelShader(PassShaders.PixelShader);

	FMeshMaterialShaderElementData ShaderElementData;
	ShaderElementData.InitializeMeshMaterialData(ViewIfDynamicMeshCommand, PrimitiveSceneProxy, MeshBatch, StaticMeshId, false);

	const FMeshDrawCommandSortKey SortKey = CalculateMeshStaticSortKey(PassShaders.VertexShader, PassShaders.PixelShader);

	BuildMeshDrawCommands(
		MeshBatch,
		BatchElementMask,
		PrimitiveSceneProxy,
		MaterialRenderProxy,
		MaterialResource,
		PassDrawRenderState,
		PassShaders,
		MeshFillMode,
		MeshCullMode,
		SortKey,
		EMeshPassFeatures::Default,
		ShaderElementData);

	return true;
}

void FToonOutlineMeshProcessor::CollectPSOInitializers(const FSceneTexturesConfig& SceneTexturesConfig, const FMaterial& Material, const FPSOPrecacheVertexFactoryData& VertexFactoryData, const FPSOPrecacheParams& PreCacheParams, TArray<FPSOPrecacheData>& PSOInitializers)
{
	if (!ShouldDraw(Material) || VertexFactoryData.VertexFactoryType->SupportsNaniteRendering())
	{
		return;
	}

	TMeshProcessorShaders<FToonOutlineVS, FToonOutlinePS> PassShaders;

	FMaterialShaderTypes ShaderTypes;
	ShaderTypes.AddShaderType<FToonOutlineVS>();
	ShaderTypes.AddShaderType<FToonOutlinePS>();

	FMaterialShaders Shaders;
	if (!Material.TryGetShaders(ShaderTypes, VertexFactoryData.VertexFactoryType, Shaders))
	{
		return;
	}

	Shaders.TryGetVertexShader(PassShaders.VertexShader);
	Shaders.TryGetPixelShader(PassShaders.PixelShader);

	const FMeshDrawingPolicyOverrideSettings OverrideSettings = ComputeMeshOverrideSettings(PreCacheParams);
	const ERasterizerFillMode MeshFillMode = ComputeMeshFillMode(Material, OverrideSettings);
	const ERasterizerCullMode MeshCullMode = InverseCullMode(ComputeMeshCullMode(Material, OverrideSettings));

	FGraphicsPipelineRenderTargetsInfo RenderTargetsInfo;
	RenderTargetsInfo.NumSamples = 1;
	AddRenderTargetInfo(SceneTexturesConfig.ColorFormat, SceneTexturesConfig.ColorCreateFlags, RenderTargetsInfo);
	SetupDepthStencilInfo(PF_DepthStencil, SceneTexturesConfig.DepthCreateFlags, ERenderTargetLoadAction::ELoad, ERenderTargetLoadAction::ELoad, FExclusiveDepthStencil::DepthWrite_StencilNop, RenderTargetsInfo);

	AddGraphicsPipelineStateInitializer(
		VertexFactoryData,
		Material,
		PassDrawRenderState,
		RenderTargetsInfo,
		PassShaders,
		MeshFillMode,
		MeshCullMode,
		(EPrimitiveType)PreCacheParams.PrimitiveType,
		EMeshPassFeatures::Default,
		true /*bRequired*/,
		PSOInitializers);
}

FMeshPassProcessor* CreateToonOutlinePassProcessor(ERHIFeatureLevel::Type FeatureLevel, const FScene* Scene, const FSceneView* InViewIfDynamicMeshCommand, FMeshPassDrawListContext* InDrawListContext)
{
	return new FToonOutlineMeshProcessor(Scene, FeatureLevel, InViewIfDynamicMeshCommand, InDrawListContext);
}

REGISTER_MESHPASSPROCESSOR_AND_PSOCOLLECTOR(RegisterToonOutlinePass, CreateToonOutlinePassProcessor, EShadingPath::Deferred, EMeshPass::ToonOutline, EMeshPassFlags::MainView);

/////////////////////////////////////////////////////////////////////////////////////////
// Render entry point

BEGIN_SHADER_PARAMETER_STRUCT(FToonOutlinePassParameters, )
	SHADER_PARAMETER_STRUCT_INCLUDE(FViewShaderParameters, View)
	SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FToonOutlineUniformParameters, Pass)
	SHADER_PARAMETER_STRUCT_INCLUDE(FInstanceCullingDrawParams, InstanceCullingDrawParams)
	RENDER_TARGET_BINDING_SLOTS()
END_SHADER_PARAMETER_STRUCT()

void RenderToonOutlinePass(
	FRDGBuilder& GraphBuilder,
	TArrayView<FViewInfo> Views,
	const FMinimalSceneTextures& SceneTextures)
{
	if (!IsToonOutlinePassEnabled())
	{
		return;
	}

	RDG_EVENT_SCOPE(GraphBuilder, "ToonOutline");
	RDG_GPU_STAT_SCOPE(GraphBuilder, ToonOutline);

	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex)
	{
		FViewInfo& View = Views[ViewIndex];

		auto* Pass = View.ParallelMeshDrawCommandPasses[EMeshPass::ToonOutline];
		if (!Pass || !View.ShouldRenderView())
		{
			continue;
		}

		RDG_EVENT_SCOPE_CONDITIONAL(GraphBuilder, Views.Num() > 1, "View%d", ViewIndex);

		View.BeginRenderView();

		FToonOutlinePassParameters* PassParameters = GraphBuilder.AllocParameters<FToonOutlinePassParameters>();
		PassParameters->View = View.GetShaderParameters();
		PassParameters->Pass = CreateToonOutlineUniformBuffer(GraphBuilder);
		PassParameters->RenderTargets[0] = FRenderTargetBinding(SceneTextures.Color.Target, ERenderTargetLoadAction::ELoad);
		PassParameters->RenderTargets.DepthStencil = FDepthStencilBinding(
			SceneTextures.Depth.Target,
			ERenderTargetLoadAction::ELoad,
			ERenderTargetLoadAction::ELoad,
			FExclusiveDepthStencil::DepthWrite_StencilNop);

		Pass->BuildRenderingCommands(GraphBuilder, View.Family->Scene->GetRenderScene()->GPUScene, PassParameters->InstanceCullingDrawParams);

		GraphBuilder.AddPass(
			RDG_EVENT_NAME("ToonOutline"),
			PassParameters,
			ERDGPassFlags::Raster,
			[&View, Pass, PassParameters](FRDGAsyncTask, FRHICommandList& RHICmdList)
		{
			FSceneRenderer::SetStereoViewport(RHICmdList, View, 1.0f);
			Pass->Draw(RHICmdList, &PassParameters->InstanceCullingDrawParams);
		});
	}
}
