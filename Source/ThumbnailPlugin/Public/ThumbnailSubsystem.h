// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "ThumbnailSubsystem.generated.h"

class FThumbnailScene;
class UStaticMesh;
class USkeletalMesh;
class UTextureRenderTarget2D;
class UGeometryCollection;

enum EJPBThumbnailRenderState
{
	ThumbnailRenderState_Init,
	ThumbnailRenderState_Processing,
	ThumbnailRenderState_Completed
};

UCLASS()
class UThumbnailQueuedEntry : public UObject
{
	GENERATED_BODY()

public:

	UPROPERTY()
	UStaticMesh* StaticMesh = nullptr;
	UPROPERTY()
	USkeletalMesh* SkeletalMesh = nullptr;
	UPROPERTY()
	UGeometryCollection* GeometryCollection;

	int SizeX = 256;
	int SizeY = 256;
	bool bProcessingCompleted = false;
	bool bComplete = false;

	int32 ProcessingCount = 5;

	TDelegate<void(UTexture2D*)> OnComplete;

	EJPBThumbnailRenderState State = EJPBThumbnailRenderState::ThumbnailRenderState_Init;
};

UCLASS()
class THUMBNAILPLUGIN_API UThumbnailSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

	FThumbnailScene* PreviewScene = nullptr;

	UPROPERTY()
	UThumbnailQueuedEntry* CurrentEntry = nullptr;
	UPROPERTY()
	TArray<UThumbnailQueuedEntry*> Queue;
	UPROPERTY()
	UTextureRenderTarget2D* RenderTarget = nullptr;
	FVector2D RenderTargetSize = FVector2D(128, 128);
	bool bIsRunning = false;
	float SceneTimeoutCount = 0.f;

	FTickerDelegate					TickDelegate;

	FTSTicker::FDelegateHandle		TickDelegateHandle;

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable)
	UThumbnailQueuedEntry* QueueStaticMeshThumbnail(UStaticMesh* mesh);
	UFUNCTION(BlueprintCallable)
	UThumbnailQueuedEntry* QueueSkeletalMeshThumbnail(USkeletalMesh* mesh);
	UFUNCTION(BlueprintCallable)
	UThumbnailQueuedEntry* QueueGeometryCollectionThumbnail(UGeometryCollection* collection, UTexture2D* thumbnail);
	bool ThumbnailTick(float DeltaTime);


private:

	void DoInitState();
	void DoProcessingState();
	void DoCompletedState();
};


