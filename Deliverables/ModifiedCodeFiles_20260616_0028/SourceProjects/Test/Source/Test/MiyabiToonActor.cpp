// Copyright Epic Games, Inc. All Rights Reserved.

#include "MiyabiToonActor.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkeletalMesh.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace MiyabiToon
{
	template <typename TObjectType>
	TObjectType* LoadDefault(const TCHAR* AssetPath)
	{
		ConstructorHelpers::FObjectFinder<TObjectType> Finder(AssetPath);
		return Finder.Succeeded() ? Finder.Object : nullptr;
	}

	bool IsPreferredMainLight(const ADirectionalLight* Light)
	{
		if (!Light)
		{
			return false;
		}

		if (Light->Tags.Contains(FName(TEXT("KeyLight"))) || Light->GetName().Contains(TEXT("KeyLight")))
		{
			return true;
		}

#if WITH_EDITOR
		return Light->GetActorLabel().Equals(TEXT("KeyLight"), ESearchCase::IgnoreCase);
#else
		return false;
#endif
	}
}

AMiyabiToonActor::AMiyabiToonActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	Face = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Face"));
	SetRootComponent(Face);
	Face->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Face->SetSkeletalMeshAsset(MiyabiToon::LoadDefault<USkeletalMesh>(
		TEXT("/Game/ZZZ/Miyabi/Mesh/星見雅_配布用_face.星見雅_配布用_face")));

	auto CreateFollower = [this](const TCHAR* Name, const TCHAR* MeshPath)
	{
		USkeletalMeshComponent* Component = CreateDefaultSubobject<USkeletalMeshComponent>(Name);
		Component->SetupAttachment(Face);
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetSkeletalMeshAsset(MiyabiToon::LoadDefault<USkeletalMesh>(MeshPath));
		return Component;
	};

	Body = CreateFollower(TEXT("Body"), TEXT("/Game/ZZZ/Miyabi/Mesh/星見雅_配布用_body.星見雅_配布用_body"));
	Cloth = CreateFollower(TEXT("Cloth"), TEXT("/Game/ZZZ/Miyabi/Mesh/星見雅_配布用_cloth.星見雅_配布用_cloth"));
	FrontHair = CreateFollower(TEXT("FrontHair"), TEXT("/Game/ZZZ/Miyabi/Mesh/星見雅_配布用_fronthair.星見雅_配布用_fronthair"));
	Eye = CreateFollower(TEXT("Eye"), TEXT("/Game/ZZZ/Miyabi/Mesh/星見雅_配布用_eye.星見雅_配布用_eye"));
	Brow = CreateFollower(TEXT("Brow"), TEXT("/Game/ZZZ/Miyabi/Mesh/星見雅_配布用_eye_brow.星見雅_配布用_eye_brow"));
	EyeLight = CreateFollower(TEXT("EyeLight"), TEXT("/Game/ZZZ/Miyabi/Mesh/星見雅_配布用_eyelight.星見雅_配布用_eyelight"));
	EyeLightOuter = CreateFollower(TEXT("EyeLightOuter"), TEXT("/Game/ZZZ/Miyabi/Mesh/星見雅_配布用_eyelightout.星見雅_配布用_eyelightout"));
	EyeShadow = CreateFollower(TEXT("EyeShadow"), TEXT("/Game/ZZZ/Miyabi/Mesh/星見雅_配布用_eyeshadow.星見雅_配布用_eyeshadow"));

	FaceMaterial = MiyabiToon::LoadDefault<UMaterialInterface>(TEXT("/Game/TestToon/Miyabi/MI_TestToon_Face.MI_TestToon_Face"));
	BodyMaterial = MiyabiToon::LoadDefault<UMaterialInterface>(TEXT("/Game/TestToon/Miyabi/MI_TestToon_Body.MI_TestToon_Body"));
	ClothMaterial = MiyabiToon::LoadDefault<UMaterialInterface>(TEXT("/Game/TestToon/Miyabi/MI_TestToon_Cloth.MI_TestToon_Cloth"));
	HairMaterial = MiyabiToon::LoadDefault<UMaterialInterface>(TEXT("/Game/TestToon/Miyabi/MI_TestToon_Hair.MI_TestToon_Hair"));

	Eye->SetMaterial(0, MiyabiToon::LoadDefault<UMaterialInterface>(TEXT("/Game/TestToon/Miyabi/MI_TestToon_Eye.MI_TestToon_Eye")));
	Brow->SetMaterial(0, MiyabiToon::LoadDefault<UMaterialInterface>(TEXT("/Game/TestToon/Miyabi/MI_TestToon_Brow.MI_TestToon_Brow")));
	EyeLight->SetMaterial(0, MiyabiToon::LoadDefault<UMaterialInterface>(TEXT("/Game/TestToon/Miyabi/MI_TestToon_EyeLight.MI_TestToon_EyeLight")));
	EyeLightOuter->SetMaterial(0, MiyabiToon::LoadDefault<UMaterialInterface>(TEXT("/Game/TestToon/Miyabi/MI_TestToon_EyeLightOuter.MI_TestToon_EyeLightOuter")));
	EyeShadow->SetMaterial(0, MiyabiToon::LoadDefault<UMaterialInterface>(TEXT("/Game/TestToon/Miyabi/MI_TestToon_EyeShadow.MI_TestToon_EyeShadow")));

	Eye->SetCastShadow(false);
	Brow->SetCastShadow(false);
	EyeLight->SetCastShadow(false);
	EyeLightOuter->SetCastShadow(false);
	EyeShadow->SetCastShadow(false);

	// The kimono sleeve is a thin shell; two-sided shadow casting avoids holes
	// in self-projected shadows while keeping the final Toon material unchanged.
	Cloth->bCastShadowAsTwoSided = true;
}

void AMiyabiToonActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// The translucent eye overlays manually compare against model custom depth.
	// Hair and all eye meshes stay out of that buffer so they can be handled by
	// the separate SceneDepth test used by the eye material.
	Face->SetRenderCustomDepth(true);
	Body->SetRenderCustomDepth(true);
	Cloth->SetRenderCustomDepth(true);
	Brow->SetRenderCustomDepth(true);
	FrontHair->SetRenderCustomDepth(false);
	Eye->SetRenderCustomDepth(false);
	EyeLight->SetRenderCustomDepth(false);
	EyeLightOuter->SetRenderCustomDepth(false);
	EyeShadow->SetRenderCustomDepth(false);

	EyeShadow->SetTranslucentSortPriority(1);
	EyeLightOuter->SetTranslucentSortPriority(2);
	EyeLight->SetTranslucentSortPriority(3);
	Cloth->bCastShadowAsTwoSided = true;

	ConfigureFollower(Body);
	ConfigureFollower(Cloth);
	ConfigureFollower(FrontHair);
	ConfigureFollower(Eye);
	ConfigureFollower(Brow);
	ConfigureFollower(EyeLight);
	ConfigureFollower(EyeLightOuter);
	ConfigureFollower(EyeShadow);

	Face->SetCustomPrimitiveDataFloat(0, FaceOutlineScale);
	Body->SetCustomPrimitiveDataFloat(0, BodyOutlineScale);
	Cloth->SetCustomPrimitiveDataFloat(0, BodyOutlineScale);
	FrontHair->SetCustomPrimitiveDataFloat(0, HairOutlineScale);

	RebuildDynamicMaterials();
	UpdateHeadMaterialParameters();
}

void AMiyabiToonActor::BeginPlay()
{
	Super::BeginPlay();
	RebuildDynamicMaterials();
	UpdateHeadMaterialParameters();
}

void AMiyabiToonActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateHeadMaterialParameters();
}

#if WITH_EDITOR
bool AMiyabiToonActor::ShouldTickIfViewportsOnly() const
{
	return true;
}
#endif

void AMiyabiToonActor::ConfigureFollower(USkeletalMeshComponent* Component) const
{
	if (Component && Face)
	{
		Component->SetLeaderPoseComponent(Face);
	}
}

