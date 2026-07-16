// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BGMTypes.h"
#include "BGMConfig.generated.h"


UCLASS()
class BUMPERCART_API UBGMConfig : public UDataAsset
{
	GENERATED_BODY()

public:
    const FBGMSceneInfo* FindBGMInfo(EBGMScene Scene) const
    {
        return SceneBGMMap.Find(Scene);
    }

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    TMap<EBGMScene, FBGMSceneInfo> SceneBGMMap;
};
