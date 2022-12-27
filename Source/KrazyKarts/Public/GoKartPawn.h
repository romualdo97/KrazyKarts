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
	float DeltaTime{0};

	UPROPERTY()
	float Time{0};
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
class KRAZYKARTS_API AGoKartPawn final : public APawn
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

	FGoKartMove SimulateLocalUpdate(const FGoKartMove& Move);
	
private:
	FGoKartMove UpdateLocation(const FGoKartMove& Move);
	void AddKineticFrictionForce();
	void AddAirResistanceForce();
	void AddMovingForce(const FGoKartMove& Move);

	// CLIENT @ INPUT
	void HandleThrottle(const FInputActionValue& ActionValue);
	void HandleBreak(const FInputActionValue& ActionValue);
	void HandleSteering(const FInputActionValue& ActionValue);

	void Throttle(float InputValue);
	void Break(float InputValue);
	void Steering(float InputValue);
	// END CLIENT @ INPUT
	
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSendMove(const FGoKartMove& Move); // Request to be execute on the server (Request started on some client)

	UFUNCTION()
	void OnReplicatedServerState(); // Called on clients
	
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

	UPROPERTY(ReplicatedUsing=OnReplicatedServerState)
	FGoKartState ServerState;
	
	FGoKartMove ClientMove{};
	FGoKartMove ServerMove{};
	FVector MovingForce{0};
	FVector AccumulatedForce{0};
	FVector Acceleration{0};
	FVector Velocity{0};
};
