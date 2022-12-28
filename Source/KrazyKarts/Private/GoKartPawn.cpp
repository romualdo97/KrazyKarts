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

	// Create components
	MovementComponent = CreateDefaultSubobject<UGoKartMovementComponent>(TEXT("Go Kart Movement Component"));
	//ReplicationComponent = CreateDefaultSubobject<UGoKartMovementReplicationComponent>(TEXT("Go Kart Movement Replication Component"));
	check(MovementComponent); //check(ReplicationComponent);
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

	// Debug role
	DebugActorRole(this, DeltaTime);
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

// ===================================================
// THROTTLE

void AGoKartPawn::HandleThrottle(const FInputActionValue& ActionValue)
{
	Throttle(ActionValue.Get<float>());
	UE_LOG(LogKrazyKarts, Log, TEXT("Throttle! %f"), ActionValue.Get<float>());
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AGoKartPawn::Throttle(const float InputValue)
{
	// If can bind to input it also means that this actor
	// is an autonomous proxy
	MovementComponent->SetThrottle(InputValue); // Populate client move
}

// ===================================================
// BREAK

void AGoKartPawn::HandleBreak(const FInputActionValue& ActionValue)
{
	Break(ActionValue.Get<float>());
	UE_LOG(LogKrazyKarts, Log, TEXT("Break! %f"), ActionValue.Get<float>());
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AGoKartPawn::Break(const float InputValue)
{
	MovementComponent->SetThrottle(-InputValue); // Populate client move
}

// ===================================================
// STEERING

void AGoKartPawn::HandleSteering(const FInputActionValue& ActionValue)
{
	Steering(ActionValue.Get<float>());
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AGoKartPawn::Steering(const float InputValue)
{
	MovementComponent->SetSteeringThrow(InputValue); // Populate client move
}

