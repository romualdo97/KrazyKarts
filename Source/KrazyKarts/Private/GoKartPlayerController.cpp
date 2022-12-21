// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKartPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"

void AGoKartPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	if (const ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player))
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (!InputMapping.IsNull())
			{
				InputSystem->AddMappingContext(InputMapping.LoadSynchronous(), 0);
			}
		}
	}
}
