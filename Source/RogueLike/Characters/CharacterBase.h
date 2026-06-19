// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
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
		return BillboardComponent_;
	}

	UFUNCTION( BlueprintCallable, BlueprintPure, Category = "CharacterBase" )
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override
	{
		return AbilitySystemComponent_;
	}

protected:
	virtual void PossessedBy( AController* NewController ) override;

	UPROPERTY( VisibleAnywhere )
	TObjectPtr<UBillboardComponent> BillboardComponent_;

	UPROPERTY( VisibleAnywhere, BlueprintReadOnly, Category = "CharacterBase" )
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent_;

	UPROPERTY( VisibleAnywhere, BlueprintReadOnly, Category = "CharacterBase" )
	TObjectPtr<UBasicAttributeSet> BasicAttributeSet_;
};
