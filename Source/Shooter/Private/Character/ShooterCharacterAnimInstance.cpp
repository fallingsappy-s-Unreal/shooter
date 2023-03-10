// Fill out your copyright notice in the Description page of Project Settings.


#include "Shooter/Public/Character/ShooterCharacterAnimInstance.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Items/Weapons/Weapon.h"
#include "Items/Weapons/WeaponType.h"
#include "Kismet/KismetMathLibrary.h"
#include "Shooter/Public/Character/ShooterCharacter.h"

UShooterCharacterAnimInstance::UShooterCharacterAnimInstance() :
	Speed(0.f), bIsInAir(false), bIsAccelerating(false), MovementOffsetYaw(0.f), LastMovementOffsetYaw(0.f),
	bAiming(false), TIPCharacterYaw(0.f), TIPCharacterYawLastFrame(0.f), Pitch(0.f), bReloading(false),
	OffsetState(EOffsetState::EOS_Hip), CharacterRotation(FRotator(0.f)), CharacterRotationLastFrame(FRotator(0.f)),
	YawDelta(0.f), RecoilWeight(1.f), bTurningInPlace(false), EquippedWeaponType(EWeaponType::EWT_MAX),
	bUseFABRIK(false)
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
		bCrouching = ShooterCharacter->GetCrouching();
		bReloading = ShooterCharacter->GetCombatState() == ECombatState::ECS_Reloading;
		bEquipping = ShooterCharacter->GetCombatState() == ECombatState::ECS_Equipping;
		bUseFABRIK = ShooterCharacter->GetCombatState() == ECombatState::ECS_Unoccupied || ShooterCharacter->
			GetCombatState() == ECombatState::ECS_FireTimerInProgress;

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

		if (bReloading)
		{
			OffsetState = EOffsetState::EOS_Reloading;
		}
		else if (bIsInAir)
		{
			OffsetState = EOffsetState::EOS_InAir;
		}
		else if (ShooterCharacter->GetAiming())
		{
			OffsetState = EOffsetState::EOS_Aiming;
		}
		else
		{
			OffsetState = EOffsetState::EOS_Hip;
		}
		if (ShooterCharacter->GetEquippedWeapon())
		{
			EquippedWeaponType = ShooterCharacter->GetEquippedWeapon()->GetWeaponType();
		}
	}

	TurnInPlace();
	Lean(DeltaTime);
}

void UShooterCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	ShooterCharacter = ShooterCharacter == nullptr ? Cast<AShooterCharacter>(TryGetPawnOwner()) : ShooterCharacter;
}

void UShooterCharacterAnimInstance::SetRecoilWeight()
{
	if (bTurningInPlace)
	{
		if (bReloading || bEquipping)
		{
			RecoilWeight = 1.f;
		}
		else
		{
			RecoilWeight = 0.f;
		}
	}
	else
	{
		if (bCrouching)
		{
			if (bReloading || bEquipping)
			{
				RecoilWeight = 1.f;
			}
			else
			{
				RecoilWeight = 0.1f;
			}
		}
		else
		{
			if (bAiming || bReloading || bEquipping)
			{
				RecoilWeight = 1.f;
			}
			else
			{
				RecoilWeight = 0.5f;
			}
		}
	}
}

void UShooterCharacterAnimInstance::TurnInPlace()
{
	if (!ShooterCharacter) return;

	Pitch = ShooterCharacter->GetBaseAimRotation().Pitch;

	if (Speed > 0 || bIsInAir)
	{
		RootYawOffset = 0.f;
		TIPCharacterYaw = ShooterCharacter->GetActorRotation().Yaw;
		TIPCharacterYawLastFrame = TIPCharacterYaw;
		RotationCurveValue = 0.f;
		RotationCurveValueLastFrame = 0.f;

		return;
	}

	TIPCharacterYawLastFrame = TIPCharacterYaw;
	TIPCharacterYaw = ShooterCharacter->GetActorRotation().Yaw;
	const float TIPYawDelta{TIPCharacterYaw - TIPCharacterYawLastFrame};

	RootYawOffset = UKismetMathLibrary::NormalizeAxis(RootYawOffset - TIPYawDelta);

	const float Turning{GetCurveValue(TEXT("Turning"))};

	if (Turning > 0)
	{
		bTurningInPlace = true;
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
	else
	{
		bTurningInPlace = false;
	}

	SetRecoilWeight();
}

void UShooterCharacterAnimInstance::Lean(float DeltaTime)
{
	if (ShooterCharacter == nullptr) return;

	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = ShooterCharacter->GetActorRotation();

	const FRotator Delta{UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame)};

	const float Target = Delta.Yaw / DeltaTime;

	const float Interp{FMath::FInterpTo(YawDelta, Target, DeltaTime, 6.f)};

	YawDelta = FMath::Clamp(Interp, -90.f, 90.f);
}
