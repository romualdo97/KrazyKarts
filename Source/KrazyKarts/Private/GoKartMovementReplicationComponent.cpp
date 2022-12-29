// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKartMovementReplicationComponent.h"

#include "GoKartPawn.h"
#include "KrazyKarts/KrazyKarts.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UGoKartMovementReplicationComponent::UGoKartMovementReplicationComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UGoKartMovementReplicationComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UGoKartMovementReplicationComponent, ServerState);
}

// Called when the game starts
void UGoKartMovementReplicationComponent::BeginPlay()
{
	Super::BeginPlay();

	MovementComponent = GetOwner()->FindComponentByClass<UGoKartMovementComponent>();
	check(MovementComponent);
}

// Called every frame
void UGoKartMovementReplicationComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                        FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const APawn* Pawn = GetOwner<APawn>();
	if (Pawn == nullptr) return;

	if (MovementComponent == nullptr)
	{
		UE_LOG(LogKrazyKarts, Warning, TEXT("[%s] No movement component at line %i"), ANSI_TO_TCHAR(__FUNCTION__), __LINE__);
		return;
	};

	if (MeshOffsetRoot == nullptr)
	{
		UE_LOG(LogKrazyKarts, Error, TEXT("[%s] No mesh offset component at line %i"), ANSI_TO_TCHAR(__FUNCTION__), __LINE__);
		return;
	};

	if (Pawn->IsLocallyControlled())
	{
		const FGoKartMove LastMove = MovementComponent->GetLastMove();
		
		if (Pawn->GetLocalRole() == ROLE_AutonomousProxy)
		{
			// Add client move's to the buffer of unacknowledged player moves
			UnacknowledgedMoves.Add(LastMove);
			// UE_LOG(LogKrazyKarts, Log, TEXT("UnacknowledgedMoves.Num() == %i"), UnacknowledgedMoves.Num());

			// Called from client and executed on the server (client request goes over network, has latency)
			ServerSendMove(LastMove);
		}
		else
		{
			// Just update state if is an Authoritative locally controlled player on the server so we avoid simulating twice
			UpdateServerState(LastMove);
		}

		// Called from client executed on the server (client request goes over network, has latency)
		// OR Called from server and executed on the server (no request goes over network, zero latency and it calls simulate)
		// ServerSendMove(LastMove);
	}
	else if (Pawn->GetLocalRole() == ROLE_SimulatedProxy)
	{
		// Interpolate based on last two received server states
		SimulatedProxyTick(DeltaTime);
		
		// Use the replicated server state to simulate some other client locally on this client
		// MovementComponent->SimulateMoveTick(ServerState.LastMove);
	}
}

void UGoKartMovementReplicationComponent::ClearUnacknowledgedMoves(const FGoKartMove& LastServerMove)
{
	TArray<FGoKartMove> NewMoves;
	for (const FGoKartMove& Move : UnacknowledgedMoves)
	{
		if (Move.Time > LastServerMove.Time)
		{
			NewMoves.Add(Move);
		}
	}

	UnacknowledgedMoves = NewMoves;
}

void UGoKartMovementReplicationComponent::UpdateServerState(const FGoKartMove& Move)
{
	// Update the server state
	ServerState.LastMove = Move;
	ServerState.Transform = GetOwner()->GetActorTransform();
	ServerState.Velocity = MovementComponent->GetVelocity();
}

