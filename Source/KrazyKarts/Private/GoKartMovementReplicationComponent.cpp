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
		// Use the replicated server state to simulate some other client locally on this client
		MovementComponent->SimulateMoveTick(ServerState.LastMove);
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

// ===================================================
// IMPLEMENT SERVER RPCs (to be executed on the server)

bool UGoKartMovementReplicationComponent::ServerSendMove_Validate(const FGoKartMove& Move)
{
	if (Move.Throttle < -1.0f || Move.Throttle > 1.0f)
	{
		UE_LOG(LogKrazyKarts, Error, TEXT("Invalid Move.CurrentThrottle == %f"), Move.Throttle)
		return false;
	}
	
	if (Move.SteeringThrow < -1.0f || Move.SteeringThrow > 1.0f)
	{
		UE_LOG(LogKrazyKarts, Error, TEXT("Invalid Move.SteeringThrow == %f"), Move.SteeringThrow)
		return false;
	}
	
	return true;
}

void UGoKartMovementReplicationComponent::ServerSendMove_Implementation(const FGoKartMove& Move)
{
	if (MovementComponent == nullptr)
	{
		UE_LOG(LogKrazyKarts, Warning, TEXT("[%s] No movement component at line %i"), ANSI_TO_TCHAR(__FUNCTION__), __LINE__);
		return;
	};
	
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


