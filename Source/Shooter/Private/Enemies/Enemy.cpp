// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemies/Enemy.h"
#include "Math/UnrealMathUtility.h"
#include "Kismet/GameplayStatics.h"
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
	FVector HitActorRightVector = FVector(0.f, HitResult.GetActor()->GetActorRightVector().Y, 0.f);
	FVector HitVector = FVector(0.f, HitResult.Normal.Y, 0.f);

		
	// Angle between two vectors:
	// Angle = ArcCosine( ( vectorA dot VectorB ) / ( VectorLengthA x VectorLengthB ) )

	// Dot Product
	float dotProduct = FVector::DotProduct(HitActorRightVector, HitVector);
	//float lengthProduct = HitActorForwardVector.Size() * HitResult.Normal.Size();

	// Angle
	//float angle = FMath::Acos(dotProduct / lengthProduct);

	float angle = FMath::Acos(dotProduct) * 180 / PI;
	UE_LOG(LogTemp, Warning, TEXT("Text, %f"), angle);

	if (angle > 0 && angle < 90)
	{
		PlayHitMontage(FName("HitReactRight"));
	}
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

	FVector HitActorRightVector = FVector(0.f, GetActorRightVector().Y, 0.f);
	FVector HitActorRightVector1 = FVector(0.f, GetActorRightVector().Y + 1000, 0.f);
	
	DrawDebugLine
	(
		GetWorld(),
		HitActorRightVector,
		HitActorRightVector1,
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
