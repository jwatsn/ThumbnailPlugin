#pragma once

#include "CoreMinimal.h"
#include "UObject/GCObject.h"
#include "Engine/World.h"
#include "SceneView.h"

class UDirectionalLightComponent;
class USkyLightComponent;
class USkyAtmosphereComponent;
class ARenderActor;
class AThumbnailPreviewMeshActor;
class UGeometryCollection;

class THUMBNAILPLUGIN_API FThumbnailScene : public FGCObject
{

	uint8 RenderDirty = 0;
	uint8 LightingDirty = 0;

	bool bSceneInitialized = false;

	FSceneViewFamilyContext* ViewFamily = nullptr;
	UTextureRenderTarget2D* RenderTarget = nullptr;
	FRotator ViewRotation = FRotator::ZeroRotator;
	FIntRect RenderTargetSize;
	FMinimalViewInfo LastViewInfo;
	FMatrix ProjectionMatrix;
	FMatrix InvProjectionMatrix;
	FMatrix ViewMatrix;
	FMatrix InvViewMatrix;

	float RenderCount = 0;
	float RenderTime = 0.02325;
public:
	FThumbnailScene();
	virtual ~FThumbnailScene();

	FVector GetCameraLocation() const;
	UWorld* GetWorld() const { return PreviewWorld; };
	FSceneInterface* GetScene() const { return PreviewWorld->Scene; };
	ARenderActor* GetRenderActor() const { return RenderActor; };
	const FMatrix& GetViewMatrix() { return ViewMatrix; };
	const FMatrix& GetProjectionMatrix() { return ProjectionMatrix; };
	const FMinimalViewInfo& GetLastViewInfo() { return LastViewInfo; };
	const FIntRect& GetRenderTargetSize() { return RenderTargetSize; };


	void AddComponent(class UActorComponent* Component, const FTransform& LocalToWorld, bool bAttachToRoot = false);
	virtual void Tick(float DeltaTime);

	bool IsRenderDirty() const { return RenderDirty > 0; };

	void SetRenderDirty(uint8 numFrames = 2) { RenderDirty = numFrames > RenderDirty ? numFrames : RenderDirty; };
	void SetLightingDirty(uint8 numFrames = 2) { LightingDirty = numFrames > LightingDirty ? numFrames : LightingDirty; };
	void DeprojectScreenToWorld(const FVector2D& ScreenPosition, FVector& WorldPosition, FVector& WorldDirection);
	void ProjectWorldToScreen(const FVector& WorldPosition, FVector2D& ScreenPosition);
	void SetRenderTarget(UTextureRenderTarget2D* target);
	UTextureRenderTarget2D* GetRenderTarget() { return RenderTarget; };
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual void ResetScene() {};
	virtual FString GetReferencerName() const override;

	void SetStaticMesh(UStaticMesh* mesh);
	void SetSkeletalMesh(USkeletalMesh* mesh);
	void SetGeometryCollection(UGeometryCollection* collection);

protected:

	virtual void InitScene();

private:
	void UpdateViewMatrix();
	void Uninitialize();
public:
	float FOV = 70.f;
	static FMatrix OffsetViewMatrix;
protected:
	TObjectPtr<class UWorld> PreviewWorld = nullptr;

private:

	TArray<TObjectPtr<class UActorComponent>> Components;

	bool bForceAllUsedMipsResident = true;

	TObjectPtr<ARenderActor> RenderActor = nullptr;

	TObjectPtr<AThumbnailPreviewMeshActor> MeshActor = nullptr;

	TObjectPtr<UDirectionalLightComponent> DirectionalLight = nullptr;
	TObjectPtr<USkyAtmosphereComponent> SkyAtmosphere = nullptr;
	TObjectPtr<USkyLightComponent> SkyLight = nullptr;
};
