// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Blueprint/UserWidget.h"
#include "Layout/Margin.h"

#include "HealthBarHearts.generated.h"

class UHeartWidget;
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
    TSubclassOf<UHeartWidget> HeartWidgetClass;

    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "HealthBarHearts", meta = ( ClampMin = 0.01 ) )
    float HealthPerHalfHeart = 0.5f;

    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "HealthBarHearts", meta = ( ClampMin = 0 ) )
    FMargin HeartPadding;

    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "HealthBarHearts|Preview", meta = ( ClampMin = 0 ) )
    float PreviewHealth = 3.f;

    UPROPERTY( BlueprintReadOnly, Category = "HealthBarHearts", meta = ( BindWidget ) )
    TObjectPtr<UHorizontalBox> HorizontalBox;

    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    // ------------------ Delegates ------------------

    void UpdateCurrentHealth( const FOnAttributeChangeData& Data );
    void UpdateCurrentHealth( float NewHealth );

    void UpdateMaxHealth( const FOnAttributeChangeData& Data );
    void UpdateMaxHealth( float NewHealth );

    // ------------------ Heart widgets lifecycle ------------------

    void UpdateMaxHearts( int MaxHearts );

    void PushHearts( int Count );
    void PopHearts( int Count );

    void PushHeart();
    void PopHeart();

    // ------------------ Heart widgets contents ------------------

    void UpdateFilledHearts( int NewHalfHearts );

    // ------------------ State ------------------

    UPROPERTY()
    TArray<TObjectPtr<UHeartWidget>> Hearts;

    // ------------------ Cache ------------------

    FDelegateHandle HealthChangedHandle;
    FDelegateHandle MaxHealthChangedHandle;
    TWeakObjectPtr<UAbilitySystemComponent> CachedAbilitySystem;
};
