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

    RefreshFromWeapon();
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

    RefreshFromWeapon();
}

void UAbilityListWidget::Show()
{
    RefreshFromWeapon();
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

    PendingScrollSteps += DeltaSteps;
    RestartScrollAnimation();
}

void UAbilityListWidget::ScrollToCurrentAbility()
{
    SyncActiveIndexFromWeapon();
    RebuildSettledView();
}

void UAbilityListWidget::RefreshFromWeapon()
{
    RebuildAbilityData();
    SyncActiveIndexFromWeapon();
    RebuildSettledView();
}

void UAbilityListWidget::RebuildAbilityData()
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

void UAbilityListWidget::RebuildSettledView()
{
    ResetScrollAnimation();

    const int32 NewShownCount = FMath::Clamp( Abilities.Num(), 0, VisibleCount );
    if ( NewShownCount != ShownCount || AbilityWidgets.Num() != ( NewShownCount > 0 ? NewShownCount + 2 : 0 ) )
    {
        ShownCount = NewShownCount;
        ClearPool();
    }

    ApplyViewportLayout();
    EnsurePool();
    RebindPool();
    RefreshTransforms();
}

void UAbilityListWidget::ClearPool()
{
    AbilityWidgets.Reset();

    if ( !IsValid( AbilityCanvas ) )
        return;

    for ( UWidget* Child : AbilityCanvas->GetAllChildren() )
    {
        if ( IsValid( Child ) )
            Child->RemoveFromParent();
    }
}

void UAbilityListWidget::EnsurePool()
{
    const int32 PoolSize = ShownCount > 0 ? ShownCount + 2 : 0;
    if ( !IsValid( AbilityCanvas ) || !AbilityWidgetClass || AbilityWidgets.Num() == PoolSize )
        return;

    ClearPool();
    if ( PoolSize <= 0 )
        return;

    AbilityWidgets.Reserve( PoolSize );
    for ( int32 PoolSlot = 0; PoolSlot < PoolSize; ++PoolSlot )
    {
        UAbilityWidget* Widget = CreateWidget<UAbilityWidget>( this, AbilityWidgetClass );
        if ( !IsValid( Widget ) )
            continue;

        Widget->SetVisibility( ESlateVisibility::Collapsed );

        UCanvasPanelSlot* CanvasSlot = AbilityCanvas->AddChildToCanvas( Widget );
        if ( IsValid( CanvasSlot ) )
        {
            CanvasSlot->SetAutoSize( true );
            ApplyRowSlotAlignment( CanvasSlot );
        }

        AbilityWidgets.Add( Widget );
    }
}

void UAbilityListWidget::ApplyViewportLayout()
{
    if ( IsValid( ViewportSizeBox ) )
    {
        ViewportSizeBox->SetWidthOverride( SlotWidth );

        if ( ShownCount > 0 )
            ViewportSizeBox->SetHeightOverride( static_cast<float>( ShownCount ) * SlotStride );
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
    const float ContentHeight = static_cast<float>( FMath::Max( ShownCount, 0 ) ) * SlotStride;
    float AreaHeight = ContentHeight;

    // Use the SizeBox (fixed override), not the canvas — AutoSize children inflate canvas
    // desired size and would shift OriginY, revealing a buffer row.
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

void UAbilityListWidget::ApplyRowSlotAlignment( UCanvasPanelSlot* CanvasSlot ) const
{
    if ( !IsValid( CanvasSlot ) )
        return;

    // Top-anchored Y so SlotY film-strip math parks buffers outside the clip.
    const float AnchorX = AbilityListWidgetPrivate::AlignmentToAnchor( SlotHorizontalAlignment.GetValue() );
    CanvasSlot->SetAnchors( FAnchors( AnchorX, 0.f, AnchorX, 0.f ) );
    CanvasSlot->SetAlignment( FVector2D( AnchorX, 0.f ) );
}

void UAbilityListWidget::RebindPool()
{
    for ( int32 PoolSlot = 0; PoolSlot < AbilityWidgets.Num(); ++PoolSlot )
        RebindPoolSlot( PoolSlot );
}

void UAbilityListWidget::RebindPoolSlot( int32 PoolSlot )
{
    if ( !AbilityWidgets.IsValidIndex( PoolSlot ) || !IsValid( AbilityWidgets[PoolSlot] ) )
        return;

    UAbilityWidget* Widget = AbilityWidgets[PoolSlot];
    const int32 AbilityIdx = AbilityIndexForPoolSlot( PoolSlot );
    if ( AbilityIdx == INDEX_NONE )
        return;

    if ( !Abilities.IsValidIndex( AbilityIdx ) || !IsValid( Abilities[AbilityIdx] ) )
    {
        Widget->ShowError();
        return;
    }

    Widget->FillWidget( Abilities[AbilityIdx]->Name, Abilities[AbilityIdx]->Icon );
}

void UAbilityListWidget::CommitScrollStep( int32 Direction )
{
    if ( AbilityWidgets.Num() < 2 )
        return;

    if ( Direction > 0 )
    {
        ActiveIndex = WrapIndex( ActiveIndex + 1 );

        // Rotate strip up: visible rows keep their widgets/content; only the new bottom buffer is filled.
        UAbilityWidget* Recycled = AbilityWidgets[0];
        AbilityWidgets.RemoveAt( 0 );
        AbilityWidgets.Add( Recycled );
        RebindPoolSlot( AbilityWidgets.Num() - 1 );
    }
    else if ( Direction < 0 )
    {
        ActiveIndex = WrapIndex( ActiveIndex - 1 );

        UAbilityWidget* Recycled = AbilityWidgets.Last();
        AbilityWidgets.RemoveAt( AbilityWidgets.Num() - 1 );
        AbilityWidgets.Insert( Recycled, 0 );
        RebindPoolSlot( 0 );
    }
}

void UAbilityListWidget::RefreshTransforms()
{
    const float OriginY = GetStripOriginY();
    const bool bSettled = FMath::IsNearlyZero( ScrollOffset ) && PendingScrollSteps == 0;

    // Pool keeps both edge buffers for bidirectional recycle; only the entering side is shown.
    const int32 ScrollDir = PendingScrollSteps != 0
        ? FMath::Sign( PendingScrollSteps )
        : FMath::Sign( ScrollOffset );

    for ( int32 PoolSlot = 0; PoolSlot < AbilityWidgets.Num(); ++PoolSlot )
    {
        UAbilityWidget* Widget = AbilityWidgets[PoolSlot];
        if ( !IsValid( Widget ) )
            continue;

        UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>( Widget->Slot );
        if ( !IsValid( CanvasSlot ) )
            continue;

        // Pool: [buffer above][ShownCount rows][buffer below]. Active = bottom row.
        const float SlotY = OriginY + ( static_cast<float>( PoolSlot - 1 ) - ScrollOffset ) * SlotStride;
        CanvasSlot->SetPosition( FVector2D( 0.f, SlotY ) );

        const bool bContentRow = PoolSlot >= 1 && PoolSlot <= ShownCount;
        const bool bEnteringBuffer = !bSettled && (
            ( ScrollDir > 0 && PoolSlot == ShownCount + 1 ) ||
            ( ScrollDir < 0 && PoolSlot == 0 ) );

        Widget->SetVisibility( ( bContentRow || bEnteringBuffer )
            ? ESlateVisibility::HitTestInvisible
            : ESlateVisibility::Collapsed );
    }
}

int32 UAbilityListWidget::AbilityIndexForPoolSlot( int32 PoolSlot ) const
{
    if ( ActiveIndex == INDEX_NONE || ShownCount <= 0 )
        return INDEX_NONE;

    const int32 VisualRow = PoolSlot - 1;
    if ( VisualRow < -1 || VisualRow > ShownCount )
        return INDEX_NONE;

    return WrapIndex( ActiveIndex - ( ShownCount - 1 ) + VisualRow );
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
    ScrollAnimStartDistance = 0.f;
    ScrollAnimCovered = 0.f;
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

void UAbilityListWidget::RestartScrollAnimation()
{
    ScrollAnimStartDistance = GetRemainingScrollDistance();
    ScrollAnimCovered = 0.f;
    ScrollAnimElapsed = 0.f;

    if ( ScrollAnimStartDistance <= KINDA_SMALL_NUMBER )
        ResetScrollAnimation();
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
        CommitScrollStep( 1 );
    }

    while ( ScrollOffset <= -1.f && PendingScrollSteps < 0 )
    {
        ScrollOffset += 1.f;
        ++PendingScrollSteps;
        CommitScrollStep( -1 );
    }
}

void UAbilityListWidget::UpdateScrollAnimation( float InDeltaTime )
{
    if ( PendingScrollSteps == 0 && FMath::IsNearlyZero( ScrollOffset ) )
    {
        ResetScrollAnimation();
        if ( GetVisibility() != ESlateVisibility::Collapsed && GetVisibility() != ESlateVisibility::Hidden )
            RefreshTransforms();
        return;
    }

    if ( !CanScroll() )
    {
        ResetScrollAnimation();
        RefreshTransforms();
        return;
    }

    if ( ScrollAnimStartDistance <= KINDA_SMALL_NUMBER )
        RestartScrollAnimation();

    ScrollAnimElapsed += InDeltaTime;

    const float Duration = FMath::Max( ScrollDuration, KINDA_SMALL_NUMBER );
    const float LinearT = FMath::Clamp( ScrollAnimElapsed / Duration, 0.f, 1.f );
    const float CurvedT = EvaluateScrollCurve( LinearT );
    const float TargetCovered = CurvedT * ScrollAnimStartDistance;
    const float DeltaCovered = FMath::Max( 0.f, TargetCovered - ScrollAnimCovered );
    ScrollAnimCovered = TargetCovered;

    AdvanceScrollDistance( DeltaCovered );

    if ( LinearT >= 1.f )
    {
        while ( PendingScrollSteps > 0 )
        {
            CommitScrollStep( 1 );
            --PendingScrollSteps;
        }

        while ( PendingScrollSteps < 0 )
        {
            CommitScrollStep( -1 );
            ++PendingScrollSteps;
        }

        ResetScrollAnimation();
    }

    RefreshTransforms();
}
