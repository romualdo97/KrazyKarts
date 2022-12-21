// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKartPawn.h"

#include "EnhancedInputComponent.h"
#include "GameFramework/HUD.h"
#include "KrazyKarts/KrazyKarts.h"

// Sets default values
AGoKartPawn::AGoKartPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AGoKartPawn::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void AGoKartPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Accumulate the moving force  
	AddMovingForce(DeltaTime);
	
	// Accumulate the tarmac friction force
	AddKineticFrictionForce();

	// Accumulate the air resistance
	AddAirResistanceForce();
	
	// Calculate acceleration from force then integrate twice to get current position
	Acceleration = AccumulatedForce / Mass;
	Velocity += DeltaTime * Acceleration;	
	const FVector DeltaLocation = DeltaTime * Velocity * 100; // convert meters to centimeters
	UpdateLocation(DeltaLocation);
}

// Called to bind functionality to input
void AGoKartPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (Input == nullptr)
	{
		UE_LOG(LogKrazyKarts, Error, TEXT("Not enhanced input!!!"));
		return;
	}

	Input->BindAction(ThrottleInputAction, ETriggerEvent::Triggered, this, &ThisClass::HandleThrottle);
	Input->BindAction(ThrottleInputAction, ETriggerEvent::Completed, this, &ThisClass::HandleThrottle);
 
	Input->BindAction(BreakInputAction, ETriggerEvent::Triggered, this, &AGoKartPawn::HandleBreak);
	Input->BindAction(BreakInputAction, ETriggerEvent::Completed, this, &AGoKartPawn::HandleBreak);

	Input->BindAction(SteeringInputAction, ETriggerEvent::Triggered, this, &AGoKartPawn::HandleSteering);
	Input->BindAction(SteeringInputAction, ETriggerEvent::Completed, this, &AGoKartPawn::HandleSteering);
}

void AGoKartPawn::UpdateLocation(const FVector& DeltaLocation)
{
	FHitResult HitResult;
	AddActorWorldOffset(DeltaLocation, true, &HitResult);

	// Bounce the car
	if (HitResult.IsValidBlockingHit())
	{
		Velocity *= -BounceFactor;
		SteeringThrow = 0;
	}
	
	// Reset before the next tick
	AccumulatedForce = FVector::Zero();
}

void AGoKartPawn::AddKineticFrictionForce()
{
	if (FVector UnitVelocity = Velocity; UnitVelocity.Normalize())
	{
		AccumulatedForce += -UnitVelocity * Mass * 9.8f * KineticFrictionCoefficient;
	}
}

void AGoKartPawn::AddAirResistanceForce()
{
	if (FVector UnitVelocity = Velocity; UnitVelocity.Normalize())
	{
		const float SpeedSquared = Velocity.SizeSquared();
		AccumulatedForce += -UnitVelocity * SpeedSquared * DragCoefficient;
	}
}

void AGoKartPawn::AddMovingForce(const float DeltaTime)
{
	// Dot product to manage reverse
	// https://en.wikipedia.org/wiki/Turning_radius
	const float DeltaDistance = FVector::DotProduct(GetActorForwardVector(), Velocity) * DeltaTime;
	const float DeltaAngle = DeltaDistance / MinTurningRadius * SteeringThrow;
	const FQuat DeltaRotation{GetActorUpVector(), DeltaAngle};
	
	Velocity = DeltaRotation * Velocity;
	AddActorWorldRotation(DeltaRotation);
	AccumulatedForce += MovingForce;
}

void AGoKartPawn::HandleThrottle(const FInputActionValue& ActionValue)
{
	const float ForceMagnitude = ThrottleForce * ActionValue.Get<float>();
	MovingForce = ForceMagnitude * GetActorForwardVector();
	UE_LOG(LogKrazyKarts, Log, TEXT("Throttle! %f"), ActionValue.Get<float>());
}

void AGoKartPawn::HandleBreak(const FInputActionValue& ActionValue)
{
	const float ForceMagnitude = -ThrottleForce * ActionValue.Get<float>();
	MovingForce = ForceMagnitude * GetActorForwardVector();
	UE_LOG(LogKrazyKarts, Log, TEXT("Break! %f"), ActionValue.Get<float>());
}

void AGoKartPawn::HandleSteering(const FInputActionValue& ActionValue)
{
	SteeringThrow = ActionValue.Get<float>();
}

