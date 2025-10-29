#pragma once

#include "CoreMinimal.h"
#include "Engine/CancellableAsyncAction.h"
#include "ThumbnailSubsystem.h"
#include "ThumbnailAsyncAction.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnThumbnailFinished, UTexture2D*, ThumbnailTexture);

UCLASS()
class THUMBNAILPLUGIN_API UThumbnailAsyncAction : public UCancellableAsyncAction
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, DisplayName = "Get Static Mesh Thumbnail", meta = (WorldContext = "WorldContext", BlueprintInternalUseOnly = "true"))
	static UThumbnailAsyncAction* GetStaticMeshThumbnail(const UObject* WorldContext, UStaticMesh* inMesh, int32 sizeX = 256, int32 sizeY = 256);
	UFUNCTION(BlueprintCallable, DisplayName = "Get Skeletal Mesh Thumbnail", meta = (WorldContext = "WorldContext", BlueprintInternalUseOnly = "true"))
	static UThumbnailAsyncAction* GetSkeletalMeshThumbnail(const UObject* WorldContext, USkeletalMesh* inMesh, int32 sizeX = 256, int32 sizeY = 256);

	virtual void Activate() override;
	virtual void Cancel() override;

	void OnThumbnailComplete(UTexture2D* inTexture);

	virtual UWorld* GetWorld() const override
	{
		return ContextWorld.IsValid() ? ContextWorld.Get() : nullptr;
	}

private:
	template <typename T>
	static UThumbnailAsyncAction* GetThumbnail(const UObject* WorldContext, T* inMesh, int32 sizeX, int32 sizeY)
	{
		UWorld* ContextWorld = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull);
		if (!ensureAlwaysMsgf(IsValid(WorldContext), TEXT("World Context was not valid.")))
		{
			return nullptr;
		}
		UThumbnailSubsystem* subsystem = ContextWorld->GetGameInstance()->GetSubsystem<UThumbnailSubsystem>();
		if (!ensureAlwaysMsgf(IsValid(subsystem), TEXT("Thumbnail subsystem was not valid.")))
		{
			return nullptr;
		}
		UThumbnailQueuedEntry* entry = subsystem->QueueMeshThumbnail(inMesh);
		if (!entry)
		{
			return nullptr;
		}
		entry->SizeX = sizeX;
		entry->SizeY = sizeY;
		UThumbnailAsyncAction* NewAction = NewObject<UThumbnailAsyncAction>();
		NewAction->ContextWorld = ContextWorld;
		NewAction->RegisterWithGameInstance(ContextWorld->GetGameInstance());

		entry->OnComplete.BindUObject(NewAction, &UThumbnailAsyncAction::OnThumbnailComplete);

		return NewAction;
	}

public:
	UPROPERTY(BlueprintAssignable)
	FOnThumbnailFinished ThumbnailCompleteEvent;

	TWeakObjectPtr<UWorld> ContextWorld = nullptr;

};
