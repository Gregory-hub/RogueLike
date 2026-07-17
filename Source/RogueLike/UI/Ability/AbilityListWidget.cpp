// Fill out your copyright notice in the Description page of Project Settings.

#include "RogueLike/UI/Ability/AbilityListWidget.h"

#include "Abilities/GameplayAbility.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"

#include "RogueLike/Data/AbilityDataAsset.h"
#include "RogueLike/Data/AbilityListDataAsset.h"
#include "RogueLike/Gameplay/Weapons/WeaponComponent.h"
#include "RogueLike/UI/Ability/AbilityWidget.h"

namespace AbilityListWidgetPrivate
{
    float AlignmentToAnchor( EHorizontalAlignment Align )
    {
        switch ( Align )
        {
            case HAlign_Left: return 0.f;
            case HAlign_Right: return 1.f;
            default: return 0.5f;
        }
    }

    float AlignmentToAnchor( EVerticalAlignment Align )
    {
        switch ( Align )
        {
            case VAlign_Top: return 0.f;
            case VAlign_Center: return 0.5f;
            default: return 1.f;
        }
    }
}

void UAbilityListWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if ( IsValid( ViewportSizeBox ) )
        ViewportSizeBox->SetClipping( EWidgetClipping::ClipToBounds );
    if ( IsValid( AbilityCanvas ) )
        AbilityCanvas->SetClipping( EWidgetClipping::ClipToBounds );
}

void UAbilityListWidget::NativeTick( const FGeometry& MyGeometry, float InDeltaTime )
{
    Super::NativeTick( MyGeometry, InDeltaTime );

    if ( GetVisibility() != ESlateVisibility::Collapsed && GetVisibility() != ESlateVisibility::Hidden )
        SyncAbilitiesFromWeaponIfNeeded();

    UpdateScrollAnimation( InDeltaTime );
}

void UAbilityListWidget::InitializeAbilityList( UWeaponComponent* InWeaponComponent, UAbilityListDataAsset* InAbilityListData )
{
    WeaponComponent = InWeaponComponent;
    if ( IsValid( InAbilityListData ) )
        AbilityListData = InAbilityListData;

    RebuildFromWeapon();
}

void UAbilityListWidget::Show()
{
    SnapToActiveAbility();
    SetVisibility( ESlateVisibility::SelfHitTestInvisible );
}

void UAbilityListWidget::Hide()
{
    ResetScrollAnimation();
    SetVisibility( ESlateVisibility::Collapsed );
}

void UAbilityListWidget::ScrollBy( int32 DeltaSteps )
{
    if ( DeltaSteps == 0 )
        return;

    if ( GetVisibility() == ESlateVisibility::Collapsed || GetVisibility() == ESlateVisibility::Hidden )
        return;

    SyncAbilitiesFromWeaponIfNeeded();

    if ( !CanScroll() )
        return;

    PendingScrollSteps += DeltaSteps;

    if ( GetRemainingScrollDistance() <= KINDA_SMALL_NUMBER )
    {
        SettleScrollAnimation();
        return;
    }

    AddScrollImpulse( DeltaSteps );
}

void UAbilityListWidget::SetVisibleWidgetLimit( int32 InMaxVisibleWidgets )
{
    const int32 NewLimit = FMath::Max( 1, InMaxVisibleWidgets );
    if ( RuntimeMaxVisibleWidgets == NewLimit )
        return;

    RuntimeMaxVisibleWidgets = NewLimit;
    RebuildStrip();
}

void UAbilityListWidget::ResetVisibleWidgetLimitToDefault()
{
    if ( RuntimeMaxVisibleWidgets == INDEX_NONE )
        return;

    RuntimeMaxVisibleWidgets = INDEX_NONE;
    RebuildStrip();
}

void UAbilityListWidget::SnapToActiveAbility()
{
    SyncAbilitiesFromWeaponIfNeeded();

    if ( Abilities.Num() == 0 )
        return;

    const int32 PreviousIndex = ActiveIndex;
    SyncActiveIndexFromWeapon();
    const int32 TargetIndex = ActiveIndex;

    if ( TargetIndex == INDEX_NONE )
        return;

    // First layout or pool too small to rotate — fill in place.
    if ( PreviousIndex == INDEX_NONE || GetDesiredWidgetPoolSize() < 2 )
    {
        ActiveIndex = TargetIndex;
        ResetScrollAnimation();
        FillWidgetPool();
        UpdateStripVisuals();
        return;
    }

    // Walk the film strip so RotateStrip owns ActiveIndex + edge fill.
    ActiveIndex = PreviousIndex;

    const int32 Count = Abilities.Num();
    int32 ForwardSteps = TargetIndex - PreviousIndex;
    if ( ForwardSteps < 0 )
        ForwardSteps += Count;

    if ( ForwardSteps == 0 )
    {
        ResetScrollAnimation();
        UpdateStripVisuals();
        return;
    }

    const int32 BackwardSteps = Count - ForwardSteps;
    if ( ForwardSteps <= BackwardSteps )
    {
        for ( int32 Step = 0; Step < ForwardSteps; ++Step )
            RotateStrip( 1 );
    }
    else
    {
        for ( int32 Step = 0; Step < BackwardSteps; ++Step )
            RotateStrip( -1 );
    }

    ResetScrollAnimation();
    UpdateStripVisuals();
}

void UAbilityListWidget::RebuildFromWeapon()
{
    RebuildAbilityEntries();
    SyncedWeaponAbilityCount = WeaponComponent.IsValid() ? WeaponComponent->GetAbilityCount() : 0;
    SyncActiveIndexFromWeapon();
    RebuildStrip();
}

void UAbilityListWidget::SyncAbilitiesFromWeaponIfNeeded()
{
    if ( !WeaponComponent.IsValid() )
        return;

    const int32 WeaponCount = WeaponComponent->GetAbilityCount();
    const int32 ExpectedVisible = FMath::Clamp( WeaponCount, 0, GetEffectiveMaxVisibleWidgets() );
    const int32 ExpectedPoolSize = ExpectedVisible > 0 ? ExpectedVisible + 2 : 0;

    const bool bAbilityCountMismatch = WeaponCount != SyncedWeaponAbilityCount || WeaponCount != Abilities.Num();
    const bool bVisibleCountMismatch = VisibleWidgetCount != ExpectedVisible;
    const bool bNeedsMoreWidgets = WidgetPool.Num() < ExpectedPoolSize;

    if ( !bAbilityCountMismatch && !bVisibleCountMismatch && !bNeedsMoreWidgets )
        return;

    RebuildFromWeapon();
}

void UAbilityListWidget::RebuildAbilityEntries()
{
    Abilities.Reset();

    if ( !WeaponComponent.IsValid() )
        return;

    const int32 Count = WeaponComponent->GetAbilityCount();
    Abilities.Reserve( Count );

    for ( int32 Index = 0; Index < Count; ++Index )
        Abilities.Add( ResolveDisplayAbilityData( WeaponComponent->GetAbilityByIndex( Index ) ) );
}

UAbilityDataAsset* UAbilityListWidget::ResolveDisplayAbilityData( TSubclassOf<UGameplayAbility> AbilityClass ) const
{
    if ( !IsValid( AbilityListData ) || !AbilityClass )
        return nullptr;

    const TSoftObjectPtr<UAbilityDataAsset>* FoundData = AbilityListData->Abilities.Find( AbilityClass );
    if ( FoundData == nullptr || FoundData->IsNull() )
        return nullptr;

    UAbilityDataAsset* AbilityData = FoundData->LoadSynchronous();
    return IsValid( AbilityData ) ? AbilityData : nullptr;
}

void UAbilityListWidget::SyncActiveIndexFromWeapon()
{
    if ( Abilities.Num() == 0 )
    {
        ActiveIndex = INDEX_NONE;
        return;
    }

    if ( !WeaponComponent.IsValid() )
    {
        ActiveIndex = ActiveIndex == INDEX_NONE ? 0 : WrapIndex( ActiveIndex );
        return;
    }

    const int32 WeaponIndex = WeaponComponent->GetCurrentAbilityIndex();
    ActiveIndex = Abilities.IsValidIndex( WeaponIndex ) ? WeaponIndex : 0;
}

int32 UAbilityListWidget::WrapIndex( int32 Index ) const
{
    const int32 Count = Abilities.Num();
    if ( Count <= 0 )
        return INDEX_NONE;

    const int32 Wrapped = Index % Count;
    return Wrapped < 0 ? Wrapped + Count : Wrapped;
}

bool UAbilityListWidget::CanScroll() const
{
    return Abilities.Num() > 1 && ActiveIndex != INDEX_NONE;
}

