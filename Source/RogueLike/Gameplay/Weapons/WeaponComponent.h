// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystemInterface.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "GameplayAbilitySpecHandle.h"
#include "WeaponComponent.generated.h"

class UAbilitySystemComponent;
class UGameplayAbility;

/** Runtime weapon component intended for hands or other attachment points, managing multiple abilities with one active ability per weapon. */
UCLASS( ClassGroup = ( Custom ), meta = ( BlueprintSpawnableComponent ) )
class ROGUELIKE_API UWeaponComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWeaponComponent();

    /** Add a new ability to the weapon. */
    UFUNCTION( BlueprintCallable, Category = "Weapon" )
    void AddAbility( TSubclassOf<UGameplayAbility> NewAbilityClass );

    /** Attack using the currently selected ability. */
    UFUNCTION( BlueprintCallable, Category = "Weapon" )
    bool Attack( bool bAllowRemoteActivation = true );

    /** Select the ability at the given index as the current ability. */
    UFUNCTION( BlueprintCallable, Category = "Weapon" )
    bool SelectAbilityByIndex( int32 Index );

    /** Select the next ability in the list, cycling back to the beginning if at the end. */
    UFUNCTION( BlueprintCallable, Category = "Weapon" )
    bool SelectNextAbility();

    /** Select the previous ability in the list, cycling back to the end if at the beginning. */
    UFUNCTION( BlueprintCallable, Category = "Weapon" )
    bool SelectPreviousAbility();

    /** Remove the ability at the given index. */
    UFUNCTION( BlueprintCallable, Category = "Weapon" )
    bool RemoveAbilityByIndex( int32 Index );

    /** Remove all abilities from this weapon. */
    UFUNCTION( BlueprintCallable, Category = "Weapon" )
    void ClearAllAbilities();

    /** Get the number of abilities stored in this weapon. */
    UFUNCTION( BlueprintPure, Category = "Weapon" )
    int32 GetAbilityCount() const;

    /** Check if the current ability is valid and can be used. */
    UFUNCTION( BlueprintPure, Category = "Weapon" )
    bool IsCurrentAbilityValid() const;

    /** Get the index of the currently selected ability. */
    UFUNCTION( BlueprintPure, Category = "Weapon" )
    int32 GetCurrentAbilityIndex() const;

protected:
    virtual void OnRegister() override;
    virtual void BeginPlay() override;
    virtual void EndPlay( const EEndPlayReason::Type EndPlayReason ) override;

    UAbilitySystemComponent* GetAbilitySystemComponent();

private:
    void ResolveAbilitySystemComponent();

    UPROPERTY( Transient )
    TArray<FGameplayAbilitySpecHandle> Abilities;

    UPROPERTY( Transient )
    int32 CurrentAbilityIndex;

    UPROPERTY()
    UAbilitySystemComponent* CachedAbilitySystemComponent;
};
