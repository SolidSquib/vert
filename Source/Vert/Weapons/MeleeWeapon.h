// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "GameFramework/Actor.h"
#include "BaseWeapon.h"
#include "MeleeWeapon.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMeleeAttackExecute, int32, comboDepth);

UCLASS()
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
	virtual void SimulateWeaponFire() override;
	virtual void StopSimulatingWeaponFire() override;
	virtual bool FireWeapon_Implementation() override;

	UFUNCTION(BlueprintCallable, Category = "Attack|Notify")
	void NotifyAttackBegin();

	UFUNCTION(BlueprintCallable, Category = "Attack|Notify")
	void NotifyAttackEnd();

	UFUNCTION(BlueprintNativeEvent, Category = Attack)
	void OnWeaponEndOverlap(UPrimitiveComponent* overlappedComp, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex);

	UFUNCTION(BlueprintNativeEvent, Category = Attack)
	void OnWeaponBeginOverlap(UPrimitiveComponent* overlappedComp, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool fromSweep, const FHitResult& sweepResult);

protected:
	float mLastHitTime = 0.f;
	int32 mComboDepth = 0;
	bool mDidHit = false;
};
