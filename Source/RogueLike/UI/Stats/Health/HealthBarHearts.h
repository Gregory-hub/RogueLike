// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Blueprint/UserWidget.h"

#include "HealthBarHearts.generated.h"

class UAbilitySystemComponent;
class UImage;
struct FOnAttributeChangeData;
class UHorizontalBox;

/**
 * Display-only health bar that shows the owning pawn's Health as half/full heart icons.
 * Binds to the Ability System Health attribute on construct and rebuilds heart images on change.
 */
UCLASS( Abstract )
class ROGUELIKE_API UHealthBarHearts : public UUserWidget
{
    GENERATED_BODY()

protected:
    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "HealthBarHearts" )
    TObjectPtr<UTexture2D> TextureHalfHeart;

    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "HealthBarHearts" )
    TObjectPtr<UTexture2D> TextureFullHeart;

    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "HealthBarHearts", meta = ( ClampMin = 0.01 ) )
    float HealthPerHalfHeart = 0.5f;

    UPROPERTY( BlueprintReadOnly, Category = "HealthBarHearts", meta = ( BindWidget ) )
    TObjectPtr<UHorizontalBox> HorizontalBox;

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    void UpdateHealthBar( const FOnAttributeChangeData& Data );
    void UpdateHealthBar( float NewHealth );

private:
    // Returns number of half hearts added
    int AddHeartImages( int HalfHeartsToAdd );
    
    // Returns number of half hearts removed
    int RemoveHeartImages( int HalfHeartsToRemove );
    
    // Returns number of half hearts added
    int AddHeartImage( UTexture2D* Texture );

    // Returns number of half hearts removed
    int RemoveHeartImage();

    int HalfHeartNum = 0;

    UPROPERTY()
    TArray<TObjectPtr<UImage>> Images;

    FDelegateHandle HealthChangedHandle;
    TWeakObjectPtr<UAbilitySystemComponent> CachedAbilitySystem;
};
