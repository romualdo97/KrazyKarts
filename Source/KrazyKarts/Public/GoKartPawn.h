// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "GameFramework/Pawn.h"
#include "GoKartPawn.generated.h"

class UInputComponent;
class UInputAction;

// https://docs.unrealengine.com/5.1/en-US/input-overview-in-unreal-engine/
// https://docs.unrealengine.com/5.1/en-US/enhanced-input-in-unreal-engine/
UCLASS(Abstract)
class KRAZYKARTS_API AGoKartPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AGoKartPawn();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	UFUNCTION()
	void HandleThrottle(const FInputActionValue& ActionValue);

	UFUNCTION()
	void HandleBreak(const FInputActionValue& ActionValue);

	float CurrentSpeed{0};
	
	UPROPERTY(EditAnywhere, Category="Gameplay")
    float Speed{100}; // cm/s

	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> ThrottleInputAction;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> BreakInputAction;
};