void UAbilityListWidget::RefreshVisibleWidgetCount()
{
    VisibleWidgetCount = FMath::Clamp( Abilities.Num(), 0, GetEffectiveMaxVisibleWidgets() );
}

void UAbilityListWidget::RebuildStrip()
{
    ResetScrollAnimation();
    SyncActiveIndexFromWeapon();
    RefreshVisibleWidgetCount();
    ApplyViewportLayout();
    EnsureWidgetPool();
    FillWidgetPool();
    UpdateStripVisuals();
}

void UAbilityListWidget::ClearWidgetPool()
{
    WidgetPool.Reset();

    if ( !IsValid( AbilityCanvas ) )
        return;

    for ( UWidget* Child : AbilityCanvas->GetAllChildren() )
    {
        if ( IsValid( Child ) )
            Child->RemoveFromParent();
    }
}

void UAbilityListWidget::EnsureWidgetPool()
{
    const int32 PoolSize = GetDesiredWidgetPoolSize();
    if ( !IsValid( AbilityCanvas ) || !AbilityWidgetClass || WidgetPool.Num() >= PoolSize )
        return;

    WidgetPool.Reserve( PoolSize );
    for ( int32 WidgetSlot = WidgetPool.Num(); WidgetSlot < PoolSize; ++WidgetSlot )
    {
        UAbilityWidget* Widget = CreateWidget<UAbilityWidget>( this, AbilityWidgetClass );
        if ( !IsValid( Widget ) )
            continue;

        Widget->SetVisibility( ESlateVisibility::Collapsed );

        UCanvasPanelSlot* CanvasSlot = AbilityCanvas->AddChildToCanvas( Widget );
        if ( IsValid( CanvasSlot ) )
        {
            CanvasSlot->SetAutoSize( true );
            ApplyWidgetSlotAlignment( CanvasSlot );
        }

        WidgetPool.Add( Widget );
    }
}

void UAbilityListWidget::ApplyViewportLayout()
{
    if ( IsValid( ViewportSizeBox ) )
    {
        ViewportSizeBox->SetWidthOverride( SlotWidth );

        if ( VisibleWidgetCount > 0 )
            ViewportSizeBox->SetHeightOverride( static_cast<float>( VisibleWidgetCount ) * SlotStride );
        else
            ViewportSizeBox->ClearHeightOverride();
    }

    using namespace AbilityListWidgetPrivate;
    const float AlignX = AlignmentToAnchor( SlotHorizontalAlignment.GetValue() );
    const float AlignY = AlignmentToAnchor( SlotVerticalAlignment.GetValue() );

    // Dock in HUD / parent: keep designer anchors, auto-size + alignment so stretch columns don't pin us to the top.
    ApplyPanelSlotAlignment( Slot, AlignX, AlignY, false );

    // Nested strip inside our root (skip when ViewportSizeBox is the root — same Slot).
    if ( IsValid( ViewportSizeBox ) && ViewportSizeBox->Slot != Slot )
        ApplyPanelSlotAlignment( ViewportSizeBox->Slot, AlignX, AlignY, true );
}

void UAbilityListWidget::ApplyPanelSlotAlignment( UPanelSlot* InSlot, float AlignX, float AlignY, bool bPinAnchors ) const
{
    if ( UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>( InSlot ) )
    {
        if ( bPinAnchors )
        {
            CanvasSlot->SetAnchors( FAnchors( AlignX, AlignY, AlignX, AlignY ) );
            CanvasSlot->SetPosition( FVector2D::ZeroVector );
        }

        CanvasSlot->SetAlignment( FVector2D( AlignX, AlignY ) );
        CanvasSlot->SetAutoSize( true );
        return;
    }

    if ( UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>( InSlot ) )
    {
        OverlaySlot->SetHorizontalAlignment( SlotHorizontalAlignment.GetValue() );
        OverlaySlot->SetVerticalAlignment( SlotVerticalAlignment.GetValue() );
    }
}

float UAbilityListWidget::GetStripOriginY() const
{
    const float ContentHeight = static_cast<float>( FMath::Max( VisibleWidgetCount, 0 ) ) * SlotStride;
    float AreaHeight = ContentHeight;

    // Use the SizeBox (fixed override), not the canvas — AutoSize children inflate canvas
    // desired size and would shift OriginY, revealing a buffer widget.
    if ( IsValid( ViewportSizeBox ) )
    {
        const float ViewportHeight = ViewportSizeBox->GetCachedGeometry().GetLocalSize().Y;
        if ( ViewportHeight > AreaHeight )
            AreaHeight = ViewportHeight;
    }

    const float Extra = FMath::Max( 0.f, AreaHeight - ContentHeight );
    switch ( SlotVerticalAlignment.GetValue() )
    {
        case VAlign_Top:
            return 0.f;
        case VAlign_Center:
            return Extra * 0.5f;
        default:
            return Extra;
    }
}

