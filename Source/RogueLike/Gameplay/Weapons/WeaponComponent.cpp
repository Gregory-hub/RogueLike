// Fill out your copyright notice in the Description page of Project Settings.

#include "RogueLike/Gameplay/Weapons/WeaponComponent.h"

#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "WeaponComponent.h"

UWeaponComponent::UWeaponComponent()
    : CurrentAbilityIndex( INDEX_NONE ), CachedAbilitySystemComponent( nullptr )
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UWeaponComponent::OnRegister()
{
    Super::OnRegister();
    ResolveAbilitySystemComponent();
}

void UWeaponComponent::BeginPlay()
{
    Super::BeginPlay();
    ResolveAbilitySystemComponent();
}

void UWeaponComponent::EndPlay( const EEndPlayReason::Type EndPlayReason )
{
    ClearAllAbilities();
    CachedAbilitySystemComponent = nullptr;
    Super::EndPlay( EndPlayReason );
}

void UWeaponComponent::ResolveAbilitySystemComponent()
{
    if ( CachedAbilitySystemComponent != nullptr )
    {
        return;
    }

    if ( AActor* Owner = GetOwner() )
    {
        if ( IAbilitySystemInterface* AbilityInterface = Cast<IAbilitySystemInterface>( Owner ) )
        {
            CachedAbilitySystemComponent = AbilityInterface->GetAbilitySystemComponent();
        }
    }
}

UAbilitySystemComponent* UWeaponComponent::GetAbilitySystemComponent() const
{
    return CachedAbilitySystemComponent;
}

void UWeaponComponent::AddAbility( TSubclassOf<UGameplayAbility> NewAbilityClass )
{
    if ( !NewAbilityClass )
    {
        return;
    }

    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if ( !ASC )
    {
        return;
    }

    FGameplayAbilitySpec AbilitySpec( NewAbilityClass, 1, INDEX_NONE, this );
    FGameplayAbilitySpecHandle Handle = ASC->GiveAbility( AbilitySpec );
    if ( Handle.IsValid() )
    {
        Abilities.Add( Handle );

        // If this is the first ability, select it as current
        if ( CurrentAbilityIndex == INDEX_NONE )
        {
            CurrentAbilityIndex = 0;
        }
    }
}

bool UWeaponComponent::Attack( bool bAllowRemoteActivation )
{
    if ( !IsCurrentAbilityValid() )
    {
        return false;
    }

    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if ( !ASC )
    {
        return false;
    }

    const FGameplayAbilitySpecHandle& CurrentHandle = Abilities[CurrentAbilityIndex];
    if ( CurrentHandle.IsValid() )
    {
        return ASC->TryActivateAbility( CurrentHandle, bAllowRemoteActivation );
    }

    return false;
}

bool UWeaponComponent::SelectAbilityByIndex( int32 Index )
{
    if ( !Abilities.IsValidIndex( Index ) )
    {
        return false;
    }

    CurrentAbilityIndex = Index;
    return true;
}

TSubclassOf<UGameplayAbility> UWeaponComponent::SelectNextAbility()
{
    if ( Abilities.Num() == 0 )
    {
        return nullptr;
    }

    CurrentAbilityIndex = ( CurrentAbilityIndex + 1 ) % Abilities.Num();

    return GetAbilityByIndex( CurrentAbilityIndex );
}

TSubclassOf<UGameplayAbility> UWeaponComponent::SelectPreviousAbility()
{
    if ( Abilities.Num() == 0 )
    {
        return nullptr;
    }

    CurrentAbilityIndex--;
    CurrentAbilityIndex = CurrentAbilityIndex < 0 ? Abilities.Num() - 1 : CurrentAbilityIndex;

    return GetAbilityByIndex( CurrentAbilityIndex );
}

bool UWeaponComponent::RemoveAbilityByIndex( int32 Index )
{
    if ( !Abilities.IsValidIndex( Index ) )
    {
        return false;
    }

    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if ( ASC && Abilities[Index].IsValid() )
    {
        ASC->ClearAbility( Abilities[Index] );
    }

    Abilities.RemoveAt( Index );

    // Update current ability index
    if ( Abilities.Num() == 0 )
    {
        CurrentAbilityIndex = INDEX_NONE;
    }
    else
    {
        if ( CurrentAbilityIndex == Index )
        {
            // If current was removed, clamp to a valid index (prefer same numeric index if still valid)
            CurrentAbilityIndex = Index < Abilities.Num() ? Index : Abilities.Num() - 1;
        }
        else if ( Index < CurrentAbilityIndex )
        {
            // Shift current down if a prior element was removed
            --CurrentAbilityIndex;
        }
    }

    return true;
}

void UWeaponComponent::ClearAllAbilities()
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();

    if ( ASC )
    {
        for ( const FGameplayAbilitySpecHandle& Handle : Abilities )
        {
            if ( Handle.IsValid() )
            {
                ASC->ClearAbility( Handle );
            }
        }
    }

    Abilities.Empty();
    CurrentAbilityIndex = INDEX_NONE;
}

int32 UWeaponComponent::GetAbilityCount() const
{
    return Abilities.Num();
}

bool UWeaponComponent::IsCurrentAbilityValid() const
{
    return CurrentAbilityIndex >= 0 && Abilities.IsValidIndex( CurrentAbilityIndex ) && Abilities[CurrentAbilityIndex].IsValid();
}

int32 UWeaponComponent::GetCurrentAbilityIndex() const
{
    return CurrentAbilityIndex;
}

TSubclassOf<UGameplayAbility> UWeaponComponent::GetAbilityByIndex( int32 Index ) const
{
    if ( !Abilities.IsValidIndex( Index ) )
    {
        return nullptr;
    }

    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if ( !ASC )
    {
        return nullptr;
    }

    const FGameplayAbilitySpecHandle& Handle = Abilities[Index];
    if ( Handle.IsValid() )
    {
        const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle( Handle );
        if ( Spec )
        {
            return Spec->Ability->GetClass();
        }
    }

    return nullptr;
}
