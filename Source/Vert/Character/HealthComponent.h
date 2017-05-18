// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHitDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHealDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeathDelegate, const FTakeHitInfo&, lastHit);

UENUM()
enum class EEffectiveDamageType : uint8
{
	ActualDamageModifier,
	ShownDamageModifier,
	None
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class VERT_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnHitDelegate OnHit;

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnHealDelegate OnHeal;

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnDeathDelegate OnDeath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float DamageApplicationRate = 100.f;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Health")
	float DamageModifier;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	EEffectiveDamageType EffectiveDamageType = EEffectiveDamageType::ActualDamageModifier;

	/** Replicate where this pawn was last hit and damaged */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_LastTakeHitInfo)
	struct FTakeHitInfo LastTakeHitInfo;

public:	
	// Sets default values for this component's properties
	UHealthComponent();

	UFUNCTION(BlueprintCallable, Category = "Health")
	int32 DealDamage(float DamageTaken, const struct FDamageEvent& DamageEvent, class APawn* PawnInstigator, class AActor* DamageCauser);

	UFUNCTION(BlueprintCallable, Category = "Health")
	int32 HealDamage(int32 magnitude);

	UFUNCTION(BlueprintCallable, Category = "Health")
	void Kill(const FHitResult& hit);

	UFUNCTION(BlueprintCallable, Category = "Health")
	int32 GetCurrentDamageModifier() const;

	UFUNCTION(BlueprintCallable, Category = "Character")
	FORCEINLINE ACharacter* GetCharacterOwner() const { if (mCharacterOwner.IsValid()) { return mCharacterOwner.Get(); } return nullptr; }

protected:
	void ReplicateHit(float Damage, struct FDamageEvent const& DamageEvent, class APawn* InstigatingPawn, class AActor* DamageCauser, bool bKilled);

	virtual void PlayHit(float DamageTaken, const struct FDamageEvent& DamageEvent, class APawn* PawnInstigator, class AActor* DamageCauser);
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type endPlayReason) override;

	UFUNCTION()
	void OnRep_LastTakeHitInfo();

private:
	void SetDamageTaken(int32 totalDamage);
	void LaunchCharacter(const FDamageEvent& damageEvent, const ABaseWeapon* damageCauser);

private:
	int32 mDamageTaken = 0;
	int32 mShownDamageTaken = 0;
	float mShownDamageTakenFloat = 0;
	float mLastTakeHitTimeTimeout; /** Time at which point the last take hit info for the actor times out and won't be replicated; Used to stop join-in-progress effects all over the screen */
	FTimerHandle mUpdateShownDamageTakenTimer;
	TWeakObjectPtr<ACharacter> mCharacterOwner = nullptr;
};
