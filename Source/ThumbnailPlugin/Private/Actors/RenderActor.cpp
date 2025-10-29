#include "Actors/RenderActor.h"
#include "Components/SceneCaptureComponent2D.h"
#include "GeometryCollection/GeometryCollection.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/SpringArmComponent.h"


ARenderActor::ARenderActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	Capturer = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Capturer"));

	RootComponent = Capturer;

	Capturer->CaptureSource = ESceneCaptureSource::SCS_FinalColorHDR;
	Capturer->bAlwaysPersistRenderingState = false;
	//Capturer->CompositeMode = ESceneCaptureCompositeMode::SCCM_Composite;
	Capturer->FOVAngle = 60.f;
}

void ARenderActor::AssignRenderTarget(UTextureRenderTarget2D* renderTarget)
{
	Capturer->TextureTarget = renderTarget;
}

void ARenderActor::UpdateCapture(FSceneInterface* scene, FMinimalViewInfo& outViewInfo)
{
	Capturer->UpdateDeferredCaptures(scene);
	Capturer->GetCameraView(0.0, outViewInfo);
}
void ARenderActor::SetFOV(float FOV)
{
	Capturer->FOVAngle = FOV;
}
void ARenderActor::SetOrthoWidth(float width)
{
	Capturer->OrthoWidth = width;
}
void ARenderActor::SetProjection(ECameraProjectionMode::Type projectionMode)
{
	Capturer->ProjectionType = projectionMode;
}
ECameraProjectionMode::Type ARenderActor::GetProjection()
{
	return Capturer->ProjectionType;
}
// Called when the game starts or when spawned
void ARenderActor::BeginPlay()
{
	Super::BeginPlay();
}

AThumbnailPreviewMeshActor::AThumbnailPreviewMeshActor()
{
	MeshOffset = CreateDefaultSubobject<USceneComponent>(TEXT("Mesh Offset"));
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Preview Mesh Component"));
	SkeletalMesh = CreateDefaultSubobject< USkeletalMeshComponent>(TEXT("Preview SkeletalMesh Component"));
	GeometryCollectionComponent = CreateDefaultSubobject<UGeometryCollectionComponent>(TEXT("Preview Geometry Collection"));
	RootComponent = MeshOffset;

	Mesh->SetupAttachment(MeshOffset);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SkeletalMesh->SetupAttachment(MeshOffset);
	SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	GeometryCollectionComponent->SetupAttachment(MeshOffset);
	GeometryCollectionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GeometryCollectionComponent->ObjectType = EObjectStateTypeEnum::Chaos_Object_Kinematic;

	PrimaryActorTick.bCanEverTick = true;
}
void AThumbnailPreviewMeshActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!GeometryCollectionName.IsNone())
	{
		GeometryCollectionComponent->SetRenderStateDirty();
	}
}
UStaticMesh* AThumbnailPreviewMeshActor::GetMesh() const
{
	return Mesh->GetStaticMesh();
}
USkeletalMesh* AThumbnailPreviewMeshActor::GetSkeletalMesh() const
{
	return SkeletalMeshAsset;
}
void AThumbnailPreviewMeshActor::SetStaticMesh(UStaticMesh* mesh)
{
	if (!mesh)
	{
		return;
	}
	if (!GeometryCollectionName.IsNone())
	{
		GeometryCollectionComponent->SetRestCollection(nullptr);
		GeometryCollectionName = NAME_None;
	}
	if (SkeletalMeshAsset)
	{
		SkeletalMesh->SetSkeletalMesh(nullptr);
		SkeletalMeshAsset = nullptr;
	}
	Mesh->SetStaticMesh(mesh);
	FBoxSphereBounds bounds = mesh->GetBounds();
	MeshOffset->SetWorldRotation(FRotator::ZeroRotator);
	Mesh->SetRelativeLocation(-(bounds.Origin));
	Mesh->LightmassSettings.bUseTwoSidedLighting = true;
	MeshAsset = mesh;
}
void AThumbnailPreviewMeshActor::SetSkeletalMesh(USkeletalMesh* mesh)
{
	if (!mesh)
	{
		return;
	}
	
	if (!GeometryCollectionName.IsNone())
	{
		GeometryCollectionComponent->SetRestCollection(nullptr);
		GeometryCollectionName = NAME_None;
	}
	if (MeshAsset)
	{
		Mesh->SetStaticMesh(nullptr);
		MeshAsset = nullptr;
	}


	SkeletalMesh->SetSkeletalMesh(mesh);
	FBoxSphereBounds bounds = mesh->GetBounds();
	MeshOffset->SetWorldRotation(FRotator::ZeroRotator);
	SkeletalMesh->SetRelativeLocation(-(bounds.Origin));
	SkeletalMeshAsset = mesh;
}
void AThumbnailPreviewMeshActor::SetGeometryCollection(UGeometryCollection* collection)
{
	if (SkeletalMeshAsset)
	{
		SkeletalMesh->SetSkeletalMesh(nullptr);
		SkeletalMeshAsset = nullptr;
	}
	if (MeshAsset)
	{
		Mesh->SetStaticMesh(nullptr);
		MeshAsset = nullptr;
	}
	
	
	GeometryCollectionComponent->SetRestCollection(collection);
	FBoxSphereBounds bounds = GeometryCollectionComponent->CalcLocalBounds();
	GeometryCollectionComponent->SetRelativeLocation(-(bounds.Origin));
	
}
double AThumbnailPreviewMeshActor::GetMeshRadius()
{
	if (MeshAsset)
	{
		return MeshAsset->GetBounds().SphereRadius;
	}
	if (SkeletalMeshAsset)
	{
		return SkeletalMeshAsset->GetBounds().SphereRadius;
	}
	if (GeometryCollectionName != NAME_None)
	{
		return GeometryCollectionComponent->Bounds.SphereRadius;
	}
	return 0;
}
