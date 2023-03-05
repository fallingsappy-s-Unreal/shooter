// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BulletHitInterface.h"
#include "Character/ShooterCharacter.h"
#include "GameFramework/Character.h"
#include "Enemy.generated.h"

class UParticleSystem;
class USoundCue;
class UBehaviorTree;
class AEnemyController;
class USphereComponent;
class UBoxComponent;

UCLASS()
class SHOOTER_API AEnemy : public ACharacter, public IBulletHitInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AEnemy();

	virtual void BulletHit_Implementation(FHitResult HitResult) override;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintNativeEvent)
	void ShowHealthBar();

	UFUNCTION(BlueprintImplementableEvent)
	void HideHealthBar();

	void Die();
	void PlayMontageSection(FName Section, float PlayRate, UAnimMontage* MontageToPlay);

	void PlayHitMontage(FName Section, float PlayRate = 1.0f);

	UFUNCTION(BlueprintCallable)
	void PlayAttackMontage(FName Section, float PlayRate = 1.0f);

	void ResetHitReactTimer();

	UFUNCTION(BlueprintCallable)
	void StoreHitNumber(UUserWidget* HitNumber, FVector Location);

	UFUNCTION()
	void DestroyHitNumber(UUserWidget* HitNumber);

	void UpdateHitNumbers();

	UFUNCTION()
	void AgroSphereOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION(BlueprintCallable)
	void SetStunned(bool Stunned);

	UFUNCTION()
	void CombatRangeOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	void CombatRangeEndOverlap(UPrimitiveComponent* OverlappedComponent,
	                           AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	bool SetIsInAttackRange(AActor* OtherActor, bool IsInAttackRange);

	UFUNCTION(BlueprintPure)
	FName GetAttackSectionName();
	void SpawnBlood(FName SocketName, AShooterCharacter* Character);

	void DoDamage(AActor* Victim, FName SocketName);

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UBoxComponent* LeftWeaponCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UBoxComponent* RightWeaponCollision;

	UFUNCTION()
	void OnLeftWeaponOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	void OnRightWeaponOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION(BlueprintCallable)
	void ActivateLeftWeapon();

	UFUNCTION(BlueprintCallable)
	void DeactivateLeftWeapon();

	UFUNCTION(BlueprintCallable)
	void ActivateRightWeapon();

	UFUNCTION(BlueprintCallable)
	void DeactivateRightWeapon();
private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UParticleSystem* ImpactParticles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	USoundCue* ImpactSound;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	float Health;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	float MaxHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	FString HeadBone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	float HealthBarDisplayTime;

	FTimerHandle HealthBarTimer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UAnimMontage* HitMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UAnimMontage* AttackMontage;

	void PlayHitMontageAccordingToHitDirection(FHitResult HitResult);

	FTimerHandle HitReactTimer;
	bool bCanHitReact;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	float HitReactTimeMin;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	float HitReactTimeMax;

	UPROPERTY(VisibleAnywhere, Category = Combat, meta = (AllowPrivateAccess = "true"))
	TMap<UUserWidget*, FVector> HitNumbers;

	UPROPERTY(EditAnywhere, Category = Combat, meta = (AllowPrivateAccess = "true"))
	float HitNumberDestroyTime;

	UPROPERTY(EditAnywhere, Category = "Behavior Tree", meta = (AllowPrivateAccess = "true"))
	UBehaviorTree* BehaviorTree;

	UPROPERTY(EditAnywhere, Category = "Behavior Tree", meta = (AllowPrivateAccess = "true", MakeEditWidget = "true"))
	FVector PatrolPoint;

	UPROPERTY(EditAnywhere, Category = "Behavior Tree", meta = (AllowPrivateAccess = "true", MakeEditWidget = "true"))
	FVector PatrolPoint2;

	UPROPERTY()
	AEnemyController* EnemyController;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	USphereComponent* AgroSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	bool bStunned;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	float StunChance;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	bool bInAttackRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	USphereComponent* CombatRangeSphere;

	FName AttackLFast;
	FName AttackRFast;
	FName AttackL;
	FName AttackR;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	float BaseDamage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	USoundCue* MeleeImpactSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	FName LeftWeaponSocket;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	FName RightWeaponSocket;
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator,
	                         AActor* DamageCauser) override;

	FORCEINLINE FString GetHeadBone() const { return HeadBone; }
	FORCEINLINE UBehaviorTree* GetBehaviorTree() const { return BehaviorTree; }

	UFUNCTION(BlueprintImplementableEvent)
	void ShowHitNumber(int32 Damage, FVector HitLocation, bool bHeadShot);
};
