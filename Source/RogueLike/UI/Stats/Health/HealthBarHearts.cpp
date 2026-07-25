// Fill out your copyright notice in the Description page of Project Settings.

#include "HealthBarHearts.h"

#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"

#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "HeartWidget.h"
#include "RogueLike/Characters/CharacterBase.h"
#include "RogueLike/Gameplay/GAS/Attributes/BasicAttributeSet.h"

void UHealthBarHearts::NativePreConstruct()
{
    Super::NativePreConstruct();

    if ( IsDesignTime() )
    {
        if ( IsValid( HorizontalBox ) )
        {
            HorizontalBox->ClearChildren();
        }
        

        Hearts.Reset();
        UpdateMaxHealth( PreviewHealth );
        UpdateCurrentHealth( PreviewHealth );
    }
}

void UHealthBarHearts::NativeConstruct()
{
    Super::NativeConstruct();

    if ( const ACharacterBase* Character = GetOwningPlayerPawn<ACharacterBase>(); IsValid( Character ) )
    {
        if ( auto* AbilitySystem = Character->GetAbilitySystemComponent(); IsValid( AbilitySystem ) )
        {
            CachedAbilitySystem = AbilitySystem;

            auto& OnHealthChanged = AbilitySystem->GetGameplayAttributeValueChangeDelegate( UBasicAttributeSet::GetHealthAttribute() );
            HealthChangedHandle = OnHealthChanged.AddUObject( this, &UHealthBarHearts::UpdateCurrentHealth );

            auto& OnMaxHealthChanged = AbilitySystem->GetGameplayAttributeValueChangeDelegate( UBasicAttributeSet::GetMaxHealthAttribute() );
            MaxHealthChangedHandle = OnMaxHealthChanged.AddUObject( this, &UHealthBarHearts::UpdateMaxHealth );

            bool Found;
            float MaxHealth = AbilitySystem->GetGameplayAttributeValue( UBasicAttributeSet::GetMaxHealthAttribute(), Found );
            if ( Found )
            {
                UpdateMaxHealth( MaxHealth );
            }
            float Health = AbilitySystem->GetGameplayAttributeValue( UBasicAttributeSet::GetHealthAttribute(), Found );
            if ( Found )
            {
                UpdateCurrentHealth( Health );
            }
        }
    }
}

void UHealthBarHearts::NativeDestruct()
{
    if ( UAbilitySystemComponent* AbilitySystem = CachedAbilitySystem.Get() )
    {
        if ( IsValid( AbilitySystem ) )
        {
            if ( HealthChangedHandle.IsValid() )
            {
                AbilitySystem->GetGameplayAttributeValueChangeDelegate( UBasicAttributeSet::GetHealthAttribute() )
                    .Remove( HealthChangedHandle );
                HealthChangedHandle.Reset();
            }

            if ( MaxHealthChangedHandle.IsValid() )
            {
                AbilitySystem->GetGameplayAttributeValueChangeDelegate( UBasicAttributeSet::GetMaxHealthAttribute() )
                    .Remove( MaxHealthChangedHandle );
                MaxHealthChangedHandle.Reset();
            }
        }
    }
    CachedAbilitySystem.Reset();

    Super::NativeDestruct();
}

void UHealthBarHearts::UpdateCurrentHealth( const FOnAttributeChangeData& Data )
{
    UpdateCurrentHealth( Data.NewValue );
}

void UHealthBarHearts::UpdateCurrentHealth( float NewHealth )
{
    if ( HealthPerHalfHeart <= 0.f )
    {
        return;
    }

    const int NewHalfHeartNum = static_cast<int>( NewHealth / HealthPerHalfHeart );
    if ( NewHalfHeartNum < 0 )
    {
        UE_LOG( LogTemp, Error, TEXT( "NewHalfHeartNum is less that 0" ) );
        return;
    }

    UpdateFilledHearts( NewHalfHeartNum );
}

void UHealthBarHearts::UpdateMaxHealth( const FOnAttributeChangeData& Data )
{
    UpdateMaxHealth( Data.NewValue );
}

