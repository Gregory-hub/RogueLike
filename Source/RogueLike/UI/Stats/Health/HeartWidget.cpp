// Fill out your copyright notice in the Description page of Project Settings.

#include "HeartWidget.h"

#include "Components/Image.h"

void UHeartWidget::SetStateEmpty()
{
    if ( !IsValid( Foreground ) )
    {
        return;
    }
    Foreground->SetBrushFromTexture( nullptr );
    Foreground->SetOpacity( 0.0f );
    bIsEmpty = true;
}

void UHeartWidget::SetStateHalf()
{
    if ( !IsValid( Foreground ) )
    {
        return;
    }
    Foreground->SetBrushFromTexture( TextureHalfHeart );
    Foreground->SetOpacity( 1.0f );
    bIsEmpty = false;
}

void UHeartWidget::SetStateFull()
{
    if ( !IsValid( Foreground ) )
    {
        return;
    }
    Foreground->SetBrushFromTexture( TextureFullHeart );
    Foreground->SetOpacity( 1.0f );
    bIsEmpty = false;
}

void UHeartWidget::NativePreConstruct()
{
    Super::NativePreConstruct();

    if ( IsValid( Background ) )
    {
        Background->SetBrushFromTexture( TextureEmpty );
    }
    
    if ( IsDesignTime() )
    {
        SetStateHalf();
    }
}
