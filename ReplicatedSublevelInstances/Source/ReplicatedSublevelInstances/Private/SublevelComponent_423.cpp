// Copyright 2025 June Rhodes. All Rights Reserved.

#if defined(IS_UNREAL_4_23)

#include "Engine.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"
#include "Net/UnrealNetwork.h"
#include "SublevelComponent.h"

USublevelComponent::USublevelComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void USublevelComponent::EndPlay(const EEndPlayReason::Type type)
{
    if (!CurrentLevelStreamInfo.PackageNameToLoad.IsNone())
    {
        RemoveFromStreamingLevels(CurrentLevelStreamInfo);
        CurrentLevelStreamInfo.PackageNameToLoad = TEXT("");
    }

    Super::EndPlay(type);
}

void USublevelComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction *ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    UWorld *const World = GetWorld();

    if (World->IsServer())
    {
        if (!LastSetLevelPath.Equals(LevelPath))
        {
            FString LongPackageName;
            bool bFoundPackage = FPackageName::SearchForPackageOnDisk(LevelPath, &LongPackageName);
            if (bFoundPackage)
            {
                TargetLevelStreamInfo.PackageNameToLoad = FName(*LongPackageName);
                LastSetLevelPath = LevelPath;
            }
            else
            {
                TargetLevelStreamInfo.PackageNameToLoad = NAME_None;
            }
        }

        TargetLevelStreamInfo.Location = GetComponentToWorld().GetLocation();
        TargetLevelStreamInfo.Rotation = GetComponentToWorld().GetRotation().Rotator();
        TargetLevelStreamInfo.bShouldBeLoaded = LevelActive;
        TargetLevelStreamInfo.bShouldBeVisible = LevelActive;
        TargetLevelStreamInfo.bShouldBlockOnLoad = false;
        TargetLevelStreamInfo.LODIndex = 1;

        if (TargetLevelStreamInfo.PackageNameToLoad != CurrentLevelStreamInfo.PackageNameToLoad ||
            TargetLevelStreamInfo.Location != CurrentLevelStreamInfo.Location ||
            TargetLevelStreamInfo.Rotation != CurrentLevelStreamInfo.Rotation ||
            TargetLevelStreamInfo.bShouldBeLoaded != CurrentLevelStreamInfo.bShouldBeLoaded ||
            TargetLevelStreamInfo.bShouldBeVisible != CurrentLevelStreamInfo.bShouldBeVisible ||
            TargetLevelStreamInfo.bShouldBlockOnLoad != CurrentLevelStreamInfo.bShouldBlockOnLoad ||
            TargetLevelStreamInfo.LODIndex != CurrentLevelStreamInfo.LODIndex)
        {
            // Things are out of sync on the server!
            if (!CurrentLevelStreamInfo.PackageNameToLoad.IsNone())
            {
                RemoveFromStreamingLevels(CurrentLevelStreamInfo);
                CurrentLevelStreamInfo.PackageNameToLoad = NAME_None;
            }

            CurrentLevelStreamInfo = TargetLevelStreamInfo;

            // Generate a new unique package name.
            const FString ShortPackageName =
                FPackageName::GetShortName(CurrentLevelStreamInfo.PackageNameToLoad.ToString());
            const FString PackagePath =
                FPackageName::GetLongPackagePath(CurrentLevelStreamInfo.PackageNameToLoad.ToString());
            FString UniqueLevelPackageName = PackagePath + TEXT("/") + World->StreamingLevelsPrefix + ShortPackageName;
            UniqueLevelPackageName += TEXT("_LevelInstance_") + FString::FromInt(this->GetUniqueID());
            CurrentLevelStreamInfo.PackageName = FName(*UniqueLevelPackageName);
            TargetLevelStreamInfo.PackageName = CurrentLevelStreamInfo.PackageName; // Replicate to clients.

            if (!CurrentLevelStreamInfo.PackageNameToLoad.IsNone())
            {
                AddToStreamingLevels(CurrentLevelStreamInfo);
            }
        }
    }
    else
    {
        if (TargetLevelStreamInfo.PackageName != CurrentLevelStreamInfo.PackageName ||
            TargetLevelStreamInfo.PackageNameToLoad != CurrentLevelStreamInfo.PackageNameToLoad ||
            TargetLevelStreamInfo.Location != CurrentLevelStreamInfo.Location ||
            TargetLevelStreamInfo.Rotation != CurrentLevelStreamInfo.Rotation ||
            TargetLevelStreamInfo.bShouldBeLoaded != CurrentLevelStreamInfo.bShouldBeLoaded ||
            TargetLevelStreamInfo.bShouldBeVisible != CurrentLevelStreamInfo.bShouldBeVisible ||
            TargetLevelStreamInfo.bShouldBlockOnLoad != CurrentLevelStreamInfo.bShouldBlockOnLoad ||
            TargetLevelStreamInfo.LODIndex != CurrentLevelStreamInfo.LODIndex)
        {
            // Things are out of sync on the client!
            if (!CurrentLevelStreamInfo.PackageNameToLoad.IsNone() && !CurrentLevelStreamInfo.PackageName.IsNone())
            {
                RemoveFromStreamingLevels(CurrentLevelStreamInfo);
                CurrentLevelStreamInfo.PackageNameToLoad = NAME_None;
            }

            CurrentLevelStreamInfo = TargetLevelStreamInfo;

            if (!CurrentLevelStreamInfo.PackageNameToLoad.IsNone() && !CurrentLevelStreamInfo.PackageName.IsNone())
            {
                AddToStreamingLevels(CurrentLevelStreamInfo);
            }
        }
    }
}

void USublevelComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(USublevelComponent, LevelPath);
    DOREPLIFETIME(USublevelComponent, LevelActive);
    DOREPLIFETIME(USublevelComponent, TargetLevelStreamInfo);
}

void USublevelComponent::AddToStreamingLevels(const FLevelStreamInstanceInfo &LevelInstanceInfo)
{
    bool bResult = true;

    UWorld *const World = GetWorld();

    if (World != nullptr)
    {
        bool bAlreadyExists = false;

        for (auto StreamingLevel : World->GetStreamingLevels())
        {
            if (StreamingLevel->GetWorldAssetPackageFName() == LevelInstanceInfo.PackageName)
            {
                bAlreadyExists = true;
                break;
            }
        }

        if (!bAlreadyExists)
        {
            FName PackageName = LevelInstanceInfo.PackageName;

            // For PIE networking: Strip off the PIE prefix for the package name, so we set a
            // non-prefixed string as the WorldAsset. Our package name might be prefixed with
            // a different PIE's network prefix (because it's replicated). Later we'll call
            // RenameForPIE which will specifically add back this PIE instance's prefix, so it
            // can then be later stripped off again when level streaming duplicates the world
            // in PIE mode.
            FString PackageNameStr = PackageName.ToString();
            if (GEngine->NetworkRemapPath(World->GetNetDriver(), PackageNameStr, true))
            {
                PackageName = FName(*PackageNameStr);
            }

            GEngine->DelayGarbageCollection();

            // Setup streaming level object that will load specified map
            ULevelStreaming *StreamingLevel = NewObject<ULevelStreamingDynamic>(
                World,
                ULevelStreamingDynamic::StaticClass(),
                NAME_None,
                RF_Transient,
                nullptr);
            StreamingLevel->SetWorldAssetByPackageName(PackageName);
            StreamingLevel->LevelColor = FColor::MakeRandomColor();
            StreamingLevel->SetShouldBeLoaded(LevelInstanceInfo.bShouldBeLoaded);
            StreamingLevel->SetShouldBeVisible(LevelInstanceInfo.bShouldBeVisible);
            StreamingLevel->bShouldBlockOnLoad = LevelInstanceInfo.bShouldBlockOnLoad;
            StreamingLevel->OnLevelLoaded = OnLevelLoaded;
            StreamingLevel->OnLevelUnloaded = OnLevelUnloaded;
            StreamingLevel->OnLevelShown = OnLevelShown;
            StreamingLevel->OnLevelHidden = OnLevelHidden;

            // Transform
            StreamingLevel->LevelTransform = FTransform(LevelInstanceInfo.Rotation, LevelInstanceInfo.Location);

            // Map to Load
            StreamingLevel->PackageNameToLoad = LevelInstanceInfo.PackageNameToLoad;

#if WITH_EDITOR
            // For PIE networking: Add back this specific PIE instance's prefix to the
            // the WorldAsset, since we stripped it off the package name above.
            auto PIEInstanceID = GetOutermost()->PIEInstanceID;
            if (PIEInstanceID != -1)
            {
                StreamingLevel->RenameForPIE(GetOutermost()->PIEInstanceID);
            }
#endif

            // Add the new level to world.
            World->AddStreamingLevel(StreamingLevel);

            World->FlushLevelStreaming(EFlushLevelStreamingType::Full);
        }
    }
}

void USublevelComponent::RemoveFromStreamingLevels(const FLevelStreamInstanceInfo &LevelInstanceInfo)
{
    UWorld *const World = GetWorld();

    // Check if the world exists and we have a level to unload
    if (World != nullptr && !LevelInstanceInfo.PackageName.IsNone())
    {

#if WITH_EDITOR
        // If we are using the editor we will use this lambda to remove the play in editor string
        auto GetCorrectPackageName = [&](FName PackageName) {
            FString PackageNameStr = PackageName.ToString();
            if (GEngine->NetworkRemapPath(World->GetNetDriver(), PackageNameStr, true))
            {
                PackageName = FName(*PackageNameStr);
            }

            return PackageName;
        };
#endif

        // Get the package name that we want to check
        FName PackageNameToCheck = LevelInstanceInfo.PackageName;

#if WITH_EDITOR
        // Remove the play in editor string and client id to be able to use it with replication
        PackageNameToCheck = GetCorrectPackageName(PackageNameToCheck);
#endif

        // Find the level to unload
        for (auto StreamingLevel : World->GetStreamingLevels())
        {

            FName LoadedPackageName = StreamingLevel->GetWorldAssetPackageFName();

#if WITH_EDITOR
            // Remove the play in editor string and client id to be able to use it with replication
            LoadedPackageName = GetCorrectPackageName(LoadedPackageName);
#endif

            // If we find the level unload it and break
            if (PackageNameToCheck == LoadedPackageName)
            {
                // This unload the level
                StreamingLevel->SetShouldBeLoaded(false);
                StreamingLevel->SetShouldBeVisible(false);
                // This removes the level from the streaming level list
                StreamingLevel->SetIsRequestingUnloadAndRemoval(true);
                // Force a refresh of the world
                World->FlushLevelStreaming(EFlushLevelStreamingType::Full);
                break;
            }
        }
    }
}

#endif