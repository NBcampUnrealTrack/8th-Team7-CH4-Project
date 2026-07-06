// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CharacterSelectionConfig.generated.h"

USTRUCT(BlueprintType)
struct FCharacterData
{
    GENERATED_BODY()

    // 이름
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character")
    FText DisplayName;

    // Pawn 클래스
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character")
    TSubclassOf<APawn> PawnClass;

    // Pawn 이미지
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character")
    TSoftObjectPtr<UTexture2D> Thumbnail;
};

UCLASS()
class BUMPERCART_API UCharacterSelectionConfig : public UDataAsset
{
	GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character")
    TArray<FCharacterData> CharacterDatas;


    TSubclassOf<APawn> GetPawnClass(int32 Index) const;
};
