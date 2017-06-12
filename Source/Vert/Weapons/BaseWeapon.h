// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Sound/SoundCue.h"
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
	Idle,
	Firing,
	Reloading,
	Equipping,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnWeaponStateChangedDelegate, ABaseWeapon*, weapon, EWeaponState, state, UAnimSequence*, anim);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponFiredWithRecoil, float, recoilAmount);

UENUM(BlueprintType)
enum class EFiringMode : uint8
{
	Automatic,
	SemiAutomatic,
	Burst
};

USTRUCT()
struct FWeaponAnim
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	UAnimSequence* PlayerAnim;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	UAnimMontage* WeaponAnim = nullptr;
};

USTRUCT()
struct FWeaponData
{
	GENERATED_USTRUCT_BODY()

	/** inifite ammo for reloads */
	UPROPERTY(EditDefaultsOnly, Category = Ammo)
	bool InfiniteAmmo;

	/** infinite ammo in clip, no reload required */
	UPROPERTY(EditDefaultsOnly, Category = Ammo)
	bool InfiniteClip;

	/** max ammo */
	UPROPERTY(EditDefaultsOnly, Category = Ammo)
	int32 MaxAmmo;

	/** clip size */
	UPROPERTY(EditDefaultsOnly, Category = Ammo)
	int32 AmmoPerClip;

	/** initial clips */
	UPROPERTY(EditDefaultsOnly, Category = Ammo)
	int32 InitialClips;

	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
	int32 BaseDamage;

	/** type of damage */
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
	TSubclassOf<UDamageType> DamageType;

	/** time between two consecutive shots */
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
	float TimeBetweenShots;
	
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
	EFiringMode FiringMode;

	UPROPERTY(EditDefaultsOnly, Category = WeaponStat, Meta = (EditCondition = "FiringMode == EFiringMode::Burst"))
	int32 BurstNumberOfShots;

	UPROPERTY(EditDefaultsOnly, Category = WeaponStat, Meta = (EditCondition = "FiringMode == EFiringMode::Burst"))
	float BurstTimeBetweenShots;

	/** defaults */
	FWeaponData()
	{
		InfiniteAmmo = false;
		InfiniteClip = false;
		MaxAmmo = 100;
		AmmoPerClip = 20;
		InitialClips = 4;
		BaseDamage = 5;
		DamageType = UDamageType::StaticClass();
		TimeBetweenShots = 0.2f;
		FiringMode = EFiringMode::Automatic;
		BurstNumberOfShots = 3;
		BurstTimeBetweenShots = 0.2f;
	}
};

UCLASS(Abstract)
class ABaseWeapon : public AInteractive
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnWeaponStateChangedDelegate Delegate_OnWeaponStateChanged;

	UPROPERTY(BlueprintAssignable)
	FOnWeaponFiredWithRecoil Delegate_OnWeaponFiredWithRecoil;
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aim")
	bool UseControllerAim = true;

	/** pawn owner */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_MyPawn)
	class AVertCharacter* MyPawn;

	/** weapon data */
	UPROPERTY(EditDefaultsOnly, Category = Config)
	FWeaponData WeaponConfig;
	
	/** camera shake on firing */
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	TSubclassOf<UCameraShake> FireCameraShake;

	/** force feedback effect to play when the weapon is fired */
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	UForceFeedbackEffect *FireForceFeedback;

	/** single fire sound (bLoopedFireSound not set) */
	UPROPERTY(EditDefaultsOnly, Category = Sound)
	USoundCue* FireSound;
	
	/** out of ammo sound */
	UPROPERTY(EditDefaultsOnly, Category = Sound)
	USoundCue* OutOfAmmoSound;

	/** reload sound */
	UPROPERTY(EditDefaultsOnly, Category = Sound)
	USoundCue* ReloadSound;

	/** reload animations */
	UPROPERTY(EditDefaultsOnly, Category = Animation)
	FWeaponAnim ReloadAnim;

	/** equip sound */
	UPROPERTY(EditDefaultsOnly, Category = Sound)
	USoundCue* EquipSound;

	/** equip animations */
	UPROPERTY(EditDefaultsOnly, Category = Animation)
	FWeaponAnim EquipAnim;

	/** fire animations */
	UPROPERTY(EditDefaultsOnly, Category = Animation)
	FWeaponAnim FireAnim;

	UPROPERTY(EditDefaultsOnly, Category = Animation)
	FWeaponAnim IdleAnim;
	
	/* Mainly for testing purposes, ignores the wait for an animation to notify after euip begin or reload begin */
	UPROPERTY(EditDefaultsOnly, Category = "Animation|Overrides")
	uint32 OverrideAnimCompleteNotify : 1;

	/* Whether we should block all user input for this weapon whilst an attack animation is playing */
	UPROPERTY(EditDefaultsOnly, Category = "Animation|Overrides")
	uint32 UseAnimsForAttackStartAndEnd : 1;

	uint32 WaitingForAttackEnd : 1;

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

	/** current total ammo */
	UPROPERTY(Transient, Replicated)
	int32 CurrentAmmo;

	/** current ammo - inside clip */
	UPROPERTY(Transient, Replicated)
	int32 CurrentAmmoInClip;

	/** burst counter, used for replicating fire events to remote clients */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_BurstCounter)
	int32 BurstCounter;

	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
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
	EWeaponState GetCurrentState() const; /** get current weapon state */
	int32 GetCurrentAmmo() const; /** get current ammo amount (total) */
	int32 GetCurrentAmmoInClip() const; /** get current ammo amount (clip) */
	int32 GetAmmoPerClip() const; /** get clip size */
	int32 GetMaxAmmo() const; /** get max ammo amount */

	void Pickup(AVertCharacter* NewOwner); /** [server] weapon was added to pawn's inventory */
	virtual void StartEquipping();
	virtual void OnDrop(); /** [server] weapon was removed from pawn's inventory */
	virtual void Interact(const TWeakObjectPtr<class UCharacterInteractionComponent>& instigator) final;
	virtual void PostInitializeComponents() override;
	virtual void Destroyed() override;
	virtual void StartAttacking(); /** [local + server] start weapon fire */
	virtual void StopAttacking(); /** [local + server] stop weapon fire */
	virtual void StartReload(bool bFromReplication = false); /** [all] start weapon reload */
	virtual void StopReload(); /** [local + server] interrupt weapon reload */
	virtual void ReloadWeapon(); /** [server] performs actual reload */

	/** trigger reload from server */
	UFUNCTION(reliable, client)
	void ClientStartReload();

	/** get pawn owner */
	UFUNCTION(BlueprintCallable, Category = "Game|Weapon")
	class AVertCharacter* GetPawnOwner() const;

	UFUNCTION(BlueprintCallable, Category = "WeaponStats")
	FORCEINLINE int32 GetBaseDamage() const { return WeaponConfig.BaseDamage; }
	
	UFUNCTION(BlueprintCallable, Category = "AnimationNotifies")
	FORCEINLINE bool IsAttackAnimationFinished() const { return !WaitingForAttackEnd; }

	/* Broadcasts the OnWeaponFiredWithRecoil delegate with a custom recoil amount; to be used in cases where more custom recoil behaviour is required. */
	UFUNCTION(BlueprintCallable, Category = "Delegates")
	void BroadcastOnWeaponFiredWithRecoil(float recoilAmount) { Delegate_OnWeaponFiredWithRecoil.Broadcast(recoilAmount); }

	/* [LOCAL} animation notify to tell us it's alright to leave the Equipping state */
	UFUNCTION(BlueprintCallable, Category = "AnimationNotifies")
	void NotifyEquipAnimationEnded();

	/* [LOCAL} animation notify to tell us it's alright to leave the Reloading state */
	UFUNCTION(BlueprintCallable, Category = "AnimationNotifies")
	void NotifyReloadAnimationEnded();

	/* [LOCAL} animation notify to tell us it's alright to leave the Attacking state */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "AnimationNotifies")
	void NotifyAttackAnimationActiveStarted();

	/* [LOCAL} animation notify to tell us it's alright to leave the Attacking state */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "AnimationNotifies")
	void NotifyAttackAnimationActiveEnded();

	/* [LOCAL} animation notify to tell us it's alright to leave the Attacking state */
	UFUNCTION(BlueprintCallable, Category = "AnimationNotifies")
	void NotifyAttackAnimationEnded();

