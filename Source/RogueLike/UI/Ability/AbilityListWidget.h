// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"

#include "Blueprint/UserWidget.h"

#include "AbilityListWidget.generated.h"

class UAbilityDataAsset;
class UAbilityListDataAsset;
class UAbilityWidget;
class UCanvasPanel;
class UCanvasPanelSlot;
class UPanelSlot;
class USizeBox;
class UWeaponComponent;

/**
 * Circular ability strip UI (view-only).
 * Clipped SizeBox + pooled AbilityWidget film strip; active ability stays on the bottom widget.
 * Reads WeaponComponent for display; never mutates gameplay selection.
 */
UCLASS( Abstract, Blueprintable )
class ROGUELIKE_API UAbilityListWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION( BlueprintCallable, Category = "AbilityList" )
    void InitializeAbilityList( UWeaponComponent* InWeaponComponent, UAbilityListDataAsset* InAbilityListData );

    UFUNCTION( BlueprintCallable, Category = "AbilityList" )
    void Show();

    UFUNCTION( BlueprintCallable, Category = "AbilityList" )
    void Hide();

    /** Queue circular scroll steps while visible (+ next / - previous). */
    UFUNCTION( BlueprintCallable, Category = "AbilityList" )
    void ScrollBy( int32 DeltaSteps );

    /** Snap strip to the weapon's active ability by rotating the pool (no rebuild, no animation). */
    UFUNCTION( BlueprintCallable, Category = "AbilityList" )
    void SnapToActiveAbility();

    /** Override the configured max widget count at runtime. */
    UFUNCTION( BlueprintCallable, Category = "AbilityList" )
    void SetVisibleWidgetLimit( int32 InMaxVisibleWidgets );

    /** Clear the runtime override and use the UPROPERTY default again. */
    UFUNCTION( BlueprintCallable, Category = "AbilityList" )
    void ResetVisibleWidgetLimitToDefault();