void AMiyabiToonActor::RebuildDynamicMaterials()
{
	DynamicMaterials.Reset();
	SetMaterialAndTrack(Face, FaceMaterial);
	SetMaterialAndTrack(Body, BodyMaterial);
	SetMaterialAndTrack(Cloth, ClothMaterial);
	SetMaterialAndTrack(FrontHair, HairMaterial);
}

void AMiyabiToonActor::SetMaterialAndTrack(USkeletalMeshComponent* Component, UMaterialInterface* Material)
{
	if (!Component || !Material)
	{
		return;
	}

	const int32 MaterialCount = FMath::Max(1, Component->GetNumMaterials());
	for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
	{
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Material, this);
		Component->SetMaterial(MaterialIndex, MID);
		DynamicMaterials.Add(MID);
	}
}

FVector AMiyabiToonActor::ResolveMainLightDirection() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return FVector::UpVector;
	}

	const ADirectionalLight* SelectedLight = nullptr;
	for (TActorIterator<ADirectionalLight> It(World); It; ++It)
	{
		const ADirectionalLight* Light = *It;
		if (!IsValid(Light) || Light->IsHidden())
		{
			continue;
		}

		if (MiyabiToon::IsPreferredMainLight(Light))
		{
			SelectedLight = Light;
			break;
		}

		if (!SelectedLight)
		{
			SelectedLight = Light;
		}
	}

	if (!SelectedLight)
	{
		return FVector::UpVector;
	}

	// UE lighting code uses the direction from the shaded point toward the light.
	// For DirectionalLight actors this is opposite the actor's forward/ray direction.
	FVector LightDirection = -SelectedLight->GetActorForwardVector();
	if (!LightDirection.Normalize())
	{
		LightDirection = FVector::UpVector;
	}
	return LightDirection;
}

void AMiyabiToonActor::UpdateHeadMaterialParameters()
{
	if (!Face || DynamicMaterials.IsEmpty())
	{
		return;
	}

	const FVector HeadCenter = Face->GetSocketLocation(TEXT("HeadCenter"));
	FVector HeadForward = Face->GetSocketLocation(TEXT("HeadForward")) - HeadCenter;
	FVector HeadRight = Face->GetSocketLocation(TEXT("HeadRight")) - HeadCenter;

	if (!HeadForward.Normalize())
	{
		HeadForward = Face->GetForwardVector();
	}
	if (!HeadRight.Normalize())
	{
		HeadRight = Face->GetRightVector();
	}

	FVector HeadUp = FVector::CrossProduct(HeadForward, HeadRight);
	if (!HeadUp.Normalize())
	{
		HeadUp = Face->GetUpVector();
	}

	const FLinearColor CenterParameter(HeadCenter.X, HeadCenter.Y, HeadCenter.Z, 1.0f);
	const FLinearColor ForwardParameter(HeadForward.X, HeadForward.Y, HeadForward.Z, 0.0f);
	const FLinearColor RightParameter(HeadRight.X, HeadRight.Y, HeadRight.Z, 0.0f);
	const FLinearColor UpParameter(HeadUp.X, HeadUp.Y, HeadUp.Z, 0.0f);
	const FVector MainLightDirection = ResolveMainLightDirection();
	const FLinearColor MainLightParameter(
		MainLightDirection.X,
		MainLightDirection.Y,
		MainLightDirection.Z,
		0.0f);

	for (UMaterialInstanceDynamic* MID : DynamicMaterials)
	{
		if (!MID)
		{
			continue;
		}

		MID->SetVectorParameterValue(TEXT("ToonHeadCenterWS"), CenterParameter);
		MID->SetVectorParameterValue(TEXT("ToonHeadForwardWS"), ForwardParameter);
		MID->SetVectorParameterValue(TEXT("ToonHeadRightWS"), RightParameter);
		MID->SetVectorParameterValue(TEXT("ToonHeadUpWS"), UpParameter);
		MID->SetVectorParameterValue(TEXT("ToonMainLightDirWS"), MainLightParameter);
	}
}
