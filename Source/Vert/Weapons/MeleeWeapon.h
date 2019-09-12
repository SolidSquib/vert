// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "GameFramework/Actor.h"
#include "BaseWeapon.h"
#include "MeleeWeapon.generated.h"

USTRUCT(BlueprintType)
struct FWeaponComboAttack
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	FWeaponAnim AttackAnim;

	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
	int32 BonusDamage;

	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
	float BonusBaseKnockback;

	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
	float ForwardImpulseMagnitude = 1000.f;

	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
	bool AllowAnimationClipping = true;

	UPROPERTY(EditDefaultsOnly, Category = WeaponState, meta = (EditCondition = "AllowAnimationClipping"))
	float ClipTime = 0.5f;
};

USTRUCT(BlueprintType)
struct FMeleeWeaponTraceConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FName TraceStartSocket = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FVector BoxTraceHalfExtent = FVector::OneVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float TraceLength = 10.f;
};

UCLASS(Abstract, Blueprintable)
class VERT_API AMeleeWeapon : public ABaseWeapon
{
	GENERATED_UCLASS_BODY()
		
protected:
	// The range within which the attack animations should loop. e.g. a range of 0 would be left punch, a range of 1 would be left punch -> right punch
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Melee", meta = (EditCondition = "UseComboAnimations"))
	int32 AnimLoopRange = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Melee", meta = (EditCondition = "UseComboAnimations"))
	float TimeForComboToDie = 0.7f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Melee")
	bool UseComboAnimations = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Melee")
	bool ManualComboManagement = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool ShowDebug = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	FName FXSocketTop = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	FName FXSocketBottom = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitScanning")
	FMeleeWeaponTraceConfig BoxTrace;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Melee", meta = (EditCondition="UseComboAnimations"))
	TArray<FWeaponComboAttack> ComboAttackAnims;
	
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
	void EnableTrace();
	void DisableTrace();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void ClientSimulateWeaponAttack_Implementation() override;
	virtual void ClientStopSimulateWeaponAttack_Implementation() override;
	virtual bool AttackWithWeapon_Implementation() override;
	virtual void NotifyAttackAnimationActiveStarted_Implementation() override;
	virtual void NotifyAttackAnimationActiveEnded_Implementation() override;
	virtual UClass* GetWeaponType_Implementation() const override;
	virtual void NotifyAttackAnimationEnded_Implementation() override;
	virtual void WeaponAttackFire(float ratio = 0.f) override;
	virtual void Reset() override;
	virtual void StartAttacking();

	UFUNCTION(BlueprintCallable, Category = "HitScanning")
	TArray<FHitResult> BoxTraceForHits();

	UFUNCTION(BlueprintCallable, Category = "Combo")
	bool DidLastAttackHit() const { return mDidHit; }

	UFUNCTION(BlueprintCallable, Category = "Combo")
	void IncrementComboDepth();

	UFUNCTION(BlueprintCallable, Category = "Combo")
	void DecrementComboDepth();

	UFUNCTION(BlueprintCallable, Category = "Combo")
	void ResetComboDepth();

	UFUNCTION(BlueprintCallable, Category = "Combo")
	FORCEINLINE int32 GetComboDepth() const { return mComboDepth; }

protected:
	float mLastHitTime = 0.f;
	int32 mComboDepth = 0;
	int32 mLastAttackUsed = -1;
	int32 mCurrentAnimationIndex = 0;
	bool mDidHit = false;
	bool mComboInProgress = false;
	bool mClipAnimation = false;
	bool mIsAttackAnimationPlaying = false;
	FTimerHandle mTimerHandle_AnimationClip;
	FTimerHandle mTimerHandle_TraceTimer;
	FTimerHandle mTimerHandle_ComboDeathTime;
	TArray<TWeakObjectPtr<AActor>> mCurrentHitActors;
};
