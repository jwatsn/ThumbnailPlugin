#include "ThumbnailSubsystem.h"
#include "ThumbnailScene.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/SceneCaptureComponent2D.h"
#include "GeometryCollection/GeometryCollection.h"
#include "Actors/RenderActor.h"
#include "Compression/OodleDataCompressionUtil.h"

#include "IImageWrapper.h"
#include "IImageWrapperModule.h"



void UThumbnailSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);


	TickDelegate = FTickerDelegate::CreateUObject(this, &UThumbnailSubsystem::ThumbnailTick);
	TickDelegateHandle = FTSTicker::GetCoreTicker().AddTicker(TickDelegate);

	RenderTarget = NewObject<UTextureRenderTarget2D>(GetTransientPackage());
	RenderTarget->RenderTargetFormat = RTF_RGBA16f;
	RenderTarget->Filter = TextureFilter::TF_MAX;
	RenderTarget->ClearColor = FLinearColor::Transparent;
	RenderTarget->InitAutoFormat(128, 128);
	RenderTarget->UpdateResourceImmediate(true);

}
void UThumbnailSubsystem::Deinitialize()
{
	FTSTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);
	Super::Deinitialize();
}

UThumbnailQueuedEntry* UThumbnailSubsystem::QueueMeshThumbnail(UStaticMesh* mesh)
{
	if (!mesh)
	{
		return nullptr;
	}
	UThumbnailQueuedEntry* newEntry = QueueThumbnail();
	newEntry->StaticMesh = mesh;
	return newEntry;
}
UThumbnailQueuedEntry* UThumbnailSubsystem::QueueMeshThumbnail(USkeletalMesh* mesh)
{
	if (!mesh)
	{
		return nullptr;
	}
	UThumbnailQueuedEntry* newEntry = QueueThumbnail();
	newEntry->SkeletalMesh = mesh;
	return newEntry;
}
UThumbnailQueuedEntry* UThumbnailSubsystem::QueueMeshThumbnail(UGeometryCollection* collection)
{
	if (!collection)
	{
		return nullptr;
	}
	UThumbnailQueuedEntry* newEntry = QueueThumbnail();
	newEntry->GeometryCollection = collection;
	return newEntry;
}
UThumbnailQueuedEntry* UThumbnailSubsystem::QueueThumbnail()
{
	UThumbnailQueuedEntry* newEntry = NewObject<UThumbnailQueuedEntry>();
	Queue.Add(newEntry);
	bIsRunning = true;
	return newEntry;
}
bool UThumbnailSubsystem::ThumbnailTick(float DeltaTime)
{
	if (!bIsRunning)
	{
		return true;
	}

	if (!PreviewScene)
	{
		PreviewScene = new FThumbnailScene();
		PreviewScene->SetRenderTarget(RenderTarget);
		PreviewScene->GetRenderActor()->SetProjection(ECameraProjectionMode::Orthographic);
	}

	if (!CurrentEntry)
	{
		if (Queue.IsEmpty())
		{
			SceneTimeoutCount += DeltaTime;
			if (SceneTimeoutCount >= 4.f)
			{
				bIsRunning = false;
				delete PreviewScene;
				PreviewScene = nullptr;
				SceneTimeoutCount = 0;
			}
			return true;
		}
		CurrentEntry = Queue.Pop();
	}
	switch (CurrentEntry->State)
	{
	case EJPBThumbnailRenderState::ThumbnailRenderState_Init:
		DoInitState();
		break;
	case EJPBThumbnailRenderState::ThumbnailRenderState_Processing:
		DoProcessingState();
		break;
	case EJPBThumbnailRenderState::ThumbnailRenderState_Completed:
		DoCompletedState();
		break;
	}

	PreviewScene->Tick(DeltaTime);
	SceneTimeoutCount = 0;

	return true;
}

void UThumbnailSubsystem::DoInitState()
{
	if (CurrentEntry)
	{
		const double sizeX = CurrentEntry->SizeX;
		const double sizeY = CurrentEntry->SizeY;

		if (sizeX != RenderTargetSize.X || sizeY != RenderTargetSize.Y)
		{
			RenderTargetSize.X = sizeX;
			RenderTargetSize.Y = sizeY;
			RenderTarget->ResizeTarget(sizeX, sizeY);
			RenderTarget->UpdateResourceImmediate(true);
		}
	}
	if (UStaticMesh* staticMesh = CurrentEntry->StaticMesh)
	{
		PreviewScene->SetStaticMesh(staticMesh);
	}
	else if (USkeletalMesh* skeletalMesh = CurrentEntry->SkeletalMesh)
	{
		PreviewScene->SetSkeletalMesh(skeletalMesh);
	}
	else if (UGeometryCollection* collection = CurrentEntry->GeometryCollection)
	{
		PreviewScene->SetGeometryCollection(collection);
	}
	

	PreviewScene->SetRenderDirty();
	PreviewScene->SetLightingDirty();
	CurrentEntry->State = EJPBThumbnailRenderState::ThumbnailRenderState_Processing;
	CurrentEntry->ProcessingCount = 12;
}
void UThumbnailSubsystem::DoProcessingState()
{
	if (!CurrentEntry)
	{
		return;
	}
	int32& ProcessingCount = CurrentEntry->ProcessingCount;
	ProcessingCount--;
	if (ProcessingCount <= 0)
	{
		CurrentEntry->State = EJPBThumbnailRenderState::ThumbnailRenderState_Completed;
		UTexture2D* newThumbnailTexture = RenderTarget->ConstructTexture2D(GetTransientPackage(), MakeUniqueObjectName(GetTransientPackage(), UTexture2D::StaticClass()).ToString(), EObjectFlags::RF_Transient);
		CurrentEntry->OnComplete.ExecuteIfBound(newThumbnailTexture);
		CurrentEntry = nullptr;
	}
}
void UThumbnailSubsystem::DoCompletedState()
{
	if (CurrentEntry)
	{
		CurrentEntry->bComplete = true;
		CurrentEntry = nullptr;
	}
}
