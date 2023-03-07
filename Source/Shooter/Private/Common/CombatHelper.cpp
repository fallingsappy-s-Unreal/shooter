#include "Common/CombatHelper.h"

#include "Common/HitDirection.h"
#include "Kismet/KismetMathLibrary.h"

EHitDirection CombatHelper::GetHitDirection(AActor* Actor, FHitResult HitResult)
{
	FVector ActorForwardVector = Actor->GetActorForwardVector();
	FVector ActorRightVector = Actor->GetActorRightVector();

	FVector ActorLocation = Actor->GetActorLocation();
	FVector Diff = HitResult.Location - ActorLocation;
	Diff.Normalize();
	FVector HitLoc = Diff;

	float ForwardVectorDotProduct = FVector::DotProduct(ActorForwardVector, HitLoc);
	float RightVectorDotProduct = FVector::DotProduct(ActorRightVector, HitLoc);

	if (UKismetMathLibrary::InRange_FloatFloat(ForwardVectorDotProduct, 0.5f, 1.0f))
	{
		return EHitDirection::EHD_Front;
	}
	if (UKismetMathLibrary::InRange_FloatFloat(ForwardVectorDotProduct, -1.0f, -0.5f))
	{
		return EHitDirection::EHD_Back;
	}
	if (UKismetMathLibrary::InRange_FloatFloat(RightVectorDotProduct, 0.0f, 1.0f))
	{
		return EHitDirection::EHD_Right;
	}
	if (UKismetMathLibrary::InRange_FloatFloat(RightVectorDotProduct, -1.0f, -0.0f))
	{
		return EHitDirection::EHD_Left;
	}

	return EHitDirection::EHD_Front;
}
