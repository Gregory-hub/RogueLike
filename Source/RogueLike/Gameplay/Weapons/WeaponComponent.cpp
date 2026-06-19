// Fill out your copyright notice in the Description page of Project Settings.

#include "RogueLike/Gameplay/Weapons/WeaponComponent.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"

UWeaponComponent::UWeaponComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    AbilityHandle = FGameplayAbilitySpecHandle();
    CachedAbilitySystemComponent = nullptr;
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
    GrantAssignedAbility();
}

void UWeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ClearCurrentAbilityInternal();
    Super::EndPlay(EndPlayReason);
}

void UWeaponComponent::ResolveAbilitySystemComponent()
{
    if (CachedAbilitySystemComponent != nullptr)
    {
        return;
    }

    if (AActor* Owner = GetOwner())
    {
        if (IAbilitySystemInterface* AbilityInterface = Cast<IAbilitySystemInterface>(Owner))
        {
            CachedAbilitySystemComponent = AbilityInterface->GetAbilitySystemComponent();
        }
    }
}

UAbilitySystemComponent* UWeaponComponent::GetAbilitySystemComponent()
{
    ResolveAbilitySystemComponent();
    return CachedAbilitySystemComponent;
}

void UWeaponComponent::GrantAssignedAbility()
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || !AbilityClass)
    {
        return;
    }

    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (!ASC)
    {
        return;
    }

    if (AbilityHandle.IsValid())
    {
        return;
    }

    FGameplayAbilitySpec AbilitySpec(AbilityClass, 1, INDEX_NONE, this);
    AbilityHandle = ASC->GiveAbility(AbilitySpec);
}

void UWeaponComponent::ClearCurrentAbilityInternal()
{
    if (AbilityHandle.IsValid())
    {
        if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
        {
            ASC->ClearAbility(AbilityHandle);
        }

        AbilityHandle = FGameplayAbilitySpecHandle();
    }
}

void UWeaponComponent::GiveAbility(TSubclassOf<UGameplayAbility> NewAbilityClass)
{
    if (AbilityClass == NewAbilityClass)
    {
        return;
    }

    ClearCurrentAbilityInternal();
    AbilityClass = NewAbilityClass;
    GrantAssignedAbility();
}

bool UWeaponComponent::ActivateAbility(bool bAllowRemoteActivation)
{
    {
        return false;
    }

    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (!ASC)
    {
        return false;
    }

    if (AbilityHandle.IsValid())
    {
        return ASC->TryActivateAbility(AbilityHandle, bAllowRemoteActivation);
    }

    return false;
}

void UWeaponComponent::RemoveAbility()
{
    ClearCurrentAbilityInternal();
    AbilityClass = nullptr;
}

bool UWeaponComponent::HasAbilityAssigned() const
{
    return AbilityClass != nullptr && AbilityHandle.IsValid();
}

TSubclassOf<UGameplayAbility> UWeaponComponent::GetAbilityClass() const
{
    return AbilityClass;
}
