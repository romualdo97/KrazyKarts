// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "GameFramework/Pawn.h"
#include "GoKartMovementComponent.h"
#include "GoKartMovementReplicationComponent.h"
#include "GoKartPawn.generated.h"

class UInputComponent;
class UInputAction;

// https://docs.unrealengine.com/5.1/en-US/input-overview-in-unreal-engine/
// https://docs.unrealengine.com/5.1/en-US/enhanced-input-in-unreal-engine/
UCLASS(Abstract)
class KRAZYKARTS_API AGoKartPawn final : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AGoKartPawn();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
private:
	// CLIENT @ INPUT
	void HandleThrottle(const FInputActionValue& ActionValue);
	void HandleBreak(const FInputActionValue& ActionValue);
	void HandleSteering(const FInputActionValue& ActionValue);

	void Throttle(float InputValue);
	void Break(float InputValue);
	void Steering(float InputValue);
	// END CLIENT @ INPUT
		
	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> ThrottleInputAction;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> BreakInputAction;
	
	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> SteeringInputAction;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UGoKartMovementComponent> MovementComponent;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UGoKartMovementReplicationComponent> ReplicationComponent;
};
