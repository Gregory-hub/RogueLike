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
    if ( DeltaSteps == 0 || !CanScroll() )
        return;

    if ( GetVisibility() == ESlateVisibility::Collapsed || GetVisibility() == ESlateVisibility::Hidden )
        return;

    const bool bWasAnimating = ScrollAnimStartDistance > KINDA_SMALL_NUMBER
        || PendingScrollSteps != 0
        || !FMath::IsNearlyZero( ScrollOffset );

    PendingScrollSteps += DeltaSteps;

    if ( bWasAnimating && ScrollSpeed > KINDA_SMALL_NUMBER )
        RestartScrollAnimationVelocityMatched();
    else
        RestartScrollAnimation();
}

void UAbilityListWidget::SnapToActiveAbility()
{
    if ( WidgetPool.Num() < 2 || Abilities.Num() == 0 )
        return;

    const int32 PreviousIndex = ActiveIndex;
    SyncActiveIndexFromWeapon();
    const int32 TargetIndex = ActiveIndex;

    if ( PreviousIndex == INDEX_NONE || TargetIndex == INDEX_NONE )
        return;

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
    SyncActiveIndexFromWeapon();
    RebuildStrip();
}

void UAbilityListWidget::RebuildAbilityEntries()
{
    Abilities.Reset();

    if ( !IsValid( AbilityListData ) || !WeaponComponent.IsValid() )
        return;

    for ( const TSubclassOf<UGameplayAbility>& AbilityClass : WeaponComponent->GetAbilityClasses() )
    {
        const TSoftObjectPtr<UAbilityDataAsset>* FoundData = AbilityListData->Abilities.Find( AbilityClass );
        if ( FoundData == nullptr || FoundData->IsNull() )
            continue;

        UAbilityDataAsset* AbilityData = FoundData->LoadSynchronous();
        if ( IsValid( AbilityData ) )
            Abilities.Add( AbilityData );
    }
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
    ActiveIndex = WeaponIndex == INDEX_NONE ? 0 : WrapIndex( WeaponIndex );
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

void UAbilityListWidget::RebuildStrip()
{
    ResetScrollAnimation();

    const int32 NewVisibleWidgetCount = FMath::Clamp( Abilities.Num(), 0, MaxVisibleWidgets );
    if ( NewVisibleWidgetCount != VisibleWidgetCount || WidgetPool.Num() != ( NewVisibleWidgetCount > 0 ? NewVisibleWidgetCount + 2 : 0 ) )
    {
        VisibleWidgetCount = NewVisibleWidgetCount;
        ClearWidgetPool();
    }

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
    const int32 PoolSize = VisibleWidgetCount > 0 ? VisibleWidgetCount + 2 : 0;
    if ( !IsValid( AbilityCanvas ) || !AbilityWidgetClass || WidgetPool.Num() == PoolSize )
        return;

    ClearWidgetPool();
    if ( PoolSize <= 0 )
        return;

    WidgetPool.Reserve( PoolSize );
    for ( int32 WidgetSlot = 0; WidgetSlot < PoolSize; ++WidgetSlot )
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
    for ( int32 WidgetSlot = 0; WidgetSlot < WidgetPool.Num(); ++WidgetSlot )
        FillWidgetSlot( WidgetSlot );
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
    if ( WidgetPool.Num() < 2 )
        return;

    if ( Direction > 0 )
    {
        ActiveIndex = WrapIndex( ActiveIndex + 1 );

        // Rotate strip up: visible widgets keep their content; only the new bottom buffer is filled.
        UAbilityWidget* Recycled = WidgetPool[0];
        WidgetPool.RemoveAt( 0 );
        WidgetPool.Add( Recycled );
        FillWidgetSlot( WidgetPool.Num() - 1 );
    }
    else if ( Direction < 0 )
    {
        ActiveIndex = WrapIndex( ActiveIndex - 1 );

        UAbilityWidget* Recycled = WidgetPool.Last();
        WidgetPool.RemoveAt( WidgetPool.Num() - 1 );
        WidgetPool.Insert( Recycled, 0 );
        FillWidgetSlot( 0 );
    }
}

void UAbilityListWidget::UpdateStripVisuals()
{
    const float OriginY = GetStripOriginY();
    const bool bSettled = FMath::IsNearlyZero( ScrollOffset ) && PendingScrollSteps == 0;

    // Pool keeps both edge buffers for bidirectional recycle; only the entering side is shown.
    const int32 ScrollDir = PendingScrollSteps != 0
        ? FMath::Sign( PendingScrollSteps )
        : FMath::Sign( ScrollOffset );

    for ( int32 WidgetSlot = 0; WidgetSlot < WidgetPool.Num(); ++WidgetSlot )
    {
        UAbilityWidget* Widget = WidgetPool[WidgetSlot];
        if ( !IsValid( Widget ) )
            continue;

        UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>( Widget->Slot );
        if ( !IsValid( CanvasSlot ) )
            continue;

        // Pool: [buffer above][VisibleWidgetCount widgets][buffer below]. Active = bottom widget.
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

void UAbilityListWidget::ResetScrollAnimation()
{
    PendingScrollSteps = 0;
    ScrollOffset = 0.f;
    ScrollAnimElapsed = 0.f;
    ScrollAnimDuration = 0.f;
    ScrollAnimStartDistance = 0.f;
    ScrollAnimCovered = 0.f;
    ScrollAnimCurveStartT = 0.f;
    ScrollAnimCurveStartValue = 0.f;
    ScrollSpeed = 0.f;
}

float UAbilityListWidget::EvaluateScrollCurve( float NormalizedTime ) const
{
    const float ClampedTime = FMath::Clamp( NormalizedTime, 0.f, 1.f );
    if ( const FRichCurve* RichCurve = ScrollCurve.GetRichCurveConst() )
    {
        if ( RichCurve->GetNumKeys() > 0 )
            return FMath::Clamp( RichCurve->Eval( ClampedTime ), 0.f, 1.f );
    }

    return ClampedTime;
}

float UAbilityListWidget::EvaluateScrollCurveDerivative( float NormalizedTime ) const
{
    constexpr float Eps = 1.e-3f;
    const float T0 = FMath::Clamp( NormalizedTime - Eps, 0.f, 1.f );
    const float T1 = FMath::Clamp( NormalizedTime + Eps, 0.f, 1.f );
    const float Denom = T1 - T0;
    if ( Denom <= KINDA_SMALL_NUMBER )
        return 1.f;

    return ( EvaluateScrollCurve( T1 ) - EvaluateScrollCurve( T0 ) ) / Denom;
}

float UAbilityListWidget::FindVelocityMatchedCurveStart( float Speed, float Distance, float Duration ) const
{
    // Segment maps curve t0→1 over Duration covering Distance.
    // Start speed = Distance * C'(t0) * (1-t0) / ((C(1)-C(t0)) * Duration).
    // Pick t0 whose start speed is closest to Speed; prefer Duration near ScrollDuration.
    const float C1 = EvaluateScrollCurve( 1.f );
    const float TargetSpeed = FMath::Max( Speed, KINDA_SMALL_NUMBER );

    float BestT = 0.f;
    float BestScore = MAX_flt;

    constexpr int32 SampleCount = 48;
    for ( int32 Sample = 0; Sample < SampleCount; ++Sample )
    {
        const float T0 = static_cast<float>( Sample ) / static_cast<float>( SampleCount );
        if ( T0 >= 0.98f )
            break;

        const float C0 = EvaluateScrollCurve( T0 );
        const float CurveRange = C1 - C0;
        if ( CurveRange <= KINDA_SMALL_NUMBER )
            continue;

        const float Deriv = FMath::Max( EvaluateScrollCurveDerivative( T0 ), 0.f );
        const float TimeFactor = Deriv * ( 1.f - T0 ) / CurveRange;
        if ( TimeFactor <= KINDA_SMALL_NUMBER )
            continue;

        // Duration needed for exact speed match at this t0.
        const float MatchedDuration = Distance * TimeFactor / TargetSpeed;
        const float StartSpeed = Distance * TimeFactor / FMath::Max( Duration, KINDA_SMALL_NUMBER );
        const float SpeedError = FMath::Abs( StartSpeed - TargetSpeed );
        const float DurationError = FMath::Abs( MatchedDuration - Duration ) / FMath::Max( Duration, KINDA_SMALL_NUMBER );
        const float Score = SpeedError + 0.25f * DurationError * TargetSpeed;

        if ( Score < BestScore )
        {
            BestScore = Score;
            BestT = T0;
        }
    }

    return BestT;
}

void UAbilityListWidget::RestartScrollAnimation()
{
    ScrollAnimCurveStartT = 0.f;
    ScrollAnimCurveStartValue = EvaluateScrollCurve( 0.f );
    ScrollAnimDuration = FMath::Max( ScrollDuration, KINDA_SMALL_NUMBER );
    ScrollAnimStartDistance = GetRemainingScrollDistance();
    ScrollAnimCovered = 0.f;
    ScrollAnimElapsed = 0.f;

    if ( ScrollAnimStartDistance <= KINDA_SMALL_NUMBER )
        ResetScrollAnimation();
}

void UAbilityListWidget::RestartScrollAnimationVelocityMatched()
{
    const float Remaining = GetRemainingScrollDistance();
    if ( Remaining <= KINDA_SMALL_NUMBER )
    {
        ResetScrollAnimation();
        return;
    }

    const float NominalDuration = FMath::Max( ScrollDuration, KINDA_SMALL_NUMBER );
    const float Speed = FMath::Abs( ScrollSpeed );
    const float T0 = FindVelocityMatchedCurveStart( Speed, Remaining, NominalDuration );
    const float C0 = EvaluateScrollCurve( T0 );
    const float C1 = EvaluateScrollCurve( 1.f );
    const float CurveRange = C1 - C0;
    const float Deriv = FMath::Max( EvaluateScrollCurveDerivative( T0 ), KINDA_SMALL_NUMBER );

    ScrollAnimCurveStartT = T0;
    ScrollAnimCurveStartValue = C0;
    ScrollAnimStartDistance = Remaining;
    ScrollAnimCovered = 0.f;
    ScrollAnimElapsed = 0.f;

    // Exact duration so join speed matches current speed; keep curve end at t=1.
    if ( CurveRange > KINDA_SMALL_NUMBER )
    {
        const float TimeFactor = Deriv * ( 1.f - T0 ) / CurveRange;
        const float MatchedDuration = Remaining * TimeFactor / Speed;
        ScrollAnimDuration = FMath::Clamp(
            MatchedDuration,
            NominalDuration * 0.25f,
            NominalDuration * 4.f );
    }
    else
    {
        ScrollAnimCurveStartT = 0.f;
        ScrollAnimCurveStartValue = EvaluateScrollCurve( 0.f );
        ScrollAnimDuration = NominalDuration;
    }
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
    if ( PendingScrollSteps == 0 && FMath::IsNearlyZero( ScrollOffset ) )
    {
        ResetScrollAnimation();
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

    if ( ScrollAnimStartDistance <= KINDA_SMALL_NUMBER )
        RestartScrollAnimation();

    ScrollAnimElapsed += InDeltaTime;

    const float Duration = FMath::Max( ScrollAnimDuration, KINDA_SMALL_NUMBER );
    const float LinearT = FMath::Clamp( ScrollAnimElapsed / Duration, 0.f, 1.f );
    const float CurveT = FMath::Lerp( ScrollAnimCurveStartT, 1.f, LinearT );
    const float CurveValue = EvaluateScrollCurve( CurveT );
    const float CurveRange = FMath::Max( EvaluateScrollCurve( 1.f ) - ScrollAnimCurveStartValue, KINDA_SMALL_NUMBER );
    const float SegmentT = FMath::Clamp( ( CurveValue - ScrollAnimCurveStartValue ) / CurveRange, 0.f, 1.f );
    const float TargetCovered = SegmentT * ScrollAnimStartDistance;
    const float DeltaCovered = FMath::Max( 0.f, TargetCovered - ScrollAnimCovered );
    ScrollAnimCovered = TargetCovered;

    AdvanceScrollDistance( DeltaCovered );

    if ( InDeltaTime > KINDA_SMALL_NUMBER )
        ScrollSpeed = DeltaCovered / InDeltaTime;

    if ( LinearT >= 1.f )
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

        ResetScrollAnimation();
    }

    UpdateStripVisuals();
}
