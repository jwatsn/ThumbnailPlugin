#pragma once

#include "CoreMinimal.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "RenderActor.generated.h"

class USceneCaptureComponent2D;
class USpringArmComponent;
UCLASS()
class THUMBNAILPLUGIN_API ARenderActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ARenderActor();

	void AssignRenderTarget(class UTextureRenderTarget2D* renderTarget);

	void UpdateCapture(FSceneInterface* scene, FMinimalViewInfo& outViewInfo);
	void SetOrthoWidth(float width);
	void SetFOV(float FOV);
	void SetProjection(ECameraProjectionMode::Type projectionMode);
	ECameraProjectionMode::Type GetProjection();
public:

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


private:

	UPROPERTY(VisibleDefaultsOnly, Category = "RenderActor")
	class USceneCaptureComponent2D* Capturer;


public:





	float TerrainScale = 4;
	float TerrainMinHeight = 0;
	float TerrainMaxHeight = 200;
};


UCLASS()
class THUMBNAILPLUGIN_API AThumbnailPreviewMeshActor : public AActor
{
	GENERATED_BODY()

private:

	UPROPERTY()
	USceneComponent* MeshOffset;

	UPROPERTY()
	UStaticMesh* MeshAsset;

	UPROPERTY()
	USkeletalMesh* SkeletalMeshAsset;

	UPROPERTY()
	UStaticMeshComponent* Mesh;

	UPROPERTY()
	USkeletalMeshComponent* SkeletalMesh;

	UPROPERTY()
	UGeometryCollectionComponent* GeometryCollectionComponent;

	UPROPERTY()
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> InstancedStaticMesh;

	
	FName GeometryCollectionName = NAME_None;


public:
	// Sets default values for this actor's properties
	AThumbnailPreviewMeshActor();

	void SetGeometryCollection(UGeometryCollection* collection);
	void SetStaticMesh(UStaticMesh* mesh);
	void SetSkeletalMesh(USkeletalMesh* mesh);
	UStaticMesh* GetMesh() const;
	USkeletalMesh* GetSkeletalMesh() const;

	const FName& GetGeometryCollection() const { return GeometryCollectionName; };
	double GetMeshRadius();

protected:
	virtual void Tick(float DeltaTime) override;
};
