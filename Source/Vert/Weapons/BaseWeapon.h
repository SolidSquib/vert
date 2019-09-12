// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BaseWeapon.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogVertBaseWeapon, Log, All);

UENUM(BlueprintType)
enum class EWeaponAnimationMode : uint8
{
	Unarmed,
	LightMelee,
	HeavyMelee,
	PistolRanged,
	RifleRanged,
	HeavyRanged
};

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	PassiveIdle,
	CombatIdle,
	CombatIdleWithIntentToFire,
	Reloading,
	Equipping,
};

UENUM(BlueprintType)
enum class EFiringMode : uint8
{
	Automatic,
	SemiAutomatic,
	Burst
};

USTRUCT(BlueprintType)
struct FWeaponAnim
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player")
	UAnimSequence* PlayerAnim = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player")
	bool HoldForAnimationEnd = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player")
	bool IsFullBody = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player")
	bool IgnoreRecoil = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player")
	bool BlendTransition = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player")
	bool UsePoseAsset = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player", meta = (EditCondition = "UsePoseAsset"))
	class UPoseAsset* OptionalPoseAsset = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UAnimSequence* WeaponAnim = nullptr;
};

USTRUCT(BlueprintType)
struct FThrownWeaponData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	int32 BaseDamage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	float Knockback;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	float KnockbackScaling;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	float StunTime;

	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
	TSubclassOf<UDamageType> DamageType;

	UPROPERTY(EditDefaultsOnly, Category = "Physics")
	bool Piercing = false;

	FThrownWeaponData()
	{
		BaseDamage = 10;
		Knockback = 20.f;
		KnockbackScaling = 0.01f;
		DamageType = UDamageType::StaticClass();
		StunTime = 0;
	}
};

USTRUCT(BlueprintType)
struct FWeaponData
{
	GENERATED_USTRUCT_BODY()

	/** inifite ammo for reloads */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Ammo)
	bool InfiniteAmmo;

	/** infinite ammo in clip, no reload required */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Ammo)
	bool InfiniteClip;
	
	/** clip size */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Ammo)
	int32 AmmoPerClip;

	/** initial clips */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Ammo)
	int32 InitialClips;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = WeaponStat)
	int32 BaseDamage;

	/** type of damage */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = WeaponStat)
	TSubclassOf<UDamageType> DamageType;

	/** time between two consecutive shots */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = WeaponStat)
	float TimeBetweenShots;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = WeaponStat)
	EFiringMode FiringMode;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = WeaponStat, Meta = (EditCondition = "FiringMode == EFiringMode::Burst"))
	int32 BurstNumberOfShots;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = WeaponStat, Meta = (EditCondition = "FiringMode == EFiringMode::Burst"))
	float BurstTimeBetweenShots;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = WeaponStat)
	float BaseKnockback;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = WeaponStat)
	float KnockbackScaling;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = WeaponStat)
	float StunTime;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = WeaponStat)
	float AutoAttackTime;

	/** defaults */
	FWeaponData()
	{
		InfiniteAmmo = false;
		InfiniteClip = false;
		AmmoPerClip = 20;
		InitialClips = 4;
		BaseDamage = 5;
		DamageType = UDamageType::StaticClass();
		TimeBetweenShots = 0.2f;
		FiringMode = EFiringMode::Automatic;
		BurstNumberOfShots = 3;
		BurstTimeBetweenShots = 0.2f;
		BaseKnockback = 10.f;
		KnockbackScaling = 1.5f;
		StunTime = 0;
		AutoAttackTime = 0.5f;
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnWeaponStateChangedDelegate, ABaseWeapon*, weapon, EWeaponState, state, UAnimSequence*, anim);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponAttackFire, const FWeaponAnim&, animToPlay, float, ratio);