void UAbilityListWidget::ApplyWidgetSlotAlignment( UCanvasPanelSlot* CanvasSlot ) const
{
    if ( !IsValid( CanvasSlot ) )
        return;

    // Top-anchored Y so SlotY film-strip math parks buffers outside the clip.
    const float AnchorX = AbilityListWidgetPrivate::AlignmentToAnchor( SlotHorizontalAlignment.GetValue() );
    CanvasSlot->SetAnchors( FAnchors( AnchorX, 0.f, AnchorX, 0.f ) );
    CanvasSlot->SetAlignment( FVector2D( AnchorX, 0.f ) );
}

void UAbilityListWidget::FillWidgetPool()
{
    const int32 PoolSize = GetDesiredWidgetPoolSize();
    for ( int32 WidgetSlot = 0; WidgetSlot < PoolSize; ++WidgetSlot )
        FillWidgetSlot( WidgetSlot );

    // Parked extras (grow-only leftovers) stay collapsed and unused.
    for ( int32 WidgetSlot = PoolSize; WidgetSlot < WidgetPool.Num(); ++WidgetSlot )
    {
        if ( IsValid( WidgetPool[WidgetSlot] ) )
            WidgetPool[WidgetSlot]->SetVisibility( ESlateVisibility::Collapsed );
    }
}

void UAbilityListWidget::FillWidgetSlot( int32 WidgetSlot )
{
    if ( !WidgetPool.IsValidIndex( WidgetSlot ) || !IsValid( WidgetPool[WidgetSlot] ) )
        return;

    UAbilityWidget* Widget = WidgetPool[WidgetSlot];
    const int32 AbilityIdx = AbilityIndexForWidgetSlot( WidgetSlot );
    if ( AbilityIdx == INDEX_NONE )
        return;

    if ( !Abilities.IsValidIndex( AbilityIdx ) || !IsValid( Abilities[AbilityIdx] ) )
    {
        Widget->ShowError();
        return;
    }

    Widget->FillWidget( Abilities[AbilityIdx]->Name, Abilities[AbilityIdx]->Icon );
}

void UAbilityListWidget::RotateStrip( int32 Direction )
{
    // Only the active prefix rotates. Extras beyond VisibleWidgetCount+2 stay parked.
    const int32 PoolSize = GetDesiredWidgetPoolSize();
    if ( PoolSize < 2 || WidgetPool.Num() < PoolSize )
        return;

    if ( Direction > 0 )
    {
        ActiveIndex = WrapIndex( ActiveIndex + 1 );

        // Rotate strip up: visible widgets keep their content; only the new bottom buffer is filled.
        UAbilityWidget* Recycled = WidgetPool[0];
        for ( int32 WidgetSlot = 0; WidgetSlot < PoolSize - 1; ++WidgetSlot )
            WidgetPool[WidgetSlot] = WidgetPool[WidgetSlot + 1];
        WidgetPool[PoolSize - 1] = Recycled;
        FillWidgetSlot( PoolSize - 1 );
    }
    else if ( Direction < 0 )
    {
        ActiveIndex = WrapIndex( ActiveIndex - 1 );

        UAbilityWidget* Recycled = WidgetPool[PoolSize - 1];
        for ( int32 WidgetSlot = PoolSize - 1; WidgetSlot > 0; --WidgetSlot )
            WidgetPool[WidgetSlot] = WidgetPool[WidgetSlot - 1];
        WidgetPool[0] = Recycled;
        FillWidgetSlot( 0 );
    }
}

