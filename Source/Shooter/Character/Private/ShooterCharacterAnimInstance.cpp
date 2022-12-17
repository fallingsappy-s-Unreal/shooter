// Fill out your copyright notice in the Description page of Project Settings.


#include "Shooter/Character/Public//ShooterCharacterAnimInstance.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Shooter/Character/Public/ShooterCharacter.h"

void UShooterCharacterAnimInstance::UpdateAnimationProperties(float DeltaTime)
{
	if (ShooterCharacter == nullptr)
	{
		ShooterCharacter = ShooterCharacter == nullptr ? Cast<AShooterCharacter>(TryGetPawnOwner()) : ShooterCharacter;
	}

	if (ShooterCharacter)
	{
		// Get the lateral speed of the character from velocity
		FVector Velocity { ShooterCharacter->GetVelocity() };
		Velocity.Z = 0;
		Speed = Velocity.Size();

		// Is the character in the air?
		bIsInAir = ShooterCharacter->GetCharacterMovement()->IsFalling();

		// Is the character accelerating?
		bIsAccelerating = ShooterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f;
	}
}

void UShooterCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	ShooterCharacter = ShooterCharacter == nullptr ? Cast<AShooterCharacter>(TryGetPawnOwner()) : ShooterCharacter;
}
