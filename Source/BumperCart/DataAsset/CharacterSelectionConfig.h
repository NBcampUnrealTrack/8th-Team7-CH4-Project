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


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character")
    FLinearColor Color;
};

UCLASS()
class BUMPERCART_API UCharacterSelectionConfig : public UDataAsset
{
	GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character")
    TArray<FCharacterData> CharacterDatas;

    UFUNCTION(BlueprintCallable)
    FLinearColor GetColor(int32 Index) const;

};
