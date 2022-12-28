// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKartMovementComponent.h"

#include "KrazyKarts/KrazyKarts.h"


// Sets default values for this component's properties
UGoKartMovementComponent::UGoKartMovementComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UGoKartMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UGoKartMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                             FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const FGoKartMove Move = CreateMoveData(DeltaTime);
	SimulateMoveTick(Move);
}


void UGoKartMovementComponent::SimulateMoveTick(const FGoKartMove& Move)
{
	// Accumulate the moving force  
	AddMovingForce(Move);
	
	// Accumulate the tarmac friction force
	AddKineticFrictionForce();

	// Accumulate the air resistance
	AddAirResistanceForce();
	
	// Calculate acceleration from force then integrate twice to get current position
	Acceleration = AccumulatedForce / Mass;
	Velocity += Move.DeltaTime * Acceleration;
	UpdateLocation(Move);
}

FGoKartMove UGoKartMovementComponent::CreateMoveData(const float DeltaTime) const
{
	// World TimeSeconds vs. ServerWorld TimeSeconds
	// time is being used here as kind of an incremental index that’s
	// determined only by the client and compared only against the client’s “indices”.
	// https://community.gamedev.tv/t/world-timeseconds-vs-serverworld-timeseconds/73314
	FGoKartMove NewMoveData;
	NewMoveData.DeltaTime = DeltaTime;
	NewMoveData.Time = GetWorld()->TimeSeconds;
	NewMoveData.Throttle = Throttle;
	NewMoveData.SteeringThrow = SteeringThrow;
	return NewMoveData;
}

void UGoKartMovementComponent::UpdateLocation(const FGoKartMove& Move)
{
	const FVector DeltaLocation = Move.DeltaTime * Velocity * 100; // convert meters to centimeters

	FHitResult HitResult;
	GetOwner()->AddActorWorldOffset(DeltaLocation, true, &HitResult);

	// Bounce the car
	if (HitResult.IsValidBlockingHit())
	{
		Velocity *= -BounceFactor;
	}
	
	// Reset before the next tick
	AccumulatedForce = FVector::Zero();
}

void UGoKartMovementComponent::AddKineticFrictionForce()
{
	if (FVector UnitVelocity = Velocity; UnitVelocity.Normalize())
	{
		AccumulatedForce += -UnitVelocity * Mass * 9.8f * KineticFrictionCoefficient;
	}
}

void UGoKartMovementComponent::AddAirResistanceForce()
{
	if (FVector UnitVelocity = Velocity; UnitVelocity.Normalize())
	{
		const float SpeedSquared = Velocity.SizeSquared();
		AccumulatedForce += -UnitVelocity * SpeedSquared * DragCoefficient;
	}
}
 
void UGoKartMovementComponent::AddMovingForce(const FGoKartMove& Move)
{
	// Dot product to manage reverse
	// https://en.wikipedia.org/wiki/Turning_radius
	const float DeltaDistance = FVector::DotProduct(GetOwner()->GetActorForwardVector(), Velocity) * Move.DeltaTime;
	const float DeltaAngle = DeltaDistance / MinTurningRadius * Move.SteeringThrow;
	const FQuat DeltaRotation{GetOwner()->GetActorUpVector(), DeltaAngle};
	
	Velocity = DeltaRotation * Velocity;
	GetOwner()->AddActorWorldRotation(DeltaRotation);
	
	MovingForce = ThrottleForce * Move.Throttle * GetOwner()->GetActorForwardVector();
	AccumulatedForce += MovingForce;
}
