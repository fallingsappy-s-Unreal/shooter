// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemies/Enemy.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "Blueprint/UserWidget.h"
#include "Character/ShooterCharacter.h"
#include "Common/CombatHelper.h"
#include "Common/HitDirection.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Enemies/EnemyController.h"
#include "Engine/SkeletalMeshSocket.h"
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
	AttackR(TEXT("AttackR")),
	BaseDamage(20.f),
	LeftWeaponSocket(TEXT("FX_Trail_L_01")),
	RightWeaponSocket(TEXT("FX_Trail_R_01")),
	bCanAttack(true),
	AttackWaitTime(1.f),
	bDying(false)
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	AgroSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AgroSphere"));
	AgroSphere->SetupAttachment(GetRootComponent());

	CombatRangeSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CombatRange"));
	CombatRangeSphere->SetupAttachment(GetRootComponent());

	LeftWeaponCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("LeftWeaponBox"));
	LeftWeaponCollision->SetupAttachment(GetMesh(), FName("LeftWeaponSocket"));

	RightWeaponCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("RightWeaponBox"));
	RightWeaponCollision->SetupAttachment(GetMesh(), FName("RightWeaponSocket"));
}

void AEnemy::PlayHitMontageAccordingToHitDirection(FHitResult HitResult)
{
	switch (CombatHelper::GetHitDirection(this, HitResult))
	{
	case EHitDirection::EHD_Front:
		PlayHitMontage(FName("HitReactFront"));
		break;
	case EHitDirection::EHD_Back:
		PlayHitMontage(FName("HitReactBack"));
		break;
	case EHitDirection::EHD_Left:
		PlayHitMontage(FName("HitReactLeft"));
		break;
	case EHitDirection::EHD_Right:
		PlayHitMontage(FName("HitReactRight"));
		break;
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

	LeftWeaponCollision->OnComponentBeginOverlap.AddDynamic(this, &AEnemy::OnLeftWeaponOverlap);
	LeftWeaponCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LeftWeaponCollision->SetCollisionObjectType(ECC_WorldDynamic);
	LeftWeaponCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	LeftWeaponCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	RightWeaponCollision->OnComponentBeginOverlap.AddDynamic(this, &AEnemy::OnRightWeaponOverlap);
	RightWeaponCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RightWeaponCollision->SetCollisionObjectType(ECC_WorldDynamic);
	RightWeaponCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	RightWeaponCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);


	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	EnemyController = Cast<AEnemyController>(GetController());

	if (EnemyController)
	{
		const FVector WorldPatrolPoint = UKismetMathLibrary::TransformLocation(GetActorTransform(), PatrolPoint);
		EnemyController->GetEnemyBlackboardComponent()->SetValueAsVector(FName("PatrolPoint"), WorldPatrolPoint);

		const FVector WorldPatrolPoint2 = UKismetMathLibrary::TransformLocation(GetActorTransform(), PatrolPoint2);
		EnemyController->GetEnemyBlackboardComponent()->SetValueAsVector(FName("PatrolPoint2"), WorldPatrolPoint2);
		
		EnemyController->GetEnemyBlackboardComponent()->SetValueAsBool(FName("CanAttack"), bCanAttack);
		
		EnemyController->RunBehaviorTree(BehaviorTree);
	}
}

void AEnemy::Die(const FHitResult& HitResult)
{
	if (bDying) return;
	bDying = true;
	
	HideHealthBar();
	PlayDeathMontage(HitResult);
	
	if (EnemyController)
	{
		EnemyController->GetEnemyBlackboardComponent()->SetValueAsBool(FName("IsDead"), true);
		EnemyController->StopMovement();
	}
}

void AEnemy::Die()
{
	if (bDying) return;
	bDying = true;
	
	HideHealthBar();
	PlayMontageSection(FName("DeathFromFront"), 1.0f, DeathMontage);
	if (EnemyController)
	{
		EnemyController->GetEnemyBlackboardComponent()->SetValueAsBool(FName("IsDead"), true);
		EnemyController->StopMovement();
	}
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
	bCanAttack = false;
	GetWorldTimerManager().SetTimer(
		AttackWaitTimer,
		this,
		&AEnemy::ResetCanAttack,
		AttackWaitTime
	);
	if (EnemyController)
	{
		EnemyController->GetEnemyBlackboardComponent()->SetValueAsBool(FName("CanAttack"), bCanAttack);
	}
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

	const auto Character = Cast<AShooterCharacter>(OtherActor);

	if (Character && EnemyController)
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
	default: return FName("");
	}
}