protected:
	UAnimSequence* GetPlayerAnimForState(EWeaponState state);
	void SimulateWeaponAttack(); /** Called in network play to do the cosmetic fx for firing */
	void StopSimulatingWeaponAttack(); /** Called in network play to stop cosmetic fx (e.g. for a looping shot). */
	void HandleAttacking(); /** [local + server] handle weapon fire */
	void SetWeaponState(EWeaponState NewState); /** update weapon state */
	void DetermineWeaponState(); /** determine current weapon state */
	void AttachMeshToPawn(); /** attaches weapon mesh to pawn's mesh */
	void DetachMeshFromPawn(); /** detaches weapon mesh from pawn */
	UAudioComponent* PlayWeaponSound(USoundCue* Sound); /** play weapon sounds */
	void PlayWeaponAnimation(const FWeaponAnim& Animation); /** play weapon animations */
	void StopWeaponAnimation(const FWeaponAnim& Animation); /** stop playing weapon animations */
	bool WeaponNotAutomatic() const;

	virtual void OnBurstStarted(); /** [local + server] firing started */
	virtual void OnBurstFinished(); /** [local + server] firing finished */
	
	UFUNCTION(BlueprintNativeEvent, Category = "FX")
	void ClientSimulateWeaponAttack();

	UFUNCTION(BlueprintNativeEvent, Category = "FX")
	void ClientStopSimulateWeaponAttack();

	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	void WeaponInteract(UCharacterInteractionComponent* interactionComponent, AVertCharacter* character);
	
	/** [local] weapon specific fire implementation */
	UFUNCTION(BlueprintNativeEvent, Category = "Attack")
	bool AttackWithWeapon();

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
	void OnStopAttacking();

	/** find hit */
	UFUNCTION(BlueprintCallable, Category = "Hit Detection")
	FHitResult WeaponTrace(const FVector& TraceFrom, const FVector& TraceTo) const;

	UFUNCTION(reliable, server, WithValidation)
	void ServerStartAttacking();

	UFUNCTION(reliable, server, WithValidation)
	void ServerStopAttacking();

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
	
protected:
	bool mAttackSpent = false;
	float mLastFireTime; /** time of last successful weapon fire */
	float mEquipStartedTime; /** last time when this weapon was switched to */
	float mEquipDuration; /** how much time weapon needs to be equipped */
	EWeaponState mCurrentState; /** current weapon state */
	FTimerHandle mTimerHandle_OnEquipFinished;
	FTimerHandle mTimerHandle_StopReload;
	FTimerHandle mTimerHandle_ReloadWeapon;
	FTimerHandle mTimerHandle_HandleFiring;
	FTimerHandle mTimerHandle_NonAutoTriggerDelay;
};