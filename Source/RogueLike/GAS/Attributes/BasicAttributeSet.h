// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "BasicAttributeSet.generated.h"

/** Attributes common for all entities */
UCLASS()
class ROGUELIKE_API UBasicAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UBasicAttributeSet();

	UPROPERTY( BlueprintReadOnly, Category = "BasicAttributeSet" )
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS_BASIC( UBasicAttributeSet, Health );

	UPROPERTY( BlueprintReadOnly, Category = "BasicAttributeSet" )
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS_BASIC( UBasicAttributeSet, MaxHealth );

	virtual void PreAttributeChange( const FGameplayAttribute& Attribute, float& NewValue ) override;

	virtual void PostGameplayEffectExecute( const struct FGameplayEffectModCallbackData& Data ) override;
};