void AEnemy::SpawnBlood(FName SocketName, AShooterCharacter* const Character)
{
	const USkeletalMeshSocket* TipSocket{GetMesh()->GetSocketByName(SocketName)};
	if (TipSocket)
	{
		const FTransform SocketTransform{TipSocket->GetSocketTransform(GetMesh())};
		if (Character->GetBloodParticles())
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), Character->GetBloodParticles(), SocketTransform);
		}
	}
}

void AEnemy::DoDamage(AActor* Victim, FName SocketName, const FHitResult& HitResult)
{
	if (MeleeImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, MeleeImpactSound, GetActorLocation());
	}

	if (Victim == nullptr) return;

	const auto Character = Cast<AShooterCharacter>(Victim);
	if (Character)
	{
		SpawnBlood(SocketName, Character);
		AttemptToStunCharacter(Character, HitResult);
		UGameplayStatics::ApplyDamage(Character, BaseDamage, EnemyController, this, UDamageType::StaticClass());
	}
}

void AEnemy::OnLeftWeaponOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                 const FHitResult& SweepResult)
{
	DoDamage(OtherActor, LeftWeaponSocket, SweepResult);
}

void AEnemy::OnRightWeaponOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                  const FHitResult& SweepResult)
{
	DoDamage(OtherActor, RightWeaponSocket, SweepResult);
}

void AEnemy::ActivateLeftWeapon()
{
	LeftWeaponCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void AEnemy::DeactivateLeftWeapon()
{
	LeftWeaponCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AEnemy::ActivateRightWeapon()
{
	RightWeaponCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void AEnemy::DeactivateRightWeapon()
{
	RightWeaponCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AEnemy::AttemptToStunCharacter(AShooterCharacter* Victim, const FHitResult& HitResult)
{
	if (Victim)
	{
		const float Stun{FMath::RandRange(0.f, 1.f)};
		if (Stun <= Victim->GetStunChance())
		{
			Victim->Stun(HitResult);
		}
	}
}

void AEnemy::ResetCanAttack()
{
	bCanAttack = true;

	if (EnemyController)
	{
		EnemyController->GetEnemyBlackboardComponent()->SetValueAsBool(FName("CanAttack"), bCanAttack);
	}
}

void AEnemy::FinishDeath()
{
	Destroy();
}

void AEnemy::ShowHealthBar_Implementation()
{
	GetWorldTimerManager().ClearTimer(HealthBarTimer);
	GetWorldTimerManager().SetTimer(HealthBarTimer, this, &AEnemy::HideHealthBar, HealthBarDisplayTime);
}

void AEnemy::PlayDeathMontage(const FHitResult& HitResult)
{
	switch (CombatHelper::GetHitDirection(this, HitResult))
	{
	case EHitDirection::EHD_Left:
	case EHitDirection::EHD_Front:
		PlayMontageSection(FName("DeathFromFront"), 1.0f, DeathMontage);
		break;
	case EHitDirection::EHD_Right:
	case EHitDirection::EHD_Back:
		PlayMontageSection(FName("DeathFromBack"), 1.0f, DeathMontage);
		break;
	}
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
	if (EnemyController)
	{
		EnemyController->GetEnemyBlackboardComponent()->SetValueAsObject(FName("Target"), DamageCauser);
	}

	if (Health - DamageAmount <= 0.f)
	{
		Health = 0.f;
	}
	else
	{
		Health -= DamageAmount;
	}
	
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		FPointDamageEvent* const PointDamageEvent = (FPointDamageEvent*) &DamageEvent;
	
		if (Health - DamageAmount <= 0.f)
		{
			Die(PointDamageEvent->HitInfo);
		}
	}
	else
	{
		if (Health - DamageAmount <= 0.f)
		{
			Die();
		}
	}

	return DamageAmount;
}
