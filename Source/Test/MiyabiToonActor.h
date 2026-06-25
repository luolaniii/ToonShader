// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MiyabiToonActor.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class USkeletalMeshComponent;

UCLASS(Blueprintable)
class TEST_API AMiyabiToonActor : public AActor
{
	GENERATED_BODY()

public:
	AMiyabiToonActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
#if WITH_EDITOR
	virtual bool ShouldTickIfViewportsOnly() const override;
#endif

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Miyabi|Meshes")
	TObjectPtr<USkeletalMeshComponent> Face;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Miyabi|Meshes")
	TObjectPtr<USkeletalMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Miyabi|Meshes")
	TObjectPtr<USkeletalMeshComponent> Cloth;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Miyabi|Meshes")
	TObjectPtr<USkeletalMeshComponent> FrontHair;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Miyabi|Meshes")
	TObjectPtr<USkeletalMeshComponent> Eye;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Miyabi|Meshes")
	TObjectPtr<USkeletalMeshComponent> Brow;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Miyabi|Meshes")
	TObjectPtr<USkeletalMeshComponent> EyeLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Miyabi|Meshes")
	TObjectPtr<USkeletalMeshComponent> EyeLightOuter;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Miyabi|Meshes")
	TObjectPtr<USkeletalMeshComponent> EyeShadow;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Miyabi|Materials")
	TObjectPtr<UMaterialInterface> FaceMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Miyabi|Materials")
	TObjectPtr<UMaterialInterface> BodyMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Miyabi|Materials")
	TObjectPtr<UMaterialInterface> ClothMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Miyabi|Materials")
	TObjectPtr<UMaterialInterface> HairMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Miyabi|Outline", meta = (ClampMin = "0.0"))
	float BodyOutlineScale = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Miyabi|Outline", meta = (ClampMin = "0.0"))
	float FaceOutlineScale = 0.65f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Miyabi|Outline", meta = (ClampMin = "0.0"))
	float HairOutlineScale = 0.8f;

private:
	void ConfigureFollower(USkeletalMeshComponent* Component) const;
	void RebuildDynamicMaterials();
	FVector ResolveMainLightDirection() const;
	void UpdateHeadMaterialParameters();
	void SetMaterialAndTrack(USkeletalMeshComponent* Component, UMaterialInterface* Material);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DynamicMaterials;
};
