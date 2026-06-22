// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "MainPlayerController.generated.h"

struct FInputActionValue;
class UInputMappingContext;
class UInputAction;

/** Central player controller designed for blueprint-driven input mapping and enhanced input setup. */
UCLASS( Abstract )
class ROGUELIKE_API AMainPlayerController : public APlayerController
{
    GENERATED_BODY()

protected:
    virtual void BeginPlay() override;

    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "MainPlayerController" )
    TObjectPtr<UInputMappingContext> InputMappingContext_;
};
