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

	DOREPLIFETIME(AGoKartPawn, ServerState);
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

	if (IsLocallyControlled())
	{
		// From client execute on the server OR from server execute on the server
		ServerSendMove(ClientMove);
	}

	if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		// Simulate on autonomous clients
		ClientMove = SimulateLocalUpdate(ClientMove);
	}
	else if (GetLocalRole() == ROLE_SimulatedProxy)
	{
		// Use the replicated server state to simulate some other client locally on this client
		SimulateLocalUpdate(ServerState.LastMove);
	}
	else if (GetLocalRole() == ROLE_Authority)
	{		
		// Time is set authoritatively by the server
		ServerMove.DeltaTime = DeltaTime;
		ServerMove.Time += DeltaTime;
		
		// Simulate on server and update the server state (to replicate on all the clients)
		ServerMove = SimulateLocalUpdate(ServerMove);
		
		// Update the server state
		ServerState.Transform = GetActorTransform();
		ServerState.Velocity = Velocity;
		ServerState.LastMove = ServerMove;
	}
}

FGoKartMove AGoKartPawn::SimulateLocalUpdate(const FGoKartMove& Move)
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
	return UpdateLocation(Move);
}

// Called to bind functionality to input
void AGoKartPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Called only on the Autonomous client or the actor controlled by this authoritative player (serving player)
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

FGoKartMove AGoKartPawn::UpdateLocation(const FGoKartMove& Move)
{
	FGoKartMove NewMove{Move};
	const FVector DeltaLocation = NewMove.DeltaTime * Velocity * 100; // convert meters to centimeters

	FHitResult HitResult;
	AddActorWorldOffset(DeltaLocation, true, &HitResult);
	
	// Bounce the car
	if (HitResult.IsValidBlockingHit())
	{
		Velocity *= -BounceFactor;
		NewMove.SteeringThrow = 0;
	}
	
	// Reset before the next tick
	AccumulatedForce = FVector::Zero();

	return NewMove; // The move object modified because of collisions
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
 
void AGoKartPawn::AddMovingForce(const FGoKartMove& Move)
{
	// Dot product to manage reverse
	// https://en.wikipedia.org/wiki/Turning_radius
	const float DeltaDistance = FVector::DotProduct(GetActorForwardVector(), Velocity) * Move.DeltaTime;
	const float DeltaAngle = DeltaDistance / MinTurningRadius * Move.SteeringThrow;
	const FQuat DeltaRotation{GetActorUpVector(), DeltaAngle};
	
	Velocity = DeltaRotation * Velocity;
	AddActorWorldRotation(DeltaRotation);
	
	MovingForce = ThrottleForce * Move.CurrentThrottle * GetActorForwardVector();
	AccumulatedForce += MovingForce;
}

// ===================================================
// THROTTLE

void AGoKartPawn::HandleThrottle(const FInputActionValue& ActionValue)
{
	Throttle(ActionValue.Get<float>());
	//UE_LOG(LogKrazyKarts, Log, TEXT("Throttle! %f"), ActionValue.Get<float>());
}

void AGoKartPawn::Throttle(const float InputValue)
{
	// If can bind to input it also means that this actor
	// is an autonomous proxy
	ClientMove.CurrentThrottle = InputValue; // Populate client move
}

// ===================================================
// BREAK

void AGoKartPawn::HandleBreak(const FInputActionValue& ActionValue)
{
	Break(ActionValue.Get<float>());
	//UE_LOG(LogKrazyKarts, Log, TEXT("Break! %f"), ActionValue.Get<float>());
}

void AGoKartPawn::Break(const float InputValue)
{
	ClientMove.CurrentThrottle = -InputValue; // Populate client move
}

// ===================================================
// STEERING

void AGoKartPawn::HandleSteering(const FInputActionValue& ActionValue)
{
	Steering(ActionValue.Get<float>());
}

void AGoKartPawn::Steering(const float InputValue)
{
	ClientMove.SteeringThrow = InputValue; // Populate client move
}

// ===================================================
// IMPLEMENT SERVER RPCs (to be executed on the server)

bool AGoKartPawn::ServerSendMove_Validate(const FGoKartMove& Move)
{
	if (Move.CurrentThrottle < -1.0f || Move.CurrentThrottle > 1.0f)
	{
		UE_LOG(LogKrazyKarts, Error, TEXT("Invalid Move.CurrentThrottle == %f"), Move.CurrentThrottle)
		return false;
	}
	
	if (Move.SteeringThrow < -1.0f || Move.SteeringThrow > 1.0f)
	{
		UE_LOG(LogKrazyKarts, Error, TEXT("Invalid Move.SteeringThrow == %f"), Move.SteeringThrow)
		return false;
	}
	
	return true;
}

void AGoKartPawn::ServerSendMove_Implementation(const FGoKartMove& Move)
{
	ServerMove = Move;
}

// ===================================================
// HANDLE REPLICATION RECEIVED ON CLIENTS

void AGoKartPawn::OnReplicatedServerState()
{
	// Called only on clients
	SetActorTransform(ServerState.Transform);
	Velocity = ServerState.Velocity;
	ClientMove = ServerState.LastMove;
}