void UAbilityListWidget::UpdateStripVisuals()
{
    const float OriginY = GetStripOriginY();
    const bool bSettled = FMath::IsNearlyZero( ScrollOffset ) && PendingScrollSteps == 0;
    const int32 PoolSize = GetDesiredWidgetPoolSize();

    // Pool keeps both edge buffers for bidirectional recycle; only the entering side is shown.
    const int32 ScrollDir = PendingScrollSteps != 0
        ? FMath::Sign( PendingScrollSteps )
        : FMath::Sign( ScrollOffset );

    for ( int32 WidgetSlot = 0; WidgetSlot < WidgetPool.Num(); ++WidgetSlot )
    {
        UAbilityWidget* Widget = WidgetPool[WidgetSlot];
        if ( !IsValid( Widget ) )
            continue;

        if ( WidgetSlot >= PoolSize )
        {
            Widget->SetVisibility( ESlateVisibility::Collapsed );
            continue;
        }

        UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>( Widget->Slot );
        if ( !IsValid( CanvasSlot ) )
            continue;

        // Active prefix: [buffer above][VisibleWidgetCount widgets][buffer below]. Active = bottom.
        const float SlotY = OriginY + ( static_cast<float>( WidgetSlot - 1 ) - ScrollOffset ) * SlotStride;
        CanvasSlot->SetPosition( FVector2D( 0.f, SlotY ) );

        const bool bContentWidget = WidgetSlot >= 1 && WidgetSlot <= VisibleWidgetCount;
        const bool bEnteringBuffer = !bSettled && (
            ( ScrollDir > 0 && WidgetSlot == VisibleWidgetCount + 1 ) ||
            ( ScrollDir < 0 && WidgetSlot == 0 ) );

        Widget->SetVisibility( ( bContentWidget || bEnteringBuffer )
            ? ESlateVisibility::HitTestInvisible
            : ESlateVisibility::Collapsed );
    }
}

int32 UAbilityListWidget::AbilityIndexForWidgetSlot( int32 WidgetSlot ) const
{
    if ( ActiveIndex == INDEX_NONE || VisibleWidgetCount <= 0 )
        return INDEX_NONE;

    const int32 VisualIndex = WidgetSlot - 1;
    if ( VisualIndex < -1 || VisualIndex > VisibleWidgetCount )
        return INDEX_NONE;

    return WrapIndex( ActiveIndex - ( VisibleWidgetCount - 1 ) + VisualIndex );
}

float UAbilityListWidget::GetRemainingScrollDistance() const
{
    if ( PendingScrollSteps > 0 )
        return FMath::Max( 0.f, static_cast<float>( PendingScrollSteps ) - ScrollOffset );

    if ( PendingScrollSteps < 0 )
        return FMath::Max( 0.f, static_cast<float>( -PendingScrollSteps ) + ScrollOffset );

    return FMath::Abs( ScrollOffset );
}

int32 UAbilityListWidget::GetDesiredWidgetPoolSize() const
{
    return VisibleWidgetCount > 0 ? VisibleWidgetCount + 2 : 0;
}

int32 UAbilityListWidget::GetEffectiveMaxVisibleWidgets() const
{
    return RuntimeMaxVisibleWidgets > 0 ? RuntimeMaxVisibleWidgets : MaxVisibleWidgets;
}

void UAbilityListWidget::ResetScrollAnimation()
{
    PendingScrollSteps = 0;
    ScrollOffset = 0.f;
    ClearScrollMotionState();
}

void UAbilityListWidget::CommitPendingScrollSteps()
{
    while ( PendingScrollSteps > 0 )
    {
        RotateStrip( 1 );
        --PendingScrollSteps;
    }

    while ( PendingScrollSteps < 0 )
    {
        RotateStrip( -1 );
        ++PendingScrollSteps;
    }

    ScrollOffset = 0.f;
}

void UAbilityListWidget::ClearScrollMotionState()
{
    ScrollSpeed = 0.f;
    ScrollPendingImpulse = 0.f;
    ScrollTravelSinceRest = 0.f;
}

void UAbilityListWidget::SettleScrollAnimation()
{
    const bool bHadLeftover = PendingScrollSteps != 0 || !FMath::IsNearlyZero( ScrollOffset );

    CommitPendingScrollSteps();
    ClearScrollMotionState();

    const int32 IndexBeforeSync = ActiveIndex;
    SyncActiveIndexFromWeapon();
    if ( bHadLeftover || IndexBeforeSync != ActiveIndex )
        FillWidgetPool();
}

bool UAbilityListWidget::IsScrollInMotion() const
{
    return ScrollSpeed > KINDA_SMALL_NUMBER
        || ScrollPendingImpulse > KINDA_SMALL_NUMBER
        || PendingScrollSteps != 0
        || !FMath::IsNearlyZero( ScrollOffset );
}

