// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapons/Weapon.h"
#include "Items/Weapons/WeaponType.h"
#include "Items/Weapons/AmmoType.h"

AWeapon::AWeapon() :
	ThrowWeaponTime(0.7f),
	bFalling(false),
	Ammo(0),
	MagazineCapacity(30),
	WeaponType(EWeaponType::EWT_SubmachineGun),
	AmmoType(EAmmoType::EAT_9mm),
	ReloadMontageSection(FName(TEXT("Reload SMG"))),
	ClipBoneName(TEXT("smg_clip"))
{
	PrimaryActorTick.bCanEverTick = true;
}

void AWeapon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Keep the Weapon upright
	if (GetItemState() == EItemState::EIS_Falling && bFalling)
	{
		const FRotator MeshRotation{0.f, GetItemMesh()->GetComponentRotation().Yaw, 0.f};
		GetItemMesh()->SetWorldRotation(MeshRotation, false, nullptr, ETeleportType::TeleportPhysics);
	}
}

void AWeapon::ThrowWeapon()
{
	FRotator MeshRotation(0.f, GetItemMesh()->GetComponentRotation().Yaw, 0.f);
	GetItemMesh()->SetWorldRotation(MeshRotation, false, nullptr, ETeleportType::TeleportPhysics);

	const FVector MeshForward{GetItemMesh()->GetForwardVector()};
	const FVector MeshRight{GetItemMesh()->GetRightVector()};

	// Direction in which we throw the Weapon
	FVector ImpulseDirection = MeshRight.RotateAngleAxis(-20.f, MeshForward);

	const float RandomRotation{FMath::FRandRange(0.f, 180.f)};
	ImpulseDirection = ImpulseDirection.RotateAngleAxis(RandomRotation, FVector(0.f, 0.f, 1.f));
	ImpulseDirection *= 200'0.f;
	GetItemMesh()->AddImpulse(ImpulseDirection);

	bFalling = true;
	GetWorldTimerManager().SetTimer(ThrowWeaponTimer, this, &AWeapon::StopFalling, ThrowWeaponTime);

	EnableGlowMaterial();
}

void AWeapon::DecrementAmmo()
{
	Ammo = FMath::Clamp(Ammo, 0, --Ammo);
}

void AWeapon::ReloadAmmo(int32 Amount)
{
	Ammo += FMath::Clamp(Amount, 0, MagazineCapacity);
}

bool AWeapon::ClipIsFull() const
{
	return Ammo >= MagazineCapacity;
}

void AWeapon::StopFalling()
{
	bFalling = false;
	SetItemState(EItemState::EIS_Pickup);
	ResetPulseTimer();
}

void AWeapon::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	const FString WeaponTablePath(TEXT("DataTable'/Game/_Game/DataTables/WeaponDataTable.WeaponDataTable'"));
	UDataTable* WeaponTableObject = Cast<UDataTable>(
		StaticLoadObject(UDataTable::StaticClass(), nullptr, *WeaponTablePath));
	if (WeaponTableObject)
	{
		FWeaponDataTable* WeaponDataRow = nullptr;
		switch (WeaponType)
		{
		case EWeaponType::EWT_SubmachineGun:
			WeaponDataRow = WeaponTableObject->FindRow<FWeaponDataTable>(FName("SubmachineGun"), TEXT(""));
			break;
		case EWeaponType::EWT_AssaultRifle:
			WeaponDataRow = WeaponTableObject->FindRow<FWeaponDataTable>(FName("AssaultRifle"), TEXT(""));
			break;
		}

		if (WeaponDataRow)
		{
			AmmoType = WeaponDataRow->AmmoType;
			Ammo = WeaponDataRow->WeaponAmmo;
			MagazineCapacity = WeaponDataRow->MagazineCapacity;
			SetPickupSound(WeaponDataRow->PickupSound);
			SetEquipSound(WeaponDataRow->EquipSound);
			GetItemMesh()->SetSkeletalMesh(WeaponDataRow->ItemMesh);
			SetItemName(WeaponDataRow->ItemName);
			SetIconAmmo(WeaponDataRow->AmmoIcon);
			SetIconItem(WeaponDataRow->InventoryIcon);
			SetMaterialInstance(WeaponDataRow->MaterialInstance);

			PreviousMaterialIndex = GetMaterialIndex();
			GetItemMesh()->SetMaterial(PreviousMaterialIndex, nullptr);
			SetMaterialIndex(WeaponDataRow->MaterialIndex);
			ClipBoneName = WeaponDataRow->ClipBoneName;
			ReloadMontageSection = WeaponDataRow->ReloadMontageSection;
		}

		if (GetMaterialInstance())
		{
			SetDynamicMaterialInstance(UMaterialInstanceDynamic::Create(GetMaterialInstance(), this));
			GetDynamicMaterialInstance()->SetVectorParameterValue(TEXT("FresnelColor"), GetGlowColor());
			GetItemMesh()->SetMaterial(GetMaterialIndex(), GetDynamicMaterialInstance());
			EnableGlowMaterial();
		}
	}
}
