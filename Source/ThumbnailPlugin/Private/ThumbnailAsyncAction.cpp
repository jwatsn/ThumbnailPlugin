#include "ThumbnailAsyncAction.h"





UThumbnailAsyncAction* UThumbnailAsyncAction::GetStaticMeshThumbnail(const UObject* WorldContext, UStaticMesh* inMesh, int32 sizeX, int32 sizeY)
{
	return GetThumbnail(WorldContext, inMesh, sizeX, sizeY);
}
UThumbnailAsyncAction* UThumbnailAsyncAction::GetSkeletalMeshThumbnail(const UObject* WorldContext, USkeletalMesh* inMesh, int32 sizeX, int32 sizeY)
{
	return GetThumbnail(WorldContext, inMesh, sizeX, sizeY);
}
void UThumbnailAsyncAction::OnThumbnailComplete(UTexture2D* inTexture)
{
	ThumbnailCompleteEvent.Broadcast(inTexture);
}
void UThumbnailAsyncAction::Activate()
{

}
void UThumbnailAsyncAction::Cancel()
{

}