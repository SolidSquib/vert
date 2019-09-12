// Copyright Inside Out Games Limited 2017

#include "ArmouredCharacter.h"

//************************************
// Method:    TakeDamage
// FullName:  AArmouredCharacter::TakeDamage
// Access:    virtual public 
// Returns:   float
// Qualifier:
// Parameter: float Damage
// Parameter: const FDamageEvent & DamageEvent
// Parameter: class AController * EventInstigator
// Parameter: class AActor * DamageCauser
//************************************
float AArmouredCharacter::TakeDamage(float Damage, const FDamageEvent& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser)
{
	static const FName scArmourTag(TEXT("Armour"));

	bool attackBlocked = false;
	if (DamageEvent.IsOfType(FVertPointDamageEvent::ClassID))
	{
		FVertPointDamageEvent* pointDamage = (FVertPointDamageEvent*)&DamageEvent;
		attackBlocked = pointDamage->HitInfo.Component->ComponentHasTag(scArmourTag) ? true : false;
	}
	else if (DamageEvent.IsOfType(FVertRadialDamageEvent::ClassID))
	{
		FVertRadialDamageEvent* radialDamage = (FVertRadialDamageEvent*)&DamageEvent;
		FHitResult outHitResult;
		FVector outImpulseDir;
		radialDamage->GetBestHitInfo(this, nullptr, outHitResult, outImpulseDir);
		attackBlocked = outHitResult.Component->ComponentHasTag(scArmourTag) ? true : false;
	}

	if (!attackBlocked)
	{
		float actualDamage = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
		TakeDamageSucceeded(actualDamage);

		AVertGameMode* GameMode = GetWorld()->GetAuthGameMode<AVertGameMode>();
		AVertPlayerController* PlayerController = Cast<AVertPlayerController>(EventInstigator);
		if (GameMode && PlayerController && !PlayerController->IsPendingKill())
		{
			AVertPlayerState* PlayerState = Cast<AVertPlayerState>(PlayerController->PlayerState);
			GameMode->ScoreMonsterHunter(actualDamage, PlayerState);
		}

		return actualDamage;
	}

	TakeDamageFailed();
	return 0;
}