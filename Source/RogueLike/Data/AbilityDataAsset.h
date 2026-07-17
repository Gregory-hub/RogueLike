// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "AbilityDataAsset.generated.h"

/** Data asset for storing ability information */
UCLASS( BlueprintType )
class ROGUELIKE_API UAbilityDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "AbilityData" )
    FText Name;

    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "AbilityData" )
    FText Description;

    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "AbilityData" )
    TSoftObjectPtr<class UTexture2D> Icon;
};
