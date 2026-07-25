// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Blueprint/UserWidget.h"

#include "HeartWidget.generated.h"

class UImage;
class UOverlay;
/** Widget for displaying health
 * Can be in empty, half anf full state */
UCLASS( Abstract )
class ROGUELIKE_API UHeartWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION( BlueprintCallable, Category = "HealthBarHearts" )
    void SetStateEmpty();

    UFUNCTION( BlueprintCallable, Category = "HealthBarHearts" )
    void SetStateHalf();

    UFUNCTION( BlueprintCallable, Category = "HealthBarHearts" )
    void SetStateFull();

    UFUNCTION( BlueprintCallable, Category = "HealthBarHearts" )
    bool IsEmpty()
    {
        return bIsEmpty;
    }

protected:
    virtual void NativePreConstruct() override;

    // Empty texture is displayed as background under heart texture
    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "HealthBarHearts" )
    TObjectPtr<UTexture2D> TextureEmpty;

    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "HealthBarHearts" )
    TObjectPtr<UTexture2D> TextureHalfHeart;

    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "HealthBarHearts" )
    TObjectPtr<UTexture2D> TextureFullHeart;

    UPROPERTY( BlueprintReadOnly, Category = "HealthBarHearts", meta = ( BindWidget ) )
    TObjectPtr<UOverlay> Overlay;

    UPROPERTY( BlueprintReadOnly, Category = "HealthBarHearts", meta = ( BindWidget ) )
    TObjectPtr<UImage> Background;

    UPROPERTY( BlueprintReadOnly, Category = "HealthBarHearts", meta = ( BindWidget ) )
    TObjectPtr<UImage> Foreground;

private:
    bool bIsEmpty = true;
};
