// Fill out your copyright notice in the Description page of Project Settings.

#include "HealthBarHearts.h"

#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"

#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "RogueLike/Characters/CharacterBase.h"
#include "RogueLike/Gameplay/GAS/Attributes/BasicAttributeSet.h"

void UHealthBarHearts::NativeConstruct()
{
    Super::NativeConstruct();

    if ( const ACharacterBase* Character = GetOwningPlayerPawn<ACharacterBase>(); IsValid( Character ) )
    {
        if ( auto* AbilitySystem = Character->GetAbilitySystemComponent(); IsValid( AbilitySystem ) )
        {
            CachedAbilitySystem = AbilitySystem;
            HealthChangedHandle = AbilitySystem->GetGameplayAttributeValueChangeDelegate( UBasicAttributeSet::GetHealthAttribute() )
                                      .AddUObject( this, &UHealthBarHearts::UpdateHealthBar );
            bool Found;
            float Health = AbilitySystem->GetGameplayAttributeValue( UBasicAttributeSet::GetHealthAttribute(), Found );
            if ( Found )
            {
                UpdateHealthBar( Health );
            }
        }
    }
}

void UHealthBarHearts::NativeDestruct()
{
    if ( UAbilitySystemComponent* AbilitySystem = CachedAbilitySystem.Get();
         IsValid( AbilitySystem ) && HealthChangedHandle.IsValid() )
    {
        AbilitySystem->GetGameplayAttributeValueChangeDelegate( UBasicAttributeSet::GetHealthAttribute() )
            .Remove( HealthChangedHandle );
        HealthChangedHandle.Reset();
    }
    CachedAbilitySystem.Reset();

    Super::NativeDestruct();
}

void UHealthBarHearts::UpdateHealthBar( const FOnAttributeChangeData& Data )
{
    UpdateHealthBar( Data.NewValue );
}

void UHealthBarHearts::UpdateHealthBar( float NewHealth )
{
    if ( HealthPerHalfHeart <= 0.f )
    {
        return;
    }

    const int NewHalfHeartNum = static_cast<int>( NewHealth / HealthPerHalfHeart );
    if ( NewHalfHeartNum < 0 )
    {
        UE_LOG( LogTemp, Error, TEXT( "Health / HealthPerHalfHeart is less that 0" ) );
        return;
    }

    if ( NewHalfHeartNum > HalfHeartNum )
    {
        HalfHeartNum += AddHeartImages( NewHalfHeartNum - HalfHeartNum );
    }
    else if ( NewHalfHeartNum < HalfHeartNum )
    {
        HalfHeartNum -= RemoveHeartImages( HalfHeartNum - NewHalfHeartNum );
    }
}

int UHealthBarHearts::AddHeartImages( int HalfHeartsToAdd )
{
    int HalfHeartsAdded = 0;

    // If last heart is half replace it with full one
    if ( !Images.IsEmpty() && IsValid( Images.Last() ) && IsValid( TextureFullHeart )
         && Cast<UTexture2D>( Images.Last()->GetBrush().GetResourceObject() ) == TextureHalfHeart )
    {
        Images.Last()->SetBrushFromTexture( TextureFullHeart, true );
        HalfHeartsAdded++;
    }

    // Add full hearts
    while ( HalfHeartsToAdd - HalfHeartsAdded > 1 )
    {
        const int NumAdded = AddHeartImage( TextureFullHeart );
        if ( NumAdded == 0 ) // add failed
        {
            return HalfHeartsAdded;
        }
        HalfHeartsAdded += NumAdded;
    }

    // Add half heart if needed
    if ( HalfHeartsToAdd - HalfHeartsAdded > 0 )
    {
        HalfHeartsAdded += AddHeartImage( TextureHalfHeart );
    }

    return HalfHeartsAdded;
}

int UHealthBarHearts::RemoveHeartImages( int HalfHeartsToRemove )
{
    // Remove full hearts
    int HalfHeartsRemoved = 0;
    while ( HalfHeartsToRemove - HalfHeartsRemoved > 1 )
    {
        const int NumberRemoved = RemoveHeartImage();
        if ( NumberRemoved == 0 ) // remove failed
        {
            break;
        }
        HalfHeartsRemoved += NumberRemoved;
    }

    if ( HalfHeartsToRemove - HalfHeartsRemoved > 0 )
    {
        if ( !Images.IsEmpty() && IsValid( Images.Last() ) && IsValid( TextureHalfHeart )
             && Cast<UTexture2D>( Images.Last()->GetBrush().GetResourceObject() ) == TextureFullHeart )
        {
            // Replace full heart with half heart
            Images.Last()->SetBrushFromTexture( TextureHalfHeart, true );
            HalfHeartsRemoved++;
        }
        else
        {
            // Remove half heart
            HalfHeartsRemoved += RemoveHeartImage();
        }
    }

    return HalfHeartsRemoved;
}

int UHealthBarHearts::AddHeartImage( UTexture2D* Texture )
{
    if ( !IsValid( Texture ) )
    {
        UE_LOG( LogTemp, Error, TEXT( "Health bar texture is invalid" ) );
        return 0;
    }

    if ( !IsValid( HorizontalBox ) )
    {
        UE_LOG( LogTemp, Error, TEXT( "HorizontalBox is invalid" ) );
        return 0;
    }

    if ( !IsValid( WidgetTree ) )
    {
        UE_LOG( LogTemp, Error, TEXT( "WidgetTree is invalid" ) );
        return 0;
    }

    UScaleBox* ScaleBox = WidgetTree->ConstructWidget<UScaleBox>( UScaleBox::StaticClass() );
    if ( !IsValid( ScaleBox ) )
    {
        UE_LOG( LogTemp, Error, TEXT( "Failed to create ScaleBox" ) );
        return 0;
    }

    ScaleBox->SetStretch( EStretch::ScaleToFitY );

    UImage* HeartImage = WidgetTree->ConstructWidget<UImage>( UImage::StaticClass() );
    if ( !IsValid( HeartImage ) )
    {
        UE_LOG( LogTemp, Error, TEXT( "Failed to create HeartImage" ) );
        return 0;
    }

    HeartImage->SetBrushFromTexture( Texture, true );

    HorizontalBox->AddChildToHorizontalBox( ScaleBox );
    ScaleBox->AddChild( HeartImage );

    Images.Add( HeartImage );

    int NumAdded = 1;
    if ( Texture == TextureFullHeart )
    {
        NumAdded = 2;
    }
    return NumAdded;
}

int UHealthBarHearts::RemoveHeartImage()
{
    if ( Images.IsEmpty() )
    {
        return 0;
    }

    if ( !IsValid( Images.Last() ) )
    {
        Images.Pop();
        return 0;
    }

    UImage* HeartImage = Images.Last();

    int NumRemoved = 1;
    if ( Cast<UTexture2D>( HeartImage->GetBrush().GetResourceObject() ) == TextureFullHeart )
    {
        NumRemoved = 2;
    }

    // remove parent scale box
    if ( auto* Parent = HeartImage->GetParent(); IsValid( Parent ) )
    {
        Parent->RemoveFromParent();
    }
    Images.Pop();

    return NumRemoved;
}
