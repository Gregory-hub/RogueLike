// Fill out your copyright notice in the Description page of Project Settings.

#include "RogueLike/Characters/CharacterBase.h"
#include "AbilitySystemComponent.h"
#include "Components/BillboardComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "RogueLike/Gameplay/GAS/Attributes/BasicAttributeSet.h"

ACharacterBase::ACharacterBase()
{
    PrimaryActorTick.bCanEverTick = false;

    // Orientation to movement
    bUseControllerRotationYaw = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator( 0.f, TNumericLimits<double>::Max(), 0.f );

    BillboardComponent_ = CreateDefaultSubobject<UBillboardComponent>( TEXT( "BillboardComponent" ) );
    BillboardComponent_->SetupAttachment( GetRootComponent() );
    BillboardComponent_->SetHiddenInGame( false );

    AbilitySystemComponent_ = CreateDefaultSubobject<UAbilitySystemComponent>( TEXT( "AbilitySystemComponent" ) );

    BasicAttributeSet_ = CreateDefaultSubobject<UBasicAttributeSet>( TEXT( "Attributes" ) );
}

void ACharacterBase::Move( const FVector2D& Vector )
{
    if ( Vector.IsZero() )
        return;

    AddMovementInput( FVector::ForwardVector, Vector.Y );
    AddMovementInput( FVector::RightVector, Vector.X );
}

void ACharacterBase::PossessedBy( AController* NewController )
{
    Super::PossessedBy( NewController );

    AbilitySystemComponent_->InitAbilityActorInfo( this, this );
}
