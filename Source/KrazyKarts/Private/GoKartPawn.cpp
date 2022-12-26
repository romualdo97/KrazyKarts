// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKartPawn.h"

#include "EnhancedInputComponent.h"
#include "GameFramework/HUD.h"
#include "KrazyKarts/KrazyKarts.h"
#include "Net/UnrealNetwork.h"

void DebugActorRole(AGoKartPawn* Actor, float DeltaTime)
{
	FString RoleNameString{"None Role"};
	switch (Actor->GetLocalRole()) { case ROLE_None: break;
	case ROLE_SimulatedProxy: RoleNameString = "SimulatedProxy"; break;
	case ROLE_AutonomousProxy: RoleNameString = "AutonomousProxy"; break;
	case ROLE_Authority: RoleNameString = "Authority"; break;
	case ROLE_MAX: break;
	default: ;
	}

	DrawDebugString(Actor->GetWorld(), FVector::UpVector * 80, RoleNameString, Actor, FColor::White, DeltaTime);
}

// Sets default values
AGoKartPawn::AGoKartPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
}

void AGoKartPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGoKartPawn, ReplicatedLocation);
	DOREPLIFETIME(AGoKartPawn, ReplicatedRotation);
}

// Called when the game starts or when spawned
void AGoKartPawn::BeginPlay()
{
	Super::BeginPlay();
	// NetUpdateFrequency
}

// Called every frame
void AGoKartPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	DebugActorRole(this, DeltaTime);

	if (HasAuthority())
	{
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
	else
	{
		SetActorLocation(ReplicatedLocation);
		SetActorRotation(ReplicatedRotation);
	}
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
	ReplicatedLocation = GetActorLocation();
	
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
	ReplicatedRotation = GetActorRotation();
	AccumulatedForce += MovingForce;
}

// ===================================================
// THROTTLE

void AGoKartPawn::HandleThrottle(const FInputActionValue& ActionValue)
{
	Throttle(ActionValue.Get<float>());
	UE_LOG(LogKrazyKarts, Log, TEXT("Throttle! %f"), ActionValue.Get<float>());
}

void AGoKartPawn::Throttle(float InputValue)
{
	// If can bind to input it also means that this actor
	// is an autonomous proxy
	const float ForceMagnitude = ThrottleForce * InputValue;
	MovingForce = ForceMagnitude * GetActorForwardVector();
	ServerThrottle(InputValue);
}

bool AGoKartPawn::ServerThrottle_Validate(const float InputValue)
{
	return InputValue >= 0.0f && InputValue <= 1.0f;
}

void AGoKartPawn::ServerThrottle_Implementation(const float InputValue)
{
	const float ForceMagnitude = ThrottleForce * InputValue;
	MovingForce = ForceMagnitude * GetActorForwardVector();
}

// ===================================================
// BREAK

void AGoKartPawn::HandleBreak(const FInputActionValue& ActionValue)
{
	Break(ActionValue.Get<float>());
	UE_LOG(LogKrazyKarts, Log, TEXT("Break! %f"), ActionValue.Get<float>());
}

void AGoKartPawn::Break(float InputValue)
{
	const float ForceMagnitude = -ThrottleForce * InputValue;
	MovingForce = ForceMagnitude * GetActorForwardVector();
	ServerBreak(InputValue);
}

bool AGoKartPawn::ServerBreak_Validate(const float InputValue)
{
	return InputValue >= 0.0f && InputValue <= 1.0f;
}

void AGoKartPawn::ServerBreak_Implementation(const float InputValue)
{
	const float ForceMagnitude = -ThrottleForce * InputValue;
	MovingForce = ForceMagnitude * GetActorForwardVector();
}

// ===================================================
// STEERING

void AGoKartPawn::HandleSteering(const FInputActionValue& ActionValue)
{
	Steering(ActionValue.Get<float>());
}

void AGoKartPawn::Steering(float InputValue)
{
	SteeringThrow = InputValue;
	ServerSteering(InputValue);
}

bool AGoKartPawn::ServerSteering_Validate(const float InputValue)
{
	return InputValue >= -1.0f && InputValue <= 1.0f;
}

void AGoKartPawn::ServerSteering_Implementation(const float InputValue)
{
	SteeringThrow = InputValue;
}
