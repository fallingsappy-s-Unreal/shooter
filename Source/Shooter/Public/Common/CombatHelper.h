#pragma once

enum class EHitDirection : uint8;

class CombatHelper
{
public:
	static EHitDirection GetHitDirection(AActor* Actor, FHitResult HitResult);
};
