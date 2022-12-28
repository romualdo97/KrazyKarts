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
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

private:
	void ClearUnacknowledgedMoves(const FGoKartMove& LastServerMove);

	void UpdateServerState(const FGoKartMove& Move);
	
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSendMove(const FGoKartMove& Move); // Request to be execute on the server (Request started on some client)

	UFUNCTION()
	void OnReplicatedServerState(); // Called on clients
	
	UPROPERTY(ReplicatedUsing=OnReplicatedServerState)
	FGoKartState ServerState;

	UPROPERTY()
	TObjectPtr<UGoKartMovementComponent> MovementComponent; // Cache movement component
	
	TArray<FGoKartMove> UnacknowledgedMoves; // Only for autonomous proxies
};
