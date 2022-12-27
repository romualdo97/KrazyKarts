// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "GameFramework/Pawn.h"
#include "GoKartPawn.generated.h"

class UInputComponent;
class UInputAction;

USTRUCT()
struct FGoKartMove
{
	GENERATED_BODY()
	
	UPROPERTY()
	float SteeringThrow{0};

	UPROPERTY()
	float CurrentThrottle{0};

	UPROPERTY()
	float DeltaTime;

	UPROPERTY()
	float Time;
};

USTRUCT()
struct FGoKartState
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Velocity;

	UPROPERTY()
	FTransform Transform;
	
	UPROPERTY()
	FGoKartMove LastMove;
};


// https://docs.unrealengine.com/5.1/en-US/input-overview-in-unreal-engine/
// https://docs.unrealengine.com/5.1/en-US/enhanced-input-in-unreal-engine/
UCLASS(Abstract)
class KRAZYKARTS_API AGoKartPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AGoKartPawn();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void SimulateLocalUpdate(float DeltaTime);
	
private:
	void UpdateLocation(const FVector& DeltaLocation);
	void AddKineticFrictionForce();
	void AddAirResistanceForce();
	void AddMovingForce(float DeltaTime);
	
	void HandleThrottle(const FInputActionValue& ActionValue);
	void HandleBreak(const FInputActionValue& ActionValue);
	void HandleSteering(const FInputActionValue& ActionValue);

	void Throttle(float InputValue);
	void Break(float InputValue);
	void Steering(float InputValue);
	
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerThrottle(float InputValue);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerBreak(float InputValue);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSteering(float InputValue);

	UFUNCTION()
	void OnReplicatedTransform();
	
	/**
	 * Mass of the vehicle, unit is Kg (Kilograms)
	 */
	UPROPERTY(EditAnywhere, Category="Gameplay")
	float Mass{100};

	/**
	 * Magnitude of the acceleration force, unit is N (Newtons)
	 */
	UPROPERTY(EditAnywhere, Category="Gameplay")
    float ThrottleForce{100};

	/**
	 * The turning radius, unit is m (meters)
	 */
	UPROPERTY(EditAnywhere, Category="Gameplay")
	float MinTurningRadius{10};
	
	/**
	 * The constant of proportionality called the coefficient of kinetic friction. It is unitless and dimensionless
	 */
	UPROPERTY(EditAnywhere, Category="Gameplay", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float KineticFrictionCoefficient{0.5};

	/**
	 * The drag coefficient of air resistance. It is unitless and dimensionless
	 */
	UPROPERTY(EditAnywhere, Category="Gameplay", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float DragCoefficient{0.5};
	
	/**
	 * Factor to apply when reflecting the velocity
	 */
	UPROPERTY(EditAnywhere, Category="Gameplay", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float BounceFactor{0.8};
	
	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> ThrottleInputAction;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> BreakInputAction;
	
	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> SteeringInputAction;

	
	UPROPERTY(Replicated)
	float SteeringThrow{0}; // Replicate variables changed by player input to smooth simulated proxies

	UPROPERTY(Replicated)
	float CurrentThrottle{0}; // Replicate variables changed by player input to smooth simulated proxies
	
	FVector MovingForce{0};
	FVector AccumulatedForce{0};
	FVector Acceleration{0};

	UPROPERTY(Replicated)
	FVector Velocity{0};

	UPROPERTY(ReplicatedUsing=OnReplicatedTransform)
	FTransform ReplicatedTransform{};
};
