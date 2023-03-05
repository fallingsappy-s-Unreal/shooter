// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemies/Enemy.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "Blueprint/UserWidget.h"
#include "Character/ShooterCharacter.h"
#include "Components/SphereComponent.h"
#include "Enemies/EnemyController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Sound/SoundCue.h"

// Sets default values
AEnemy::AEnemy() :
	Health(100.f),
	MaxHealth(100.f),
	HealthBarDisplayTime(4.f),
	bCanHitReact(true),
	HitReactTimeMin(.5f),
	HitReactTimeMax(3.f),
	HitNumberDestroyTime(1.5f),
	bStunned(false),
	StunChance(0.5f),
	AttackLFast(TEXT("AttackLFast")),
	AttackRFast(TEXT("AttackRFast")),
	AttackL(TEXT("AttackL")),
	AttackR(TEXT("AttackR"))
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	AgroSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AgroSphere"));
	AgroSphere->SetupAttachment(GetRootComponent());

	CombatRangeSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CombatRange"));
	CombatRangeSphere->SetupAttachment(GetRootComponent());
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

	const float Stunned = FMath::FRandRange(0.f, 1.f);
	if (Stunned <= StunChance)
	{
		PlayHitMontageAccordingToHitDirection(HitResult);
		SetStunned(true);
	}
}

// Called when the game starts or when spawned
void AEnemy::BeginPlay()
{
	Super::BeginPlay();

	AgroSphere->OnComponentBeginOverlap.AddDynamic(this, &AEnemy::AgroSphereOverlap);
	CombatRangeSphere->OnComponentBeginOverlap.AddDynamic(this, &AEnemy::CombatRangeOverlap);
	CombatRangeSphere->OnComponentEndOverlap.AddDynamic(this, &AEnemy::CombatRangeEndOverlap);

	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	EnemyController = Cast<AEnemyController>(GetController());

	if (EnemyController)
	{
		const FVector WorldPatrolPoint = UKismetMathLibrary::TransformLocation(GetActorTransform(), PatrolPoint);
		EnemyController->GetEnemyBlackboardComponent()->SetValueAsVector(FName("PatrolPoint"), WorldPatrolPoint);

		const FVector WorldPatrolPoint2 = UKismetMathLibrary::TransformLocation(GetActorTransform(), PatrolPoint2);
		EnemyController->GetEnemyBlackboardComponent()->SetValueAsVector(FName("PatrolPoint2"), WorldPatrolPoint2);

		EnemyController->RunBehaviorTree(BehaviorTree);
	}
}

void AEnemy::Die()
{
	HideHealthBar();
}

void AEnemy::PlayMontageSection(FName Section, float PlayRate, UAnimMontage* MontageToPlay)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && MontageToPlay)
	{
		AnimInstance->Montage_Play(MontageToPlay, PlayRate);
		AnimInstance->Montage_JumpToSection(Section, MontageToPlay);
	}
}

void AEnemy::PlayHitMontage(FName Section, float PlayRate)
{
	if (!bCanHitReact) return;

	PlayMontageSection(Section, PlayRate, HitMontage);

	bCanHitReact = false;
	const float HitReactTime{FMath::FRandRange(HitReactTimeMin, HitReactTimeMax)};
	GetWorldTimerManager().SetTimer(HitReactTimer, this, &AEnemy::ResetHitReactTimer, HitReactTime);
}

void AEnemy::PlayAttackMontage(FName Section, float PlayRate)
{
	PlayMontageSection(Section, PlayRate, AttackMontage);
}

void AEnemy::ResetHitReactTimer()
{
	bCanHitReact = true;
}

void AEnemy::StoreHitNumber(UUserWidget* HitNumber, FVector Location)
{
	HitNumbers.Add(HitNumber, Location);

	FTimerHandle HitNumberTimer;
	FTimerDelegate HitNumberDelegate;
	HitNumberDelegate.BindUFunction(this, FName("DestroyHitNumber"), HitNumber);
	GetWorld()->GetTimerManager().SetTimer(
		HitNumberTimer,
		HitNumberDelegate,
		HitNumberDestroyTime,
		false
	);
}

void AEnemy::DestroyHitNumber(UUserWidget* HitNumber)
{
	HitNumbers.Remove(HitNumber);
	HitNumber->RemoveFromParent();
}

void AEnemy::UpdateHitNumbers()
{
	for (const auto& HitPair : HitNumbers)
	{
		UUserWidget* HitNumber{HitPair.Key};
		const FVector Location{HitPair.Value};

		FVector2D ScreenPosition;
		UGameplayStatics::ProjectWorldToScreen(GetWorld()->GetFirstPlayerController(), Location, ScreenPosition);
		HitNumber->SetPositionInViewport(ScreenPosition);
	}
}

void AEnemy::AgroSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                               UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                               const FHitResult& SweepResult)
{
	if (OtherActor == nullptr) return;

	if (const auto Character = Cast<AShooterCharacter>(OtherActor))
	{
		EnemyController->GetEnemyBlackboardComponent()->SetValueAsObject(FName("Target"), Character);
	}
}

void AEnemy::SetStunned(bool Stunned)
{
	bStunned = Stunned;
	if (EnemyController)
	{
		EnemyController->GetEnemyBlackboardComponent()->SetValueAsBool(FName("Stunned"), Stunned);
	}
}

void AEnemy::CombatRangeOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                const FHitResult& SweepResult)
{
	SetIsInAttackRange(OtherActor, true);
}

void AEnemy::CombatRangeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	SetIsInAttackRange(OtherActor, false);
}

bool AEnemy::SetIsInAttackRange(AActor* OtherActor, bool IsInAttackRange)
{
	if (OtherActor == nullptr) return true;

	const auto ShooterCharacter = Cast<AShooterCharacter>(OtherActor);
	if (ShooterCharacter == nullptr) return true;

	bInAttackRange = IsInAttackRange;
	if (EnemyController)
	{
		EnemyController->GetEnemyBlackboardComponent()->SetValueAsBool(FName("InAttackRange"), IsInAttackRange);
	}
	return false;
}

FName AEnemy::GetAttackSectionName()
{
	const int32 Section{FMath::RandRange(1, 4)};
	switch (Section)
	{
	case 1: return AttackLFast;
	case 2: return AttackRFast;
	case 3: return AttackL;
	case 4: return AttackR;
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

	UpdateHitNumbers();
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
