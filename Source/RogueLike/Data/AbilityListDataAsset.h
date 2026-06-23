// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UObject/SoftObjectPtr.h"

#include "AbilityListDataAsset.generated.h"

/** Data asset for accessing abilities data */
UCLASS( BlueprintType )
class ROGUELIKE_API UAbilityListDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "AbilityListDataAsset" )
    TMap<TSubclassOf<class UGameplayAbility>, TSoftObjectPtr<class UAbilityDataAsset>> Abilities;
};
