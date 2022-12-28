// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GoKartMovementComponent.generated.h"

/**
 * Encapsulates data required to "simulate a move", i.e. move the actor
 */
USTRUCT()
struct FGoKartMove
{
	GENERATED_BODY()
	
	UPROPERTY()
	float SteeringThrow{0};

	UPROPERTY()
	float Throttle{0};

	UPROPERTY()
	float DeltaTime{0};

	UPROPERTY()
	float Time{0};
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class KRAZYKARTS_API UGoKartMovementComponent final : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UGoKartMovementComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	// Update actor transform from move data
	FGoKartMove CreateMoveData(float DeltaTime) const;
	void SimulateMoveTick(const FGoKartMove& Move);
	void SetSteeringThrow(const float Value) { SteeringThrow = Value; }
	void SetThrottle(const float Value) { Throttle = Value; }
	void SetVelocity(const FVector& InVelocity) { Velocity = InVelocity; }
	FVector GetVelocity() const { return Velocity; }
	
private:
	void AddKineticFrictionForce();
	void AddAirResistanceForce();
	void AddMovingForce(const FGoKartMove& Move);
	void UpdateLocation(const FGoKartMove& Move);
	
	/**
	 * Mass of the vehicle, unit is Kg (Kilograms)
	 */
	UPROPERTY(EditAnywhere, Category="Gameplay")
	float Mass{100};

	/**
	 * Magnitude of the acceleration force, unit is N (Newtons)
	 */
	UPROPERTY(EditAnywhere, Category="Gameplay")
	float ThrottleForce{1000};

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
	
	float SteeringThrow{0};
	float Throttle{0};
	FVector MovingForce{0};
	FVector AccumulatedForce{0};
	FVector Acceleration{0};
	FVector Velocity{0};
};
