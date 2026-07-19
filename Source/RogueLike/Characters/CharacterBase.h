// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"

#include "AbilitySystemInterface.h"

#include "CharacterBase.generated.h"

class UBasicAttributeSet;

/** Shared character base for gameplay actors that need movement, abilities, and attributes. */
UCLASS( Abstract )
class ROGUELIKE_API ACharacterBase : public ACharacter, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    ACharacterBase();

    UFUNCTION( BlueprintCallable, Category = "CharacterBase" )
    void Move( const FVector2D& Vector );

    UFUNCTION( BlueprintCallable, BlueprintPure, Category = "CharacterBase" )
    UBillboardComponent* GetBillboardComponent() const
    {
        return BillboardComponent;
    }

    UFUNCTION( BlueprintCallable, BlueprintPure, Category = "CharacterBase" )
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override
    {
        return AbilitySystemComponent;
    }
    
    UFUNCTION( BlueprintCallable, BlueprintPure, Category = "CharacterBase" )
    const TSet<FGameplayTag>& GetAllyTags() const
    {
        return AllyTags;
    }

protected:
    virtual void BeginPlay() override;

    virtual void PossessedBy( AController* NewController ) override;

    UPROPERTY( VisibleAnywhere )
    TObjectPtr<UBillboardComponent> BillboardComponent;

    UPROPERTY( VisibleAnywhere, BlueprintReadOnly, Category = "CharacterBase" )
    TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

    UPROPERTY( VisibleAnywhere, BlueprintReadOnly, Category = "CharacterBase" )
    TObjectPtr<UBasicAttributeSet> BasicAttributeSet;

    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "CharacterBase" )
    TSet<FGameplayTag> ActorTags;
    
    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "CharacterBase", meta = ( ToolTip = "Characters with this tag are not hit by this character") )
    TSet<FGameplayTag> AllyTags;
};