UCLASS(Abstract)
class ABaseWeapon : public AInteractive
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnWeaponStateChangedDelegate Delegate_OnWeaponStateChanged;

	UPROPERTY(BlueprintAssignable)
	FOnWeaponAttackFire Delegate_OnWeaponAttackFire;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aim")
	bool UseControllerAim = true;

	/** pawn owner */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_MyPawn)
	class AVertCharacter* MyPawn;

	/** weapon data */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Config)
	FWeaponData WeaponConfig;

	UPROPERTY(EditDefaultsOnly, Category = Config)
	FThrownWeaponData ThrowingConfig;
	
	/** camera shake on firing */
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	TSubclassOf<UCameraShake> FireCameraShake;

	/** force feedback effect to play when the weapon is fired */
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	UForceFeedbackEffect *FireForceFeedback;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	class UAkAudioEvent* ThrowImpactSound = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	UAkAudioEvent* OutOfAmmoSound = nullptr;

	/** reload animations */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Animation)
	FWeaponAnim ReloadAnim;

	/** equip animations */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Animation)
	FWeaponAnim EquipAnim;

	/** fire animations */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Animation)
	FWeaponAnim AttackAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	FWeaponAnim PassiveIdleAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Animation)
	FWeaponAnim CombatIdleAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Animation)
	FWeaponAnim LedgeHangOverrideAnim;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Animation)
	FWeaponAnim RunOverrideAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Animation)
	FWeaponAnim TauntOverrideAnim;

	/* Mainly for testing purposes, ignores the wait for an animation to notify after euip begin or reload begin */
	UPROPERTY(EditDefaultsOnly, Category = "Animation|Overrides")
	uint32 OverrideAnimCompleteNotify : 1;
	
	uint32 IsWaitingForAttackAnimEnd : 1;

	/** is weapon currently equipped? */
	uint32 IsEquipped : 1;

	/** is weapon fire active? */
	uint32 WantsToFire : 1;

	/** is reload animation playing? */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_Reload)
	uint32 PendingReload : 1;

	/** is equip animation playing? */
	uint32 PendingEquip : 1;

	/** weapon is refiring */
	uint32 Refiring : 1;

	uint32 mAttackSpent : 1;
	uint32 mCombatIdle : 1;
	uint32 mDashAttacking : 1;

	/** current total ammo */
	UPROPERTY(Transient, Replicated)
	int32 CurrentAmmo;

	/** current ammo - inside clip */
	UPROPERTY(Transient, Replicated)
	int32 CurrentAmmoInClip;

	/** burst counter, used for replicating fire events to remote clients */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_BurstCounter)
	int32 BurstCounter;

	UPROPERTY(EditDefaultsOnly, Category = "Config|ObjectPool")
	float DespawnTriggerTime = 15.f;

	UPROPERTY(EditDefaultsOnly, Category = "Config|ObjectPool")
	float FlashSpeed = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Config|ObjectPool")
	float FlashForTimeBeforeDespawning = 3.f;

	UPROPERTY(EditDefaultsOnly, Category = "Config|ObjectPool")
	class UParticleSystem* DespawnFX = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh)
	USkeletalMeshComponent* WeaponMesh;

public:	
	void GiveAmmo(int AddAmount); /** [server] add ammo */
	void UseAmmo(); /** consume a bullet */
	bool HasInfiniteAmmo() const; /** check if weapon has infinite ammo (include owner's cheats) */
	bool HasInfiniteClip() const; /** check if weapon has infinite clip (include owner's cheats) */
	void SetOwningPawn(AVertCharacter* AVertCharacter); /** set the weapon's owning pawn */
	float GetEquipStartedTime() const; /** gets last time when this weapon was switched to */
	float GetEquipDuration() const; /** gets the duration of equipping weapon*/
	bool IsWeaponEquipped() const; /** check if it's currently equipped */
	bool IsAttachedToPawn() const; /** check if mesh is already attached */
	bool CanFire() const; /** check if weapon can fire */
	bool CanReload() const; /** check if weapon can be reloaded */
	int32 GetAmmoPerClip() const; /** get clip size */
	int32 GetMaxAmmo() const; /** get max ammo amount */
	bool GetWantsToAttack() const;
	void Pickup(AVertCharacter* NewOwner); /** [server] weapon was added to pawn's inventory */

	virtual void StartEquipping();
	virtual void OnDrop(); /** [server] weapon was removed from pawn's inventory */
	virtual void Interact(const TWeakObjectPtr<class UCharacterInteractionComponent>& instigator) final;
	virtual void PostInitializeComponents() override;
	virtual void Destroyed() override;
	virtual void StartAttacking(); /** [local + server] start weapon fire */
	virtual void StopAttacking(bool forced = false); /** [local + server] stop weapon fire */
	virtual void StartReload(bool bFromReplication = false); /** [all] start weapon reload */
	virtual void StopReload(); /** [local + server] interrupt weapon reload */
	virtual void ReloadWeapon(); /** [server] performs actual reload */
	virtual void Reset();

	/** trigger reload from server */
	UFUNCTION(reliable, client)
	void ClientStartReload();

	UFUNCTION(BlueprintCallable, Category = Weapon)
	EWeaponState GetCurrentState() const; /** get current weapon state */

	/** get pawn owner */
	UFUNCTION(BlueprintCallable, Category = "Game|Weapon")
	class AVertCharacter* GetPawnOwner() const;

	UFUNCTION(BlueprintCallable, Category = "WeaponStats")
	FORCEINLINE int32 GetBaseDamage() const { return WeaponConfig.BaseDamage; }
	
	UFUNCTION(BlueprintCallable, Category = "AnimationNotifies")
	FORCEINLINE bool IsAttackAnimationFinished() const { return !IsWaitingForAttackAnimEnd; }

	UFUNCTION(BlueprintCallable, Category = WeaponStats)
	FORCEINLINE float GetAutoAttackTime() const { return WeaponConfig.AutoAttackTime; }

	UFUNCTION(BlueprintCallable, Category = Ammo)
	int32 GetCurrentAmmo() const; /** get current ammo amount (total) */

	UFUNCTION(BlueprintCallable, Category = Ammo)
	int32 GetCurrentAmmoInClip() const; /** get current ammo amount (clip) */
	
	/* Broadcasts the OnWeaponFiredWithRecoil delegate with a custom recoil amount; to be used in cases where more custom recoil behaviour is required. */
	UFUNCTION(BlueprintCallable, Category = "Delegates")
	virtual void WeaponAttackFire(float ratio = 0.f);

	UFUNCTION(BlueprintCallable, Category = "Attack")
	int32 GetBonusDamage() const { return mBonusDamage; }

	UFUNCTION(BlueprintCallable, Category = "Attack")
	float GetBonusKnockback() const { return mBonusKnockback; }

	/* [LOCAL} animation notify to tell us it's alright to leave the Equipping state */
	UFUNCTION(BlueprintCallable, Category = "AnimationNotifies")
	void NotifyEquipAnimationEnded();

	/* [LOCAL} animation notify to tell us it's alright to leave the Reloading state */
	UFUNCTION(BlueprintCallable, Category = "AnimationNotifies")
	void NotifyReloadAnimationEnded();

	/* Blueprintable function to add a dash specific attack to a weapon. Surpasses standard attack code in exchange for more customization. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Attack")
	bool DashAttack();

	/* [LOCAL} animation notify to tell us it's alright to leave the Attacking state */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "AnimationNotifies")
	void NotifyAttackAnimationActiveStarted();

	/* [LOCAL} animation notify to tell us it's alright to leave the Attacking state */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "AnimationNotifies")
	void NotifyAttackAnimationActiveEnded();

	/* [LOCAL} animation notify to tell us it's alright to leave the Attacking state */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "AnimationNotifies")
	void NotifyAttackAnimationEnded();

	UFUNCTION(BlueprintCallable, Category = "Throwing")
	void EnableWeaponPhysics(bool simulate = false, bool shouldBlockPawn = true, const FVector& initialImpulse = FVector::ZeroVector, float blockDelay = 0.f);

	UFUNCTION(BlueprintCallable, Category = "Throwing")
	void DisableWeaponPhysics();

	UFUNCTION(BlueprintCallable)
	FORCEINLINE float GetBaseKnockback() const { return WeaponConfig.BaseKnockback; }

	UFUNCTION(BlueprintCallable)
	FORCEINLINE float GetKnockbackScaling() const { return WeaponConfig.KnockbackScaling; }

protected:
	UAnimSequence* GetPlayerAnimForState(EWeaponState state);
	void SimulateWeaponAttack(); /** Called in network play to do the cosmetic fx for firing */
	void StopSimulatingWeaponAttack(); /** Called in network play to stop cosmetic fx (e.g. for a looping shot). */
	void HandleAttacking(); /** [local + server] handle weapon fire */
	void SetWeaponState(EWeaponState NewState, bool broadcast = true); /** update weapon state */
	void DetermineWeaponState(bool broadcast = true); /** determine current weapon state */
	void AttachMeshToPawn(); /** attaches weapon mesh to pawn's mesh */
	void DetachMeshFromPawn(); /** detaches weapon mesh from pawn */
	void PlayWeaponAnimation(const FWeaponAnim& Animation); /** play weapon animations */
	void StopWeaponAnimation(const FWeaponAnim& Animation); /** stop playing weapon animations */
	bool WeaponNotAutomatic() const;
	void InitThrowVelocity(const FVector& throwDirection);
	void StartDespawnTimer();
	void PrepareForDespawn();
	void CancelDespawn();
	void DespawnFlash();
	void DisableAndDestroy();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnBurstStarted(); /** [local + server] firing started */
	virtual void OnBurstFinished(); /** [local + server] firing finished */
	
	UFUNCTION()
	void PoolBeginPlay();
	
	UFUNCTION()
	void PoolEndPlay();

	UFUNCTION(BlueprintNativeEvent, Category = "FX")
	void ClientSimulateWeaponAttack();

	UFUNCTION(BlueprintNativeEvent, Category = "FX")
	void ClientStopSimulateWeaponAttack();

	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	void WeaponInteract(UCharacterInteractionComponent* interactionComponent, AVertCharacter* character);
	
	/** [local] weapon specific fire implementation */
	UFUNCTION(BlueprintNativeEvent, Category = "Attack")
	bool AttackWithWeapon();

	/* Utility function to determine the base type of this weapon */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "WeaponType")
	UClass* GetWeaponType() const;

	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	void ThrowWeapon();

	/* [LOCAL] Called when a weapon has been equipped. */
	UFUNCTION(BlueprintImplementableEvent)
	void WeaponReadyFromPickup();

	/* [LOCAL] Called at the end of a burst, after a semi-auto shot, or when the weapon is out of ammo. */
	UFUNCTION(BlueprintImplementableEvent)
	void OnAttackFinished();

	UFUNCTION(BlueprintImplementableEvent)
	void OnStartAttacking();

	UFUNCTION(BlueprintImplementableEvent)
	void OnStopAttacking(bool forced = false);

	UFUNCTION(BlueprintCallable, Category = "Attack")
	void AddBonusDamageAndKnockback(int32 bonusDamage = 0, float bonusKnockback = 0.f);

	UFUNCTION(BlueprintCallable, Category = "Attack")
	void ResetBonusDamageAndKnockback();

	/** find hit */
	UFUNCTION(BlueprintCallable, Category = "Hit Detection")
	virtual FHitResult WeaponTrace(const FVector& TraceFrom, const FVector& TraceTo, bool useSphere = false, float sphereRadius = 0.f) const;

	/** find hits */
	UFUNCTION(BlueprintCallable, Category = "Hit Detection")
	virtual TArray<FHitResult> WeaponTraceWithPenetration(const FVector& TraceFrom, const FVector& TraceTo, bool useSphere = false, float sphereRadius = 0.f, bool ignoreBlocking = false) const;
	
	UFUNCTION(reliable, server, WithValidation)
	void ServerStartAttacking();

	UFUNCTION(reliable, server, WithValidation)
	void ServerStopAttacking(bool forced = false);

	UFUNCTION(reliable, server, WithValidation)
	void ServerStartReload();

	UFUNCTION(reliable, server, WithValidation)
	void ServerStopReload();

	/** [server] fire & update ammo */
	UFUNCTION(reliable, server, WithValidation)
	void ServerHandleAttacking();

	UFUNCTION()
	void OnRep_MyPawn();

	UFUNCTION()
	void OnRep_BurstCounter();

	UFUNCTION()
	void OnRep_Reload();

	UFUNCTION(BlueprintNativeEvent, Category = "Throwing")
	void OnBeginOverlap(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool fromSweep, const FHitResult& sweepResult);

	UFUNCTION()
	void OnComponentHit(UPrimitiveComponent* hitComponent, AActor* otherActor, UPrimitiveComponent* otherComp, FVector normalImpulse, const FHitResult& hit);

	UFUNCTION(BlueprintImplementableEvent, Category = "Physics")
	void OnWeaponHit(AActor* otherActor, UPrimitiveComponent* otherComp, FVector hitNormal);

protected:
	int32 mBonusDamage = 0.f;
	float mBonusKnockback = 0.f;
	float mLastFireTime = 0.f; /** time of last successful weapon fire */

	FCollisionResponseContainer mThrownCollisionResponses;
	FCollisionResponseContainer mSimulatedCollisionResponses;

	EWeaponState mCurrentState; /** current weapon state */
	FTimerHandle mTimerHandle_HandleFiring;
	FTimerHandle mTimerHandle_NonAutoTriggerDelay;
	FTimerHandle mTimerHandle_CombatIdle;
	FTimerHandle mTimerHandle_ThrowGravityTimer;
	FTimerHandle mTimerHandle_BlockDelay;
	FTimerHandle mTimerHandle_Despawn;
	FTimerHandle mTimerHandle_DespawnFlash;
	FTimerHandle mTimerHandle_DespawnFinish;

private:
	TArray<AActor*> mIgnoreThrowDamage;
};