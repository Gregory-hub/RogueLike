// Fill out your copyright notice in the Description page of Project Settings.

#include "RogueLike/UI/Ability/AbilityWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

#include "RogueLike/Data/AbilityDataAsset.h"

void UAbilityWidget::FillWidget( const FText& InAbilityName, TSoftObjectPtr<UTexture2D> InAbilityIcon )
{
    if ( InAbilityName.IsEmpty() && InAbilityIcon.IsNull() )
    {
        ShowError();
        return;
    }

    if ( IsValid( AbilityName ) )
        AbilityName->SetText( InAbilityName );

    ApplyIcon( InAbilityIcon );
}

void UAbilityWidget::ShowError()
{
    if ( IsValid( ErrorAbilityData ) )
    {
        const FText& ErrorName = ErrorAbilityData->Name;
        if ( IsValid( AbilityName ) )
            AbilityName->SetText( ErrorName.IsEmpty() ? NSLOCTEXT( "AbilityWidget", "MissingAbility", "???" ) : ErrorName );

        ApplyIcon( ErrorAbilityData->Icon );
        return;
    }

    if ( IsValid( AbilityName ) )
        AbilityName->SetText( NSLOCTEXT( "AbilityWidget", "MissingAbility", "???" ) );

    ApplyIcon( nullptr );
}

void UAbilityWidget::ApplyIcon( TSoftObjectPtr<UTexture2D> InAbilityIcon )
{
    if ( !IsValid( AbilityIcon ) )
        return;

    if ( InAbilityIcon.IsNull() )
    {
        AbilityIcon->SetBrushFromTexture( nullptr );
        // Hidden keeps layout space — Collapsed makes the name snap sideways.
        AbilityIcon->SetVisibility( ESlateVisibility::Hidden );
        return;
    }

    UTexture2D* Texture = InAbilityIcon.Get();
    if ( !IsValid( Texture ) )
        Texture = InAbilityIcon.LoadSynchronous();

    if ( IsValid( Texture ) )
    {
        AbilityIcon->SetBrushFromTexture( Texture );
        AbilityIcon->SetVisibility( ESlateVisibility::Visible );
        return;
    }

    AbilityIcon->SetBrushFromTexture( nullptr );
    AbilityIcon->SetVisibility( ESlateVisibility::Hidden );
}
