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
 * Clipped SizeBox + pooled AbilityWidget film strip; active ability stays on the bottom row.
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

    /** Snap to the weapon's current ability with no animation. */
    UFUNCTION( BlueprintCallable, Category = "AbilityList" )
    void ScrollToCurrentAbility();

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

    /** Max rows shown. Actual count is min(weapon abilities, this). */
    UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category = "AbilityList", meta = ( ClampMin = "1" ) )
    int32 VisibleCount = 4;

    /** Distance between consecutive row tops. */
    UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category = "AbilityList", meta = ( ClampMin = "1.0" ) )
    float SlotStride = 64.f;

    /** Strip width (SizeBox override). */
    UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category = "AbilityList", meta = ( ClampMin = "1.0" ) )
    float SlotWidth = 256.f;

    /** Row alignment inside the strip. */
    UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category = "AbilityList" )
    TEnumAsByte<EHorizontalAlignment> SlotHorizontalAlignment = HAlign_Center;

    /** Docks this strip in its parent slot / tall root (Bottom = foot of HUD column). */
    UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category = "AbilityList" )
    TEnumAsByte<EVerticalAlignment> SlotVerticalAlignment = VAlign_Bottom;

    /** Total time to play through the current scroll queue (any length), in seconds. */
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

    /** min(Abilities.Num(), VisibleCount). Drives pool size and viewport height. */
    UPROPERTY( BlueprintReadOnly, Category = "AbilityList" )
    int32 ShownCount = 0;

    /** Pool of ShownCount + 2; rebound while scrolling, never resized mid-scroll. */
    UPROPERTY( BlueprintReadOnly, Category = "AbilityList" )
    TArray<TObjectPtr<UAbilityWidget>> AbilityWidgets;

    // ------------------ Internals ------------------

    void RefreshFromWeapon();
    void RebuildSettledView();
    void RebuildAbilityData();
    void SyncActiveIndexFromWeapon();
    void ClearPool();
    void EnsurePool();
    void ApplyViewportLayout();
    void ApplyPanelSlotAlignment( UPanelSlot* InSlot, float AlignX, float AlignY, bool bPinAnchors ) const;
    void ApplyRowSlotAlignment( UCanvasPanelSlot* CanvasSlot ) const;
    void RebindPool();
    void RebindPoolSlot( int32 PoolSlot );
    void CommitScrollStep( int32 Direction );
    void RefreshTransforms();
    void UpdateScrollAnimation( float InDeltaTime );
    void RestartScrollAnimation();
    void ResetScrollAnimation();
    void AdvanceScrollDistance( float Distance );

    int32 WrapIndex( int32 Index ) const;
    int32 AbilityIndexForPoolSlot( int32 PoolSlot ) const;
    float GetStripOriginY() const;
    float GetRemainingScrollDistance() const;
    float EvaluateScrollCurve( float NormalizedTime ) const;
    bool CanScroll() const;

private:
    // ------------------ Motion state ------------------

    /** Fractional strip motion; 0 when settled. Positive moves icons up (toward next). */
    float ScrollOffset = 0.f;

    /** Queued scroll steps not yet committed (+ next / - previous). */
    int32 PendingScrollSteps = 0;

    /** Elapsed time of the current queue animation. */
    float ScrollAnimElapsed = 0.f;

    /** Remaining distance (in steps) when the current animation was started. */
    float ScrollAnimStartDistance = 0.f;

    /** How much of ScrollAnimStartDistance has already been applied. */
    float ScrollAnimCovered = 0.f;
};
