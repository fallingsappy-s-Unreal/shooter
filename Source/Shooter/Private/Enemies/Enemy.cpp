// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemies/Enemy.h"
#include "Math/UnrealMathUtility.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Sound/SoundCue.h"

// Sets default values
AEnemy::AEnemy() :
	Health(100.f),
	MaxHealth(100.f),
	HealthBarDisplayTime(4.f)
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

void AEnemy::PlayHitMontageAccordingToHitDirection(FHitResult HitResult)
{
	FVector ActorForwardVector = GetActorForwardVector();
	FVector ActorRightVector = GetActorRightVector();

	FVector ActorLocation = GetActorLocation();
	FVector Diff = HitResult.Location - ActorLocation;
	Diff.Normalize();
	FVector HitLoc = Diff;

	float ForwardVectorDotProduct = FVector::DotProduct(ActorForwardVector, HitLoc);
	float RightVectorDotProduct = FVector::DotProduct(ActorRightVector, HitLoc);
	
	if (UKismetMathLibrary::InRange_FloatFloat(ForwardVectorDotProduct, 0.5f, 1.0f))
	{
		PlayHitMontage(FName("HitReactFront"));
	}
	else if (UKismetMathLibrary::InRange_FloatFloat(ForwardVectorDotProduct, -1.0f, -0.5f))
	{
		PlayHitMontage(FName("HitReactBack"));
	}
	else if (UKismetMathLibrary::InRange_FloatFloat(RightVectorDotProduct, 0.0f, 1.0f))
	{
		PlayHitMontage(FName("HitReactRight"));
	}
	else if (UKismetMathLibrary::InRange_FloatFloat(RightVectorDotProduct, -1.0f, -0.0f))
	{
		PlayHitMontage(FName("HitReactLeft"));
	}
}

void AEnemy::BulletHit_Implementation(FHitResult HitResult)
{
	IBulletHitInterface::BulletHit_Implementation(HitResult);

	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}

	if (ImpactParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ImpactParticles,
			HitResult.Location,
			FRotator(0.f),
			true
		);
	}
	ShowHealthBar();

	PlayHitMontageAccordingToHitDirection(HitResult);
}

// Called when the game starts or when spawned
void AEnemy::BeginPlay()
{
	Super::BeginPlay();

	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}

void AEnemy::Die()
{
	HideHealthBar();
}

void AEnemy::PlayHitMontage(FName Section, float PlayRate)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{
		AnimInstance->Montage_Play(HitMontage, PlayRate);
		AnimInstance->Montage_JumpToSection(Section, HitMontage);
	}
}

void AEnemy::ShowHealthBar_Implementation()
{
	GetWorldTimerManager().ClearTimer(HealthBarTimer);
	GetWorldTimerManager().SetTimer(HealthBarTimer, this, &AEnemy::HideHealthBar, HealthBarDisplayTime);
}

// Called every frame
void AEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	DrawDebugLine
	(
		GetWorld(),
		GetActorLocation(),
		GetActorForwardVector() * 10000,
		FColor::Red,
		true,
		10.f
	);
}

// Called to bind functionality to input
void AEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

float AEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator,
                         AActor* DamageCauser)
{
	if (Health - DamageAmount <= 0.f)
	{
		Health = 0.f;
		Die();
	}
	else
	{
		Health -= DamageAmount;
	}

	return DamageAmount;
}
