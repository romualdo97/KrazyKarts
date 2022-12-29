// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GoKartMovementComponent.h"
#include "GoKartMovementReplicationComponent.generated.h"

USTRUCT()
struct FGoKartState
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Velocity{0};

	UPROPERTY()
	FTransform Transform{};

	UPROPERTY()
	FGoKartMove LastMove{};
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class KRAZYKARTS_API UGoKartMovementReplicationComponent final : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UGoKartMovementReplicationComponent();

	// Not required to declare... was automatically declared when using replication
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

private:
	// Remove movements that were already handled and confirmed by the server @ AutonomousProxy
	void ClearUnacknowledgedMoves(const FGoKartMove& LastServerMove);

	// Helper called on the server to update the server state data which is replicated
	// to the clients @ Authoritative
	void UpdateServerState(const FGoKartMove& Move);

	// Called every frame only on SimulatedProxy clients
	void SimulatedProxyTick(float DeltaTime);

	// Request to update the server state, this request will be executed on the server
	// (Request started on some client or in a locally controlled authoritative player)
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSendMove(const FGoKartMove& Move);

	// Called on clients when new ServerState data is available
	UFUNCTION()
	void OnReplicatedServerState();
	void OnReplicatedServerStateForAutonomousProxy();
	void OnReplicatedServerStateForSimulatedProxy();

	// On server, replicated on clients
	UPROPERTY(ReplicatedUsing=OnReplicatedServerState)
	FGoKartState ServerState; 

	// If true location and velocity on simulated proxies will be interpolated using cubic interpolation, otherwise we interpolate using linear
	UPROPERTY(EditDefaultsOnly)
	bool bSimulatedProxyUsesCubicInterpolation{true}; 
	
	UPROPERTY()
	TObjectPtr<UGoKartMovementComponent> MovementComponent; // Cache the movement component
	TArray<FGoKartMove> UnacknowledgedMoves; // Only for autonomous proxies
	FTransform StartTransformForSimulatedProxy; // Only for simulated proxies
	FVector StartVelocityForSimulatedProxy; // Only for simulated proxies
	float ClientTimeSinceLastReplication{0.0f}; // Only for simulated proxies
	float ClientTimeBetweenLastReplication{0.0f}; // Only for simulated proxies
};
