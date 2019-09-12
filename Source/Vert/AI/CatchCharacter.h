// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "CoreMinimal.h"
#include "Character/VertCharacterBase.h"
#include "CatchCharacter.generated.h"

UENUM(BlueprintType)
enum class ECatchAICombatState : uint8
{
	CCAI_INACTIVE UMETA(DisplayName = "Inactive"),
	CCAI_ENTERING UMETA(DisplayName = "Entering"),
	CCAI_FIGHTING UMETA(DisplayName = "Fighting"),
	CCAI_FLEEING UMETA(DisplayName = "Fleeing"),
	CCAI_RESTING UMETA(DisplayName = "Resting")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCatchStateChangedDelegate);

UCLASS()
class VERT_API ACatchCharacter : public AVertCharacterBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnCatchStateChangedDelegate OnReadyToAttack;

	UPROPERTY(BlueprintAssignable)
	FOnCatchStateChangedDelegate OnCatchLanded;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Entering")
	AActor* TargetEntryPoint = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fleeing")
	AActor* TargetFleePoint = nullptr;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Jump")
	TSubclassOf<class UCameraShake> LandingCameraShake = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooldowns")
	float ChestPoundCooldownTime = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooldowns")
	float ChargeCooldownTime = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooldowns")
	float SpitCooldownTime = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooldowns")
	float SlamCooldownTime = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooldowns")
	float PunchCooldownTime = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooldowns")
	float RoarCooldownTime = 10.f;

public:
	// Sets default values for this character's properties
	ACatchCharacter(const class FObjectInitializer& ObjectInitializer);

	virtual float TakeDamage(float Damage, const FDamageEvent& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser) override;

	UFUNCTION(BlueprintCallable, Category = "Target")
	void UpdateCharacter(AActor* targetActor = nullptr, const FVector& targetVector = FVector(0,0,0));

	UFUNCTION(BlueprintCallable, Category = "Target")
	AActor* ChooseTargetActor();

	UFUNCTION(BlueprintCallable, Category = "Cooldowns")
	bool IsChestPoundOnCooldown() const;

	UFUNCTION(BlueprintCallable, Category = "Cooldowns")
	bool IsChargeOnCooldown() const;

	UFUNCTION(BlueprintCallable, Category = "Cooldowns")
	bool IsSpitOnCooldown() const;

	UFUNCTION(BlueprintCallable, Category = "Cooldowns")
	bool IsSlamOnCooldown() const;

	UFUNCTION(BlueprintCallable, Category = "Cooldowns")
	bool IsPunchOnCooldown() const;

	UFUNCTION(BlueprintCallable, Category = "Cooldowns")
	bool IsRoarOnCooldown() const;

	UFUNCTION(BlueprintCallable, Category = "Activation")
	void NotifyArenaIsReadyToEnter();

	UFUNCTION(BlueprintCallable, Category = "Activation")
	void NotifyEntryStateFinished();

	UFUNCTION(BlueprintCallable, Category = "Activation")
	bool GetIsReadyToEnterArena() const { return !mIsWaitingToEnterArena; }

	UFUNCTION(BlueprintCallable, Category = "Activation")
	bool GetIsReadyForCombat() const { return mIsReadyForCombat; }

	UFUNCTION(BlueprintCallable, Category = "State")
	bool IsRunning() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "CombatActions")
	void ExecuteRoarAttack();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "CombatActions")
	void ExecuteChestPound();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "CombatActions")
	void ExecutePunchAttack();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "CombatActions")
	void ExecuteSlamAttack();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "CombatActions")
	void ExecuteSpitAttack();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "CombatActions")
	void ExecuteChargeAttack();

	UFUNCTION(BlueprintCallable, Category = "Heuristics")
	float GetDamageDealtByPlayer(class AController* player) const;

	UFUNCTION(BlueprintCallable, Category = "Heuristics")
	const TMap<AController*, float>& GetDamageDealtMap() const;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void NotifyJumpApex() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintImplementableEvent)
	void OnJumpApex();

private:
	FTimerHandle mTimerHandle_PunchCooldown;
	FTimerHandle mTimerHandle_ChargeCooldown;
	FTimerHandle mTimerHandle_SlamCooldown;
	FTimerHandle mTimerHandle_SpitCooldown;
	FTimerHandle mTimerHandle_RoarCooldown;
	FTimerHandle mTimerHandle_ChestPoundCooldown;

	bool mIsWaitingToEnterArena = true;
	bool mIsReadyForCombat = false;
	TMap<AController*, float> mDamageMap;
};
