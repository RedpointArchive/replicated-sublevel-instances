// Copyright 2025 June Rhodes. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Engine/LevelStreamingDynamic.h"
#include "SublevelComponent.generated.h"

USTRUCT(BlueprintType)
struct FLevelStreamInstanceInfo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FName PackageName;

	UPROPERTY()
	FName PackageNameToLoad;

	UPROPERTY()
	FVector Location = FVector();

	UPROPERTY()
	FRotator Rotation = FRotator();

	UPROPERTY()
	bool bShouldBeLoaded = 0;

	UPROPERTY()
	bool bShouldBeVisible = 0;

	UPROPERTY()
	bool bShouldBlockOnLoad = 0;

	UPROPERTY()
	int32 LODIndex = 0;

	FLevelStreamInstanceInfo() {}

	FLevelStreamInstanceInfo(ULevelStreamingDynamic* LevelInstance);

	FString ToString() const
	{
		return FString::Printf(TEXT("PackageName: %s\nPackageNameToLoad:%s\nLocation:%s\nRotation:%s\nbShouldBeLoaded:%s\nbShouldBeVisible:%s\nbShouldBlockOnLoad:%s\nLODIndex:%i")
			, *PackageName.ToString()
			, *PackageNameToLoad.ToString()
			, *Location.ToString()
			, *Rotation.ToString()
			, (bShouldBeLoaded) ? TEXT("True") : TEXT("False")
			, (bShouldBeVisible) ? TEXT("True") : TEXT("False")
			, (bShouldBlockOnLoad) ? TEXT("True") : TEXT("False")
			, LODIndex);
	}
};

UCLASS(Blueprintable, ClassGroup = Utility, meta = (BlueprintSpawnableComponent))
class REPLICATEDSUBLEVELINSTANCES_API USublevelComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USublevelComponent();

	/** The path to the level asset to manage as a sublevel instance */
	UPROPERTY(Replicated, BlueprintReadWrite, EditAnywhere, Category = "Level")
	FString LevelPath;

	UPROPERTY(Replicated, BlueprintReadWrite, EditAnywhere, Category = "Level")
	bool LevelActive;

	/** Called when level is streamed in  */
	UPROPERTY(BlueprintAssignable)
	FLevelStreamingLoadedStatus			OnLevelLoaded;

	/** Called when level is streamed out  */
	UPROPERTY(BlueprintAssignable)
	FLevelStreamingLoadedStatus			OnLevelUnloaded;

	/** Called when level is added to the world  */
	UPROPERTY(BlueprintAssignable)
	FLevelStreamingVisibilityStatus		OnLevelShown;

	/** Called when level is removed from the world  */
	UPROPERTY(BlueprintAssignable)
	FLevelStreamingVisibilityStatus		OnLevelHidden;

protected:
	virtual void EndPlay(const EEndPlayReason::Type type) override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY(Replicated)
	FLevelStreamInstanceInfo TargetLevelStreamInfo;

	UPROPERTY()
	FLevelStreamInstanceInfo CurrentLevelStreamInfo;

	FString LastSetLevelPath;

	void AddToStreamingLevels(const FLevelStreamInstanceInfo& LevelInstanceInfo);
	void RemoveFromStreamingLevels(const FLevelStreamInstanceInfo& LevelInstanceInfo);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

};