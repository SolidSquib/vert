// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Engine/VertGlobals.h"
#include "VertCharacterBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTakeDamageDelegate, float, Damage, TSubclassOf<UDamageType>, DamageType);

UCLASS(config = Game, Abstract)
class VERT_API AVertCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnTakeDamageDelegate OnDamageTaken;

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Items")
	float ThrowForce = 10000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	bool CameraShouldFollow = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health")
	TArray<TSubclassOf<UDamageType>> IgnoredDamageTypes;

protected:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Health")
	class UHealthComponent* HealthComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health", meta = (UIMin = "0", UIMax = "1", ClampMin = "0", ClampMax = "1"))
	float GlobalDamageResistance = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health")
	bool ImmuneToStun = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health")
	bool ImmuneToKnockback = false;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	class UParticleSystem* DeathFX = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	class UAkAudioEvent* PlayerDeathSound = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	class UParticleSystem* KnockbackTrailFX = nullptr;

	UPROPERTY()
	class UParticleSystemComponent* KnockbackTrailPSC = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	float KnockbackTrailTriggerMagnitude = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	float KnockbackTrailDestroySpeed = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "ForceFeedback")
	class UForceFeedbackEffect* DeathFeedback = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "CameraShake")
	TSubclassOf<class UCameraShake> DeathCameraShakeEvent = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "CameraShake")
	float DeathCameraShakeScale = 1.f;

public:
	// Sets default values for this character's properties
	AVertCharacterBase(const class FObjectInitializer& ObjectInitializer);

	bool IsMoving();
	bool CanComponentRecharge(ERechargeRule rule);

	virtual bool KilledBy(class APawn* EventInstigator);
	virtual bool CanDie() const;
	virtual void OnDeath(float killingDamage, const FDamageEvent& damageEvent, APawn* pawnInstigator, AActor* damageCauser);
	virtual void ApplyDamageMomentum(float DamageTaken, const FDamageEvent& DamageEvent, APawn* PawnInstigator, AActor* DamageCauser) override;
	virtual void FellOutOfWorld(const class UDamageType& dmgType) override;

	UFUNCTION(BlueprintCallable, Category = "Hitstun")
	bool IsInHitstun() const;

	UFUNCTION(BlueprintCallable, Category = "Floor")
	AActor* GetFloorActor();

	UFUNCTION(BlueprintCallable, Category = "Health")
	virtual float TakeDamage(float Damage, const FDamageEvent& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser) override;

	UFUNCTION(BlueprintCallable, Category = "Health")
	virtual bool Die();

	UFUNCTION(BlueprintCallable, Category = "CharacterMovement")
	FORCEINLINE bool IsGrounded() const { return !IsJumpProvidingForce() && !GetCharacterMovement()->IsFalling(); }

	UFUNCTION(BlueprintCallable, Category = "Effects")
	UParticleSystem* GetDeathFX() const { return DeathFX; }

protected:
	virtual void ApplyDamageHitstun(float hitstunTime);
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "Health")
	virtual bool Suicide();

	UFUNCTION(BlueprintCallable, Category = "Health")
	virtual bool DieInternal(float KillingDamage, const FDamageEvent& DamageEvent, class AController* Killer, class AActor* DamageCauser);

private:
	void SetRagdollPhysics();
	void InternalTakeRadialKnockback(float Damage, float StunTime, struct FVertRadialDamageEvent& RadialDamageEvent, class AController* EventInstigator, class AActor* DamageCauser);
	void CheckForEndKnockbackTrail();

protected:
	FTimerHandle mHitStunTimer;
	FTimerHandle mTimerHandle_TrailCheck;

	bool mIsDying = false;
	bool mIsRagdolling = false;
};