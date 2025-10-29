#include "ThumbnailScene.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ReflectionCaptureComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "AudioDevice.h"
#include "EngineUtils.h"
#include "Actors/RenderActor.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GeometryCollection/GeometryCollectionObject.h"

FMatrix FThumbnailScene::OffsetViewMatrix = FMatrix(
	FPlane(0, 1, 0, 0),
	FPlane(0, 0, 1, 0),
	FPlane(1, 0, 0, 0),
	FPlane(0, 0, 0, 1));

FThumbnailScene::FThumbnailScene()
{
	EObjectFlags NewObjectFlags = RF_NoFlags;
	
	PreviewWorld = NewObject<UWorld>(GetTransientPackage(), NAME_None, NewObjectFlags);
	
	PreviewWorld->WorldType = EWorldType::GamePreview;

	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(PreviewWorld->WorldType);
	WorldContext.SetCurrentWorld(PreviewWorld);

	PreviewWorld->InitializeNewWorld(UWorld::InitializationValues()
		.AllowAudioPlayback(false)
		.CreatePhysicsScene(false)
		.RequiresHitProxies(false)
		.CreateNavigation(false)
		.CreateAISystem(false)
		.ShouldSimulatePhysics(false)
		.SetTransactional(false)
		.SetDefaultGameMode(nullptr)
		.EnableTraceCollision(true)
		.InitializeScenes(true)
		.CreatePhysicsScene(true)
		.ForceUseMovementComponentInNonGameWorld(false));

	FURL URL = FURL();

	PreviewWorld->InitializeActorsForPlay(URL);


	RenderActor = PreviewWorld->SpawnActor<ARenderActor>();
	RenderActor->SetFOV(FOV);

}
FThumbnailScene::~FThumbnailScene()
{
	FCoreDelegates::OnEnginePreExit.RemoveAll(this);
	Uninitialize();
}
void FThumbnailScene::InitScene()
{
	DirectionalLight = NewObject<UDirectionalLightComponent>(GetTransientPackage(), NAME_None, RF_Transient);
	DirectionalLight->Intensity = 5;
	DirectionalLight->bTransmission = true;
	DirectionalLight->Mobility = EComponentMobility::Movable;
	AddComponent(DirectionalLight, FTransform(FRotator(-45, 180, 0), FVector::ZeroVector, FVector::OneVector));

	SkyAtmosphere = NewObject<USkyAtmosphereComponent>(GetTransientPackage(), NAME_None, RF_Transient);
	SkyAtmosphere->Mobility = EComponentMobility::Movable;
	AddComponent(SkyAtmosphere, FTransform::Identity);

	SkyLight = NewObject<USkyLightComponent>(GetTransientPackage(), NAME_None, RF_Transient);
	SkyLight->Intensity = 2;
	SkyLight->Mobility = EComponentMobility::Movable;
	AddComponent(SkyLight, FTransform::Identity);
}
void FThumbnailScene::Uninitialize()
{
	if (ViewFamily)
	{
		delete ViewFamily;
	}
	if (GEngine)
	{
		UWorld* World = GetWorld();
		if (World)
		{
			if (FAudioDeviceHandle AudioDevice = World->GetAudioDevice())
			{
				AudioDevice->Flush(GetWorld(), false);
			}
		}
	}

	// Remove all the attached components
	for (int32 ComponentIndex = 0; ComponentIndex < Components.Num(); ComponentIndex++)
	{
		UActorComponent* Component = Components[ComponentIndex];

		if (bForceAllUsedMipsResident)
		{
			// Remove the mip streaming override on the mesh to be removed
			UMeshComponent* pMesh = Cast<UMeshComponent>(Component);
			if (pMesh != NULL)
			{
				pMesh->SetTextureForceResidentFlag(false);
			}
		}

		Component->UnregisterComponent();
	}

	// Uninitialize can get called from destructor or FCoreDelegates::OnPreExit (or both)
	// so make sure we empty Components and set PreviewWorld to nullptr
	Components.Empty();

	if (PreviewWorld->GetBegunPlay())
	{
		for (FActorIterator actorIter{ PreviewWorld }; actorIter; ++actorIter)
		{
			actorIter->Destroy();
		}
	}
	PreviewWorld->EndPlay(EEndPlayReason::Destroyed);

	UWorld* LocalPreviewWorld = PreviewWorld;
	PreviewWorld = nullptr;

	// The world may be released by now.
	if (LocalPreviewWorld && GEngine)
	{
		LocalPreviewWorld->CleanupWorld();
		GEngine->DestroyWorldContext(LocalPreviewWorld);
		// Release PhysicsScene for fixing big fbx importing bug
		LocalPreviewWorld->ReleasePhysicsScene();

		// The preview world is a heavy-weight object and may hold a significant amount of resources,
		// including various GPU render targets and buffers required for rendering the scene.
		// Since UWorld is garbage-collected, this memory may not be cleaned for an indeterminate amount of time.
		// By forcing garbage collection explicitly, we allow memory to be reused immediately.
		GEngine->ForceGarbageCollection(true /*bFullPurge*/);
	}
}
void FThumbnailScene::AddComponent(class UActorComponent* Component, const FTransform& LocalToWorld, bool bAttachToRoot)
{
	Components.AddUnique(Component);

	USceneComponent* SceneComp = Cast<USceneComponent>(Component);
	if (SceneComp && SceneComp->GetAttachParent() == NULL)
	{
		SceneComp->SetRelativeTransform(LocalToWorld);
	}

	Component->RegisterComponentWithWorld(GetWorld());

	if (bForceAllUsedMipsResident)
	{
		// Add a mip streaming override to the new mesh
		UMeshComponent* pMesh = Cast<UMeshComponent>(Component);
		if (pMesh != NULL)
		{
			pMesh->SetTextureForceResidentFlag(true);
		}
	}

	{
		UStaticMeshComponent* pStaticMesh = Cast<UStaticMeshComponent>(Component);
		if (pStaticMesh != nullptr)
		{
			pStaticMesh->bEvaluateWorldPositionOffset = true;
			pStaticMesh->bEvaluateWorldPositionOffsetInRayTracing = true;
		}
	}
}
void FThumbnailScene::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObjects(Components);
	Collector.AddReferencedObject(PreviewWorld);
}

FString FThumbnailScene::GetReferencerName() const
{
	return TEXT("FThumbnailScene");
}

void FThumbnailScene::Tick(float DeltaTime)
{
	if (!bSceneInitialized)
	{
		bSceneInitialized = true;
		InitScene();
	}
	if (!PreviewWorld->GetBegunPlay())
	{
		for (FActorIterator actorIter{ PreviewWorld }; actorIter; ++actorIter)
		{
			actorIter->DispatchBeginPlay();
		}
		PreviewWorld->SetBegunPlay(true);
	}
	if (LightingDirty > 0)
	{
		USkyLightComponent::UpdateSkyCaptureContents(PreviewWorld);
		UReflectionCaptureComponent::UpdateReflectionCaptureContents(PreviewWorld, nullptr, false, false, true);
		LightingDirty--;
	}

	PreviewWorld->Tick(LEVELTICK_All, DeltaTime);
	if (PreviewWorld->Scene)
	{

		RenderActor->UpdateCapture(PreviewWorld->Scene, LastViewInfo);
		//RenderActor->UpdateCapture(PreviewWorld->Scene, LastViewInfo);
		UpdateViewMatrix();
	}
	PreviewWorld->SendAllEndOfFrameUpdates();
}
void FThumbnailScene::UpdateViewMatrix()
{
	ProjectionMatrix = LastViewInfo.CalculateProjectionMatrix();//AdjustProjectionMatrixForRHI(LastViewInfo.CalculateProjectionMatrix());
	InvProjectionMatrix = ProjectionMatrix.Inverse();
	InvViewMatrix = OffsetViewMatrix * FRotationTranslationMatrix(LastViewInfo.Rotation, LastViewInfo.Location);

	FMatrix ViewRotationMatrix = FInverseRotationMatrix(LastViewInfo.Rotation) * FMatrix(
		FPlane(0, 0, 1, 0),
		FPlane(1, 0, 0, 0),
		FPlane(0, 1, 0, 0),
		FPlane(0, 0, 0, 1));

	ViewMatrix = FTranslationMatrix(-LastViewInfo.Location) * ViewRotationMatrix * ProjectionMatrix;
}
void FThumbnailScene::DeprojectScreenToWorld(const FVector2D& ScreenPosition, FVector& WorldPosition, FVector& WorldDirection)
{
	FSceneView::DeprojectScreenToWorld(ScreenPosition * RenderTargetSize.Max, RenderTargetSize, InvViewMatrix, InvProjectionMatrix, WorldPosition, WorldDirection);
}
void FThumbnailScene::ProjectWorldToScreen(const FVector& WorldPosition, FVector2D& ScreenPosition)
{
	FSceneView::ProjectWorldToScreen(WorldPosition, RenderTargetSize, ViewMatrix, ScreenPosition, true);
}
void FThumbnailScene::SetRenderTarget(UTextureRenderTarget2D* target)
{
	RenderTarget = target;
	if (RenderActor)
	{
		RenderActor->AssignRenderTarget(target);
		SetRenderDirty();
		SetLightingDirty();
		if (RenderTarget)
		{
			RenderTargetSize.Max.X = RenderTarget->SizeX;
			RenderTargetSize.Max.Y = RenderTarget->SizeY;
		}
	}
}
FVector FThumbnailScene::GetCameraLocation() const
{
	if (!RenderActor)
	{
		return FVector::ZeroVector;
	}
	return RenderActor->GetActorLocation();
}
void FThumbnailScene::SetStaticMesh(UStaticMesh* mesh)
{
	if (!mesh)
	{
		return;
	}
	if (!MeshActor)
	{
		MeshActor = PreviewWorld->SpawnActor<AThumbnailPreviewMeshActor>();
	}
	else
	{
		if (UStaticMesh* currentMesh = MeshActor->GetMesh())
		{
			if (currentMesh == mesh)
			{
				return;
			}
		}
	}
	MeshActor->SetStaticMesh(mesh);
	ARenderActor* renderActor = GetRenderActor();
	const double meshRadius = MeshActor->GetMeshRadius();
	ViewRotation = FRotator::ZeroRotator;
	const FVector pos = FVector(-meshRadius * 1.5f, meshRadius, meshRadius);
	renderActor->SetActorLocationAndRotation(pos, (-pos).Rotation());
	renderActor->SetOrthoWidth(meshRadius * 2.f);

	SetLightingDirty();
	SetRenderDirty();

}
void FThumbnailScene::SetSkeletalMesh(USkeletalMesh* mesh)
{
	if (!mesh)
	{
		return;
	}
	if (!MeshActor)
	{
		MeshActor = PreviewWorld->SpawnActor<AThumbnailPreviewMeshActor>();
	}
	else
	{
		if (USkeletalMesh* currentMesh = MeshActor->GetSkeletalMesh())
		{
			if (currentMesh == mesh)
			{
				return;
			}
		}
	}
	MeshActor->SetSkeletalMesh(mesh);
	ARenderActor* renderActor = GetRenderActor();
	const double meshRadius = MeshActor->GetMeshRadius();
	ViewRotation = FRotator::ZeroRotator;
	const FVector pos = FVector(-meshRadius * 1.5f, meshRadius, meshRadius);
	renderActor->SetActorLocationAndRotation(pos, (-pos).Rotation());
	renderActor->SetOrthoWidth(meshRadius * 2.f);

	SetLightingDirty();
	SetRenderDirty();
}
void FThumbnailScene::SetGeometryCollection(UGeometryCollection* collection)
{
	if (!MeshActor)
	{
		MeshActor = PreviewWorld->SpawnActor<AThumbnailPreviewMeshActor>();
	}
	
	MeshActor->SetGeometryCollection(collection);
	ARenderActor* renderActor = GetRenderActor();
	const double meshRadius = MeshActor->GetMeshRadius();
	ViewRotation = FRotator::ZeroRotator;
	const FVector pos = FVector(-meshRadius * 1.5f, meshRadius, meshRadius);
	renderActor->SetActorLocationAndRotation(pos, (-pos).Rotation());
	renderActor->SetOrthoWidth(meshRadius * 2.f);

	SetLightingDirty();
	SetRenderDirty();
}