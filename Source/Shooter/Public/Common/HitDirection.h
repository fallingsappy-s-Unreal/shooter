#pragma once

UENUM(BlueprintType)
enum class EHitDirection: uint8
{
	EHD_Front UMETA(DisplayName = "Front"),
	EHD_Back UMETA(DisplayName = "Back"),
	EHD_Left UMETA(DisplayName = "Left"),
	EHD_Right UMETA(DisplayName = "Right"),

	EHD_MAX UMETA(DisplayName = "DefaultMAX")
};