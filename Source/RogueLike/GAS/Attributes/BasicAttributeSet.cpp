// Fill out your copyright notice in the Description page of Project Settings.

#include "BasicAttributeSet.h"
#include "GameplayEffectExtension.h"

UBasicAttributeSet::UBasicAttributeSet()
{
	MaxHealth = 3.0f;
	Health = MaxHealth;
}

void UBasicAttributeSet::PreAttributeChange( const FGameplayAttribute& Attribute, float& NewValue )
{
	Super::PreAttributeChange( Attribute, NewValue );

	if ( Attribute == GetHealthAttribute() )
	{
		NewValue = FMath::Clamp( NewValue, 0.0f, GetMaxHealth() );
	}
}

void UBasicAttributeSet::PostGameplayEffectExecute( const FGameplayEffectModCallbackData& Data )
{
	Super::PostGameplayEffectExecute( Data );

	if ( Data.EvaluatedData.Attribute == GetHealthAttribute() )
	{
		SetHealth( GetHealth() );
	}
}