void UGoKartMovementReplicationComponent::SimulatedProxyTick(const float DeltaTime)
{
	ClientTimeSinceLastReplication += DeltaTime;
	
	// If first frame then skip
	if (ClientTimeBetweenLastReplication < KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float LerpRatio = ClientTimeSinceLastReplication / ClientTimeBetweenLastReplication;
	
	const FVector StartLocation = StartTransformForSimulatedProxy.GetLocation();
	const FVector TargetLocation = ServerState.Transform.GetLocation();
	FVector NewLocation;
	FVector NewVelocity;

	if (bSimulatedProxyUsesCubicInterpolation)
	{
		// NOTE: This is required because we need the derivative in terms of Alpha
		// (1) Slope = Derivative = DeltaLocation / DeltaAlpha
		// (2) Velocity = DeltaLocation / DeltaTime
		// (3) DeltaAlpha = DeltaTime / ClientTimeBetweenLastReplication
		// Put (3) in (1)
		// (4) Derivative = DeltaLocation / (DeltaTime / ClientTimeBetweenLastReplication)
		// (5) Derivative = Velocity * ClientTimeBetweenLastReplication

		// Calculate a point over the cubic function
		const float VelocityToDerivative = ClientTimeBetweenLastReplication * 100; // Multiply by 100 to convert velocity at next line from m/s to cm/s
		const FVector StartDerivative = StartVelocityForSimulatedProxy * VelocityToDerivative;
		const FVector TargetDerivative = ServerState.Velocity * VelocityToDerivative; 
		NewLocation = FMath::CubicInterp(StartLocation, StartDerivative, TargetLocation, TargetDerivative, LerpRatio);

		// Calculate the first derivative of point over the cubic curve
		NewVelocity = FMath::CubicInterpDerivative(StartLocation, StartDerivative, TargetLocation, TargetDerivative, LerpRatio);
		NewVelocity /= VelocityToDerivative; // Resolve velocity at (5)
	}
	else
	{
		NewLocation = FMath::LerpStable(StartLocation, TargetLocation, LerpRatio);
		NewVelocity = MovementComponent	->GetVelocity(); // Just does nothing
	}

	const FQuat StartRotation = StartTransformForSimulatedProxy.GetRotation();
	const FQuat TargetRotation = ServerState.Transform.GetRotation();
	const FQuat NewRotation = FQuat::Slerp(StartRotation, TargetRotation, LerpRatio);

	// Apply side effects
	MeshOffsetRoot->SetWorldLocation(NewLocation);
	MeshOffsetRoot->SetWorldRotation(FRotator{NewRotation});
	MovementComponent->SetVelocity(NewVelocity);
}

// ===================================================
// IMPLEMENT SERVER RPCs (to be executed on the server)

bool UGoKartMovementReplicationComponent::ServerSendMove_Validate(const FGoKartMove& Move)
{
	if (const float ProposedTime = ClientSimulatedTime + Move.DeltaTime;
		ProposedTime > GetWorld()->GetTimeSeconds())
	{
		UE_LOG(LogKrazyKarts, Error, TEXT("Invalid simulated time on client == %f"), ProposedTime)
		return false;
	}
	
	return Move.IsValid();
}

void UGoKartMovementReplicationComponent::ServerSendMove_Implementation(const FGoKartMove& Move)
{
	if (MovementComponent == nullptr)
	{
		UE_LOG(LogKrazyKarts, Warning, TEXT("[%s] No movement component at line %i"), ANSI_TO_TCHAR(__FUNCTION__), __LINE__);
		return;
	};

	ClientSimulatedTime += Move.DeltaTime;
	
	// NOTE: Doing here and not on Tick because otherwise the server would be simulating
	// with the last received input and would cause the client to move back to the last "speculative"
	// server state
	// https://bugs.mojang.com/browse/MCPE-102760
	// https://forum.unity.com/threads/glitchy-client-side-prediction.1192453/	
	
	MovementComponent->SimulateMoveTick(Move);
	UpdateServerState(Move);
}

// ===================================================
// HANDLE REPLICATION RECEIVED ON CLIENTS

void UGoKartMovementReplicationComponent::OnReplicatedServerState()
{
	if (MovementComponent == nullptr)
	{
		UE_LOG(LogKrazyKarts, Warning, TEXT("[%s] No movement component at line %i"), ANSI_TO_TCHAR(__FUNCTION__), __LINE__);
		return;
	};

	// Handle replication on distinct clients
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		OnReplicatedServerStateForAutonomousProxy();
	}
	else if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		OnReplicatedServerStateForSimulatedProxy();		
	}
}

void UGoKartMovementReplicationComponent::OnReplicatedServerStateForAutonomousProxy()
{
	// Called only on clients
	GetOwner()->SetActorTransform(ServerState.Transform);
	MovementComponent->SetVelocity(ServerState.Velocity);

	// Clear moves generated previously than last server replicated move
	ClearUnacknowledgedMoves(ServerState.LastMove);

	// Simulate client moves that are ahead of the last server response
	for (const FGoKartMove& Move : UnacknowledgedMoves)
	{
		MovementComponent->SimulateMoveTick(Move);
	}
}

void UGoKartMovementReplicationComponent::OnReplicatedServerStateForSimulatedProxy()
{
	if (MeshOffsetRoot == nullptr)
	{
		UE_LOG(LogKrazyKarts, Error, TEXT("[%s] No mesh offset component at line %i"), ANSI_TO_TCHAR(__FUNCTION__), __LINE__);
		return;
	};
	
	ClientTimeBetweenLastReplication = ClientTimeSinceLastReplication;
	ClientTimeSinceLastReplication = 0;
	StartTransformForSimulatedProxy.SetLocation(MeshOffsetRoot->GetComponentLocation());
	StartTransformForSimulatedProxy.SetRotation(MeshOffsetRoot->GetComponentQuat());
	StartVelocityForSimulatedProxy = MovementComponent->GetVelocity();

	GetOwner()->SetActorTransform(ServerState.Transform);
}