void UHealthBarHearts::UpdateMaxHealth( float NewHealth )
{
    if ( HealthPerHalfHeart <= 0.f )
    {
        return;
    }

    int HalfHearts = FMath::FloorToInt( NewHealth / HealthPerHalfHeart );
    const int NewHeartNum = FMath::CeilToInt( StaticCast<float>( HalfHearts ) / 2.0f );
    if ( NewHeartNum < 0 )
    {
        UE_LOG( LogTemp, Error, TEXT( "NewHalfHeartNum is less that 0" ) );
        return;
    }

    UpdateMaxHearts( NewHeartNum );
}

void UHealthBarHearts::UpdateMaxHearts( int MaxHearts )
{
    if ( MaxHearts <= 0 )
    {
        Hearts.Reset();
        return;
    }

    int HeartsToAdd = MaxHearts - Hearts.Num();
    if ( HeartsToAdd > 0 )
    {
        PushHearts( HeartsToAdd );
    }
    else if ( HeartsToAdd < 0 )
    {
        PopHearts( -HeartsToAdd );
    }
}

void UHealthBarHearts::PushHearts( int Count )
{
    if ( Count <= 0 )
    {
        return;
    }

    Hearts.Reserve( Hearts.Num() + Count );
    for ( int i = 0; i < Count; ++i )
    {
        PushHeart();
    }
}

void UHealthBarHearts::PopHearts( int Count )
{
    if ( Hearts.Num() - Count <= 0 )
    {
        if ( IsValid( HorizontalBox ) )
        {
            HorizontalBox->ClearChildren();
        }
        Hearts.Reset();
    }

    for ( int i = 0; i < Count; ++i )
    {
        PopHeart();
    }
}

void UHealthBarHearts::PushHeart()
{
    if ( !IsValid( HorizontalBox ) )
    {
        UE_LOG( LogTemp, Error, TEXT( "HorizontalBox is invalid" ) );
        return;
    }

    if ( !IsValid( WidgetTree ) )
    {
        UE_LOG( LogTemp, Error, TEXT( "WidgetTree is invalid" ) );
        return;
    }

    if ( !IsValid( HeartWidgetClass ) )
    {
        return;
    }

    UHeartWidget* Heart = WidgetTree->ConstructWidget<UHeartWidget>( HeartWidgetClass );
    if ( !IsValid( Heart ) )
    {
        UE_LOG( LogTemp, Error, TEXT( "Failed to create Heart" ) );
        return;
    }

    UHorizontalBoxSlot* BoxSlot = HorizontalBox->AddChildToHorizontalBox( Heart );
    if ( IsValid( BoxSlot ) )
    {
        BoxSlot->SetPadding( HeartPadding );
    }

    Hearts.Add( Heart );
}

void UHealthBarHearts::PopHeart()
{
    if ( Hearts.IsEmpty() )
    {
        return;
    }

    if ( !IsValid( Hearts.Last() ) )
    {
        Hearts.Pop();
        return;
    }

    UHeartWidget* Heart = Hearts.Last();
    if ( IsValid( Heart ) )
    {
        Heart->RemoveFromParent();
    }
    Hearts.Pop();
}

void UHealthBarHearts::UpdateFilledHearts( int NewHalfHearts )
{
    if ( Hearts.IsEmpty() || NewHalfHearts < 0 )
    {
        return;
    }
    
    NewHalfHearts = FMath::Min( NewHalfHearts, Hearts.Num() * 2 );

    int i = 0;
    while ( i < NewHalfHearts / 2 )
    {
        if ( IsValid( Hearts[i] ) )
        {
            Hearts[i]->SetStateFull();
        }
        i++;
    }

    if ( NewHalfHearts % 2 == 1 )
    {
        if ( IsValid( Hearts[i] ) )
        {
            Hearts[i]->SetStateHalf();
        }
        i++;
    }

    while ( i < Hearts.Num() && !Hearts[i]->IsEmpty() )
    {
        if ( IsValid( Hearts[i] ) )
        {
            Hearts[i]->SetStateEmpty();
        }
        i++;
    }
}
