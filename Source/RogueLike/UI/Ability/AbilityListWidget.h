// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Blueprint/UserWidget.h"

#include "AbilityListWidget.generated.h"

class UAbilityDataAsset;
class UAbilityListDataAsset;
class UAbilityWidget;
class UGameplayAbility;
class UCanvasPanel;
class UCanvasPanelSlot;
class UPanelSlot;
class USizeBox;
class UWeaponComponent;

/**
 * Circular ability strip UI (view-only).
 * Clipped SizeBox + pooled AbilityWidget film strip; active ability stays on the bottom widget.
 * Reads WeaponComponent for display; never mutates gameplay selection.
 *
 * Scroll is velocity-based: ScrollBy adds impulse (instant or via ScrollHitAcceleration).
 * Optional start/end easing is distance-based (quadratic in, sqrt out).
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

    UPROPERTY( BlueprintReadOnly, meta = ( BindWidget ), Category = "AbilityList|Widgets" )
    TObjectPtr<USizeBox> ViewportSizeBox;

    UPROPERTY( BlueprintReadOnly, meta = ( BindWidget ), Category = "AbilityList|Widgets" )
    TObjectPtr<UCanvasPanel> AbilityCanvas;

    // ------------------ Layout ------------------

    UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category = "AbilityList|Layout" )
    TSubclassOf<UAbilityWidget> AbilityWidgetClass;

    /** Max widgets in the strip. Actual count = min(weapon abilities, this). */
    UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category = "AbilityList|Layout", meta = ( ClampMin = "1" ) )
    int32 MaxVisibleWidgets = 4;

    /** Distance between consecutive widget tops. */
    UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category = "AbilityList|Layout", meta = ( ClampMin = "1.0" ) )
    float SlotStride = 64.f;

    /** Strip width (SizeBox override). */
    UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category = "AbilityList|Layout", meta = ( ClampMin = "1.0" ) )
    float SlotWidth = 256.f;

    /** Widget alignment inside the strip. */
    UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category = "AbilityList|Layout" )
    TEnumAsByte<EHorizontalAlignment> SlotHorizontalAlignment = HAlign_Center;

    /** Docks this strip in its parent slot / tall root (Bottom = foot of HUD column). */
    UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category = "AbilityList|Layout" )
    TEnumAsByte<EVerticalAlignment> SlotVerticalAlignment = VAlign_Bottom;

    // ------------------ Scroll ------------------

    /** Distance (steps) over which start speed eases in from 0 (quadratic). 0 = no start ease. */
    UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category = "AbilityList|Scroll", meta = ( ClampMin = "0.0" ) )
    float ScrollStartEaseInDistance = 0.f;

    /** Velocity added per scroll notch, from rest or mid-scroll (steps/sec). */
    UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category = "AbilityList|Scroll", meta = ( ClampMin = "0.0" ) )
    float ScrollImpulse = 8.f;

    /**
     * How fast queued impulse is applied to ScrollSpeed (steps/sec^2).
     * 0 = apply the full impulse immediately.
     */
    UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category = "AbilityList|Scroll", meta = ( ClampMin = "0.0" ) )
    float ScrollHitAcceleration = 40.f;

    /**
     * Remaining distance (steps) over which end speed eases out (sqrt curve).
     * Smaller = shorter ease-out; sqrt finishes without a crawl.
     */
    UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category = "AbilityList|Scroll", meta = ( ClampMin = "0.05" ) )
    float ScrollEndEaseOutDistance = 0.35f;

    /** Velocity decay while coasting (steps/sec^2). */
    UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category = "AbilityList|Scroll", meta = ( ClampMin = "0.0" ) )
    float ScrollFriction = 6.f;

    /** Cap on scroll speed (steps/sec). */
    UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category = "AbilityList|Scroll", meta = ( ClampMin = "0.1" ) )
    float ScrollMaxSpeed = 24.f;

    // ------------------ Data ------------------

    UPROPERTY( EditDefaultsOnly, BlueprintReadOnly, Category = "AbilityList|Data" )
    TObjectPtr<UAbilityListDataAsset> AbilityListData;

    // ------------------ Runtime state ------------------

    UPROPERTY( BlueprintReadOnly, Category = "AbilityList|Runtime" )
    TWeakObjectPtr<UWeaponComponent> WeaponComponent;

    UPROPERTY( BlueprintReadOnly, Category = "AbilityList|Runtime" )
    TArray<TObjectPtr<UAbilityDataAsset>> Abilities;

    UPROPERTY( BlueprintReadOnly, Category = "AbilityList|Runtime" )
    int32 ActiveIndex = INDEX_NONE;

    /** min(Abilities.Num(), MaxVisibleWidgets). Drives pool size and viewport height. */
    UPROPERTY( BlueprintReadOnly, Category = "AbilityList|Runtime" )
    int32 VisibleWidgetCount = 0;

    /**
     * Widget pool. Active film strip is the prefix of size VisibleWidgetCount + 2;
     * any extras stay parked at the end (grow-only; never deleted on shrink).
     */
    UPROPERTY( BlueprintReadOnly, Category = "AbilityList|Runtime" )
    TArray<TObjectPtr<UAbilityWidget>> WidgetPool;

    // ------------------ Internals ------------------

    void RebuildFromWeapon();
    void RebuildStrip();
    void RebuildAbilityEntries();
    void RefreshVisibleWidgetCount();
    void SyncActiveIndexFromWeapon();
    /** Follow weapon selection changes without interrupting a matching scroll animation. */
    void SyncActiveAbilityFromWeaponIfNeeded();
    /** Rebuild strip when weapon ability count no longer matches the cached list. */
    void SyncAbilitiesFromWeaponIfNeeded();
    UAbilityDataAsset* ResolveDisplayAbilityData( TSubclassOf<UGameplayAbility> AbilityClass ) const;
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
    void AddScrollImpulse( int32 ImpulseSteps );
    void ResetScrollAnimation();
    void CommitPendingScrollSteps();
    void ClearScrollMotionState();
    void SettleScrollAnimation();
    void AdvanceScrollDistance( float Distance );

    int32 WrapIndex( int32 Index ) const;
    int32 AbilityIndexForWidgetSlot( int32 WidgetSlot ) const;
    float GetStripOriginY() const;
    float GetRemainingScrollDistance() const;
    int32 GetDesiredWidgetPoolSize() const;
    int32 GetEffectiveMaxVisibleWidgets() const;
    float GetEasedScrollSpeed( float RemainingDistance ) const;
    bool IsScrollInMotion() const;
    bool CanScroll() const;

private:
    // ------------------ Motion state ------------------

    /** Fractional strip motion; 0 when settled. Positive moves icons up (toward next). */
    float ScrollOffset = 0.f;

    /** Queued scroll steps not yet committed (+ next / - previous). */
    int32 PendingScrollSteps = 0;

    /** Current coast speed (steps/sec). */
    float ScrollSpeed = 0.f;

    /** Impulse waiting to be applied via ScrollHitAcceleration (steps/sec). */
    float ScrollPendingImpulse = 0.f;

    /** Distance traveled since the strip was last settled (for start ease-in). */
    float ScrollTravelSinceRest = 0.f;

    /** Negative value means use MaxVisibleWidgets from the widget defaults. */
    int32 RuntimeMaxVisibleWidgets = INDEX_NONE;

    /** Weapon ability count last applied to Abilities / the strip. */
    int32 SyncedWeaponAbilityCount = 0;

    /** Weapon selection last observed by the strip. */
    int32 ObservedWeaponAbilityIndex = INDEX_NONE;
};
