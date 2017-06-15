// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "GameFramework/Actor.h"
#include "BaseWeapon.h"
#include "MeleeWeapon.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMeleeAttackExecute, int32, comboDepth);

UCLASS(Abstract, Blueprintable)
class VERT_API AMeleeWeapon : public ABaseWeapon
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnMeleeAttackExecute OnMeleeAttack;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	FName FXSocketTop = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	FName FXSocketBottom = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitScanning")
	TArray<FName> ScanSockets;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitScanning")
	float MeleeTraceRange = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	UParticleSystem* AttackStartFX = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	UParticleSystem* ArcFX = nullptr;

	UPROPERTY(Transient)
	UParticleSystemComponent* ArcPSC = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	UParticleSystem* AttackEndFX = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	UParticleSystem* StrikeFX = nullptr;

protected:	
	virtual void BeginPlay() override;
	virtual void ClientSimulateWeaponAttack_Implementation() override;
	virtual void ClientStopSimulateWeaponAttack_Implementation() override;
	virtual bool AttackWithWeapon_Implementation() override;
	virtual void NotifyAttackAnimationActiveStarted_Implementation() override;
	virtual void NotifyAttackAnimationActiveEnded_Implementation() override;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void MeleeAttackWithWeapon();

protected:
	float mLastHitTime = 0.f;
	int32 mComboDepth = 0;
	bool mDidHit = false;
	bool mTraceHit = false;
	bool mAttackDone = false;
};
