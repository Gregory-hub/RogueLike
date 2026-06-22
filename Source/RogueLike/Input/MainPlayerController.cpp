// Fill out your copyright notice in the Description page of Project Settings.

#include "RogueLike/Input/MainPlayerController.h"

#include "AbilitySystemComponent.h"
#include "EnhancedInputSubsystems.h"

void AMainPlayerController::BeginPlay()
{
    Super::BeginPlay();

    // mouse settings
    bShowMouseCursor = true;
    FInputModeGameAndUI inputMode;
    inputMode.SetLockMouseToViewportBehavior( EMouseLockMode::DoNotLock );
    inputMode.SetHideCursorDuringCapture( false );
    SetInputMode( inputMode );

    // input
    if ( UEnhancedInputLocalPlayerSubsystem* subsystem =
             ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>( GetLocalPlayer() ) )
    {
        subsystem->AddMappingContext( InputMappingContext_, 0 );
    }
}
