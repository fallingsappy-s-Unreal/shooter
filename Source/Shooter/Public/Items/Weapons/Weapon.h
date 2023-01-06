// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/Item.h"
#include "Weapon.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTER_API AWeapon : public AItem
{
	GENERATED_BODY()

public:
	AWeapon();
	virtual void Tick(float DeltaSeconds) override;
protected:
	void StopFalling();
private:
	FTimerHandle ThrowWeaponTimer;
	float ThrowWeaponTime;
	bool bFalling;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	int32 Ammo;
public:
	void ThrowWeapon();

	FORCEINLINE int32 GetAmmo() const { return Ammo; }

	void DecrementAmmo();
};
