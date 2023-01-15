// Fill out your copyright notice in the Description page of Project Settings.


#include "Shooter/Public/Character/ShooterCharacterAnimInstance.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Shooter//Public/Character/ShooterCharacter.h"

UShooterCharacterAnimInstance::UShooterCharacterAnimInstance() :
	Speed(0.f), bIsInAir(false), bIsAccelerating(false), MovementOffsetYaw(0.f), LastMovementOffsetYaw(0.f),
	bAiming(false), CharacterYaw(0.f), CharacterYawLastFrame(0.f), Pitch(0.f), bReloading(false)
{
}

void UShooterCharacterAnimInstance::UpdateAnimationProperties(float DeltaTime)
{
	if (ShooterCharacter == nullptr)
	{
		ShooterCharacter = ShooterCharacter == nullptr ? Cast<AShooterCharacter>(TryGetPawnOwner()) : ShooterCharacter;
	}

	if (ShooterCharacter)
	{
		bReloading = ShooterCharacter->GetCombatState() == ECombatState::ECS_Reloading;
		
		// Get the lateral speed of the character from velocity
		FVector Velocity{ShooterCharacter->GetVelocity()};
		Velocity.Z = 0;
		Speed = Velocity.Size();

		// Is the character in the air?
		bIsInAir = ShooterCharacter->GetCharacterMovement()->IsFalling();

		// Is the character accelerating?
		bIsAccelerating = ShooterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f;

		FRotator AimRotation = ShooterCharacter->GetBaseAimRotation();

		FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(ShooterCharacter->GetVelocity());

		MovementOffsetYaw = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation).Yaw;

		if (ShooterCharacter->GetVelocity().Size() > 0.f)
		{
			LastMovementOffsetYaw = MovementOffsetYaw;
		}

		bAiming = ShooterCharacter->GetAiming();
	}

	TurnInPlace();
}

void UShooterCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	ShooterCharacter = ShooterCharacter == nullptr ? Cast<AShooterCharacter>(TryGetPawnOwner()) : ShooterCharacter;
}

void UShooterCharacterAnimInstance::TurnInPlace()
{
	if (!ShooterCharacter) return;

	Pitch = ShooterCharacter->GetBaseAimRotation().Pitch;
	
	if (Speed > 0)
	{
		RootYawOffset = 0.f;
		CharacterYaw = ShooterCharacter->GetActorRotation().Yaw;
		CharacterYawLastFrame = CharacterYaw;
		RotationCurveValue = 0.f;
		RotationCurveValueLastFrame = 0.f;
		
		return;
	}

	CharacterYawLastFrame = CharacterYaw;
	CharacterYaw = ShooterCharacter->GetActorRotation().Yaw;
	const float YawDelta{CharacterYaw - CharacterYawLastFrame};
  
	RootYawOffset = UKismetMathLibrary::NormalizeAxis(RootYawOffset - YawDelta);

	const float Turning{GetCurveValue(TEXT("Turning"))};
	if (Turning > 0)
	{
		RotationCurveValueLastFrame = RotationCurveValue;
		RotationCurveValue = GetCurveValue(TEXT("Rotation"));
		const float DeltaRotation{RotationCurveValue - RotationCurveValueLastFrame};

		// RootYawOffset > 0 -> Turning Left, RootYawOffset < 0 -> Turning Right
		RootYawOffset > 0 ? RootYawOffset -= DeltaRotation : RootYawOffset += DeltaRotation;

		const float AbsRootYawOffset{FMath::Abs(RootYawOffset)};
		if (AbsRootYawOffset > 90.f)
		{
			const float YawExcess{AbsRootYawOffset - 90.f};
			RootYawOffset > 0 ? RootYawOffset -= YawExcess : RootYawOffset += YawExcess;
		}
	}
}