void UAbilityListWidget::AddScrollImpulse( int32 ImpulseSteps )
{
    ScrollPendingImpulse += ScrollImpulse * static_cast<float>( FMath::Abs( ImpulseSteps ) );
}

float UAbilityListWidget::GetEasedScrollSpeed( float RemainingDistance ) const
{
    float Speed = FMath::Clamp( ScrollSpeed, 0.f, ScrollMaxSpeed );

    // Start ease-in: 0 → full over ScrollStartEaseInDistance (quadratic).
    if ( ScrollStartEaseInDistance > KINDA_SMALL_NUMBER )
    {
        const float StartT = FMath::Clamp( ScrollTravelSinceRest / ScrollStartEaseInDistance, 0.f, 1.f );
        Speed *= StartT * StartT;
    }

    // End ease-out: sqrt keeps speed up then finishes in finite time (no crawl).
    const float EaseDistance = FMath::Max( ScrollEndEaseOutDistance, KINDA_SMALL_NUMBER );
    const float EndT = FMath::Clamp( RemainingDistance / EaseDistance, 0.f, 1.f );
    Speed *= FMath::Sqrt( EndT );

    return Speed;
}

void UAbilityListWidget::AdvanceScrollDistance( float Distance )
{
    if ( Distance <= KINDA_SMALL_NUMBER )
        return;

    if ( PendingScrollSteps == 0 )
    {
        const float AbsOffset = FMath::Abs( ScrollOffset );
        if ( Distance >= AbsOffset )
            ScrollOffset = 0.f;
        else
            ScrollOffset -= FMath::Sign( ScrollOffset ) * Distance;
        return;
    }

    const float Direction = PendingScrollSteps > 0 ? 1.f : -1.f;
    ScrollOffset += Direction * Distance;

    while ( ScrollOffset >= 1.f && PendingScrollSteps > 0 )
    {
        ScrollOffset -= 1.f;
        --PendingScrollSteps;
        RotateStrip( 1 );
    }

    while ( ScrollOffset <= -1.f && PendingScrollSteps < 0 )
    {
        ScrollOffset += 1.f;
        ++PendingScrollSteps;
        RotateStrip( -1 );
    }
}

void UAbilityListWidget::UpdateScrollAnimation( float InDeltaTime )
{
    if ( !IsScrollInMotion() )
    {
        ClearScrollMotionState();
        if ( GetVisibility() != ESlateVisibility::Collapsed && GetVisibility() != ESlateVisibility::Hidden )
            UpdateStripVisuals();
        return;
    }

    if ( !CanScroll() )
    {
        ResetScrollAnimation();
        UpdateStripVisuals();
        return;
    }

    const float Remaining = GetRemainingScrollDistance();
    if ( Remaining <= KINDA_SMALL_NUMBER )
    {
        SettleScrollAnimation();
        UpdateStripVisuals();
        return;
    }

    // Apply queued impulse: instantly, or ramped by ScrollHitAcceleration (smooth start).
    if ( ScrollPendingImpulse > KINDA_SMALL_NUMBER )
    {
        if ( ScrollHitAcceleration <= KINDA_SMALL_NUMBER )
        {
            ScrollSpeed += ScrollPendingImpulse;
            ScrollPendingImpulse = 0.f;
        }
        else
        {
            const float Applied = FMath::Min( ScrollPendingImpulse, ScrollHitAcceleration * InDeltaTime );
            ScrollSpeed += Applied;
            ScrollPendingImpulse -= Applied;
        }
    }

    ScrollSpeed = FMath::Clamp( ScrollSpeed, 0.f, ScrollMaxSpeed );

    float Travel = GetEasedScrollSpeed( Remaining ) * InDeltaTime;
    if ( Travel > Remaining )
        Travel = Remaining;

    // Keep finishing even when eased speed is tiny so we never freeze near the end.
    if ( Travel <= KINDA_SMALL_NUMBER && Remaining > KINDA_SMALL_NUMBER )
        Travel = FMath::Min( Remaining, Remaining * 8.f * InDeltaTime );

    AdvanceScrollDistance( Travel );
    ScrollTravelSinceRest += Travel;

    // Coast friction after motion; ease helpers already shape output speed.
    ScrollSpeed = FMath::Max( 0.f, ScrollSpeed - ScrollFriction * InDeltaTime );

    if ( GetRemainingScrollDistance() <= KINDA_SMALL_NUMBER )
        SettleScrollAnimation();

    UpdateStripVisuals();
}
