// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectPtr.h"
#include "UObject/SoftObjectPtr.h"

#include "Blueprint/UserWidget.h"

#include "AbilityWidget.generated.h"

class UAbilityDataAsset;
class UImage;
class UTextBlock;
class UTexture2D;

/** Display-only ability row used inside UAbilityListWidget. */
UCLASS( Abstract, Blueprintable )
class ROGUELIKE_API UAbilityWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION( BlueprintCallable, Category = "AbilityWidget" )
    void FillWidget( const FText& InAbilityName, TSoftObjectPtr<UTexture2D> InAbilityIcon );

    /** Shows ErrorAbilityData, or a built-in placeholder if that asset is unset. */
    UFUNCTION( BlueprintCallable, Category = "AbilityWidget" )
    void ShowError();

protected:
    UPROPERTY( meta = ( BindWidget ) )
    TObjectPtr<UTextBlock> AbilityName;

    UPROPERTY( meta = ( BindWidget ) )
    TObjectPtr<UImage> AbilityIcon;

    /** Optional fallback row content when ability data cannot be shown. */
    UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category = "AbilityWidget" )
    TObjectPtr<UAbilityDataAsset> ErrorAbilityData;

private:
    void ApplyIcon( TSoftObjectPtr<UTexture2D> InAbilityIcon );
};