protected:
    // ------------------ Lifecycle ------------------

    virtual void NativeConstruct() override;
    virtual void NativeTick( const FGeometry& MyGeometry, float InDeltaTime ) override;

    // ------------------ Bound widgets ------------------

    UPROPERTY( BlueprintReadOnly, meta = ( BindWidget ), Category = "AbilityList" )
    TObjectPtr<USizeBox> ViewportSizeBox;

    UPROPERTY( BlueprintReadOnly, meta = ( BindWidget ), Category = "AbilityList" )
    TObjectPtr<UCanvasPanel> AbilityCanvas;

    // ------------------ Designer knobs ------------------

    UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category = "AbilityList" )
    TSubclassOf<UAbilityWidget> AbilityWidgetClass;

    /** Max widgets in the strip. Actual count = min(weapon abilities, this). */
    UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category = "AbilityList", meta = ( ClampMin = "1" ) )
    int32 MaxVisibleWidgets = 4;

    /** Distance between consecutive widget tops. */
    UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category = "AbilityList", meta = ( ClampMin = "1.0" ) )
    float SlotStride = 64.f;

    /** Strip width (SizeBox override). */
    UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category = "AbilityList", meta = ( ClampMin = "1.0" ) )
    float SlotWidth = 256.f;

    /** Widget alignment inside the strip. */
    UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category = "AbilityList" )
    TEnumAsByte<EHorizontalAlignment> SlotHorizontalAlignment = HAlign_Center;

    /** Docks this strip in its parent slot / tall root (Bottom = foot of HUD column). */
    UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category = "AbilityList" )
    TEnumAsByte<EVerticalAlignment> SlotVerticalAlignment = VAlign_Bottom;

    /** Nominal time for one scroll animation segment, in seconds. */
    UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category = "AbilityList", meta = ( ClampMin = "0.01" ) )
    float ScrollDuration = 0.2f;

    /**
     * Progress curve over the scroll (X = 0..1 time, Y = 0..1 distance covered).
     * Empty curve falls back to linear.
     */
    UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category = "AbilityList" )
    FRuntimeFloatCurve ScrollCurve;

    UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category = "AbilityList" )
    TObjectPtr<UAbilityListDataAsset> AbilityListData;

    // ------------------ Runtime state ------------------

    UPROPERTY( BlueprintReadOnly, Category = "AbilityList" )
    TWeakObjectPtr<UWeaponComponent> WeaponComponent;

    UPROPERTY( BlueprintReadOnly, Category = "AbilityList" )
    TArray<TObjectPtr<UAbilityDataAsset>> Abilities;

    UPROPERTY( BlueprintReadOnly, Category = "AbilityList" )
    int32 ActiveIndex = INDEX_NONE;

    /** min(Abilities.Num(), MaxVisibleWidgets). Drives pool size and viewport height. */
    UPROPERTY( BlueprintReadOnly, Category = "AbilityList" )
    int32 VisibleWidgetCount = 0;

    /** Pool of VisibleWidgetCount + 2; rotated while scrolling, never resized mid-scroll. */
    UPROPERTY( BlueprintReadOnly, Category = "AbilityList" )
    TArray<TObjectPtr<UAbilityWidget>> WidgetPool;

    // ------------------ Internals ------------------

    void RebuildFromWeapon();
    void RebuildStrip();
    void RebuildAbilityEntries();
    void RefreshVisibleWidgetCount();
    void SyncActiveIndexFromWeapon();
    void ClearWidgetPool();
    void EnsureWidgetPool();
    void ApplyViewportLayout();
    void ApplyPanelSlotAlignment( UPanelSlot* InSlot, float AlignX, float AlignY, bool bPinAnchors ) const;
    void ApplyWidgetSlotAlignment( UCanvasPanelSlot* CanvasSlot ) const;
    void FillWidgetPool();
    void FillWidgetSlot( int32 WidgetSlot );
    void RotateStrip( int32 Direction );
    void UpdateStripVisuals();
    void UpdateScrollAnimation( float InDeltaTime );
    void RestartScrollAnimation();
    void RestartScrollAnimationVelocityMatched();
    void ResetScrollAnimation();
    void AdvanceScrollDistance( float Distance );

    int32 WrapIndex( int32 Index ) const;
    int32 AbilityIndexForWidgetSlot( int32 WidgetSlot ) const;
    float GetStripOriginY() const;
    float GetRemainingScrollDistance() const;
    int32 GetDesiredWidgetPoolSize() const;
    int32 GetEffectiveMaxVisibleWidgets() const;
    float EvaluateScrollCurve( float NormalizedTime ) const;
    float EvaluateScrollCurveDerivative( float NormalizedTime ) const;
    float FindVelocityMatchedCurveStart( float Speed, float Distance, float Duration ) const;
    bool CanScroll() const;

private:
    // ------------------ Motion state ------------------

    /** Fractional strip motion; 0 when settled. Positive moves icons up (toward next). */
    float ScrollOffset = 0.f;

    /** Queued scroll steps not yet committed (+ next / - previous). */
    int32 PendingScrollSteps = 0;

    /** Elapsed time of the current animation segment. */
    float ScrollAnimElapsed = 0.f;

    /** Wall-clock duration of the current animation segment. */
    float ScrollAnimDuration = 0.f;

    /** Distance (in steps) this segment covers. */
    float ScrollAnimStartDistance = 0.f;

    /** How much of ScrollAnimStartDistance has already been applied. */
    float ScrollAnimCovered = 0.f;

    /** Curve time where this segment begins (0 = full curve; >0 = velocity-matched join). */
    float ScrollAnimCurveStartT = 0.f;

    /** EvaluateScrollCurve(ScrollAnimCurveStartT), cached. */
    float ScrollAnimCurveStartValue = 0.f;

    /** Last applied scroll speed in steps/sec (for velocity-matched restarts). */
    float ScrollSpeed = 0.f;

    /** Negative value means use MaxVisibleWidgets from the widget defaults. */
    int32 RuntimeMaxVisibleWidgets = INDEX_NONE;
};
