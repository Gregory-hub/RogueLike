// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayAbilitySpecHandle.h"
#include "WeaponComponent.generated.h"

class UAbilitySystemComponent;
class UGameplayAbility;

/** Runtime weapon component intended for hands or other attachment points, allowing one active ability per weapon. */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ROGUELIKE_API UWeaponComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWeaponComponent();

    UFUNCTION(BlueprintCallable, Category="Weapon")
    void GiveAbility(TSubclassOf<UGameplayAbility> NewAbilityClass);

    UFUNCTION(BlueprintCallable, Category="Weapon")
    bool ActivateAbility(bool bAllowRemoteActivation = true);

    UFUNCTION(BlueprintCallable, Category="Weapon")
    void RemoveAbility();

    UFUNCTION(BlueprintPure, Category="Weapon")
    bool HasAbilityAssigned() const;

    UFUNCTION(BlueprintPure, Category="Weapon")
    TSubclassOf<UGameplayAbility> GetAbilityClass() const;

protected:
    virtual void OnRegister() override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UAbilitySystemComponent* GetAbilitySystemComponent();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability Socket")
    TSubclassOf<UGameplayAbility> AbilityClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ability Socket")
    FGameplayAbilitySpecHandle AbilityHandle;

private:
    void ResolveAbilitySystemComponent();
    void GrantAssignedAbility();
    void ClearCurrentAbilityInternal();

    UPROPERTY()
    UAbilitySystemComponent* CachedAbilitySystemComponent;
};
