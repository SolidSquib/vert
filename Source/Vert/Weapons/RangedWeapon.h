// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "GameFramework/Actor.h"
#include "Interactives/Interactive.h"
#include "RangedWeapon.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogRangedWeapon, Log, All);

namespace EWeaponState
{
	enum Type
	{
		Idle,
		Firing,
		Reloading,
		Equipping,
	};
}

USTRUCT()
struct FWeaponData
{
	GENERATED_BODY()

	/** inifite ammo for reloads */
	UPROPERTY(EditDefaultsOnly, Category = Ammo)
	bool bInfiniteAmmo;

	/** infinite ammo in clip, no reload required */
	UPROPERTY(EditDefaultsOnly, Category = Ammo)
	bool bInfiniteClip;

	/** max ammo */
	UPROPERTY(EditDefaultsOnly, Category = Ammo)
	int32 MaxAmmo;

	/** clip size */
	UPROPERTY(EditDefaultsOnly, Category = Ammo)
	int32 AmmoPerClip;

	/** initial clips */
	UPROPERTY(EditDefaultsOnly, Category = Ammo)
	int32 InitialClips;

	/** time between two consecutive shots */
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
	float TimeBetweenShots;

	/** failsafe reload duration if weapon doesn't have any animation for it */
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
	float NoAnimReloadDuration;

	/** defaults */
	FWeaponData()
	{
		bInfiniteAmmo = false;
		bInfiniteClip = false;
		MaxAmmo = 100;
		AmmoPerClip = 20;
		InitialClips = 4;
		TimeBetweenShots = 0.2f;
		NoAnimReloadDuration = 1.0f;
	}
};

USTRUCT()
struct FRangedWeaponAnim
{
	GENERATED_BODY()

	/** animation played on pawn (1st person view) */
	UPROPERTY(EditDefaultsOnly, Category = Animation)
	UPaperFlipbook* ReloadAnimation;

	UPROPERTY(EditDefaultsOnly, Category = Animation)
	UPaperFlipbook* EquipAnimation;
	// Add more if needed
};

UCLASS(Abstract, Blueprintable)
class ARangedWeapon : public ABaseWeapon
{
	GENERATED_UCLASS_BODY()

	enum class EAmmoType
	{
		EBullet,
		ERocket,
		EMax,
	};

	UPROPERTY(Transient, ReplicatedUsing = OnRep_MyPawn)
	class AVertCharacter* MyPawn;

	UPROPERTY(EditDefaultsOnly, Category = Config)
	FWeaponData WeaponConfig;

	/** firing audio (bLoopedFireSound set) */
	UPROPERTY(Transient)
	UAudioComponent* FireAC;

	UPROPERTY(EditDefaultsOnly, Category = Effects)
	FName MuzzleAttachPoint;

	UPROPERTY(EditDefaultsOnly, Category = Effects)
	UParticleSystem* MuzzleFX;

	UPROPERTY(Transient)
	UParticleSystemComponent* MuzzlePSC;

	UPROPERTY(Transient)
	UParticleSystemComponent* MuzzlePSCSecondary;

	UPROPERTY(EditDefaultsOnly, Category = Effects)
	UForceFeedbackEffect *FireForceFeedback;

	/** single fire sound (bLoopedFireSound not set) */
	UPROPERTY(EditDefaultsOnly, Category = Sound)
	USoundCue* FireSound;

	/** looped fire sound (bLoopedFireSound set) */
	UPROPERTY(EditDefaultsOnly, Category = Sound)
	USoundCue* FireLoopSound;

	/** finished burst sound (bLoopedFireSound set) */
	UPROPERTY(EditDefaultsOnly, Category = Sound)
	USoundCue* FireFinishSound;

	UPROPERTY(EditDefaultsOnly, Category = Sound)
	USoundCue* OutOfAmmoSound;

	UPROPERTY(EditDefaultsOnly, Category = Sound)
	USoundCue* ReloadSound;

	UPROPERTY(EditDefaultsOnly, Category = Animation)
	FRangedWeaponAnim ReloadAnim;

	UPROPERTY(EditDefaultsOnly, Category = Sound)
	USoundCue* EquipSound;

	UPROPERTY(EditDefaultsOnly, Category = Animation)
	FRangedWeaponAnim EquipAnim;

	UPROPERTY(EditDefaultsOnly, Category = Animation)
	FRangedWeaponAnim FireAnim;

	UPROPERTY(EditDefaultsOnly, Category = Effects)
	uint32 bLoopedMuzzleFX : 1;

	UPROPERTY(EditDefaultsOnly, Category = Sound)
	uint32 bLoopedFireSound : 1;

	UPROPERTY(EditDefaultsOnly, Category = Animation)
	uint32 bLoopedFireAnim : 1;

	uint32 bPlayingFireAnim : 1;

	uint32 bIsEquipped : 1;

	uint32 bWantsToFire : 1;

	UPROPERTY(Transient, ReplicatedUsing = OnRep_Reload)
	uint32 bPendingReload : 1;

	uint32 bPendingEquip : 1;

	UPROPERTY(Transient, Replicated) /** current total ammo */
	int32 CurrentAmmo;

	UPROPERTY(Transient, Replicated) /** current ammo - inside clip */
	int32 CurrentAmmoInClip;
	
	UPROPERTY(Transient, ReplicatedUsing = OnRep_BurstCounter) /** burst counter, used for replicating fire events to remote clients */
	int32 BurstCounter;

protected:
	virtual EAmmoType GetAmmoType() const
	{
		return EAmmoType::EBullet;
	}

	void GiveAmmo(int AddAmount); // [server]
	void UseAmmo();
	bool IsEquipped() const;
	bool IsAttachedToPawn() const;
	bool CanFire() const;
	bool CanReload() const;
	EWeaponState::Type GetCurrentState() const;
	int32 GetCurrentAmmo() const;
	int32 GetCurrentAmmoInClip() const;
	int32 GetAmmoPerClip() const;
	int32 GetMaxAmmo() const;
	bool HasInfiniteAmmo() const;
	bool HasInfiniteClip() const;
	void SetOwningPawn(AVertCharacter* AVertCharacter);
	float GetEquipStartedTime() const;
	float GetEquipDuration() const;
	void SetWeaponState(EWeaponState::Type NewState); /** update weapon state */
	void DetermineWeaponState(); /** determine current weapon state */
	void HandleFiring(); /** [local + server] handle weapon fire */
	UAudioComponent* PlayWeaponSound(USoundCue* Sound); /** play weapon sounds */
	float PlayWeaponAnimation(const FRangedWeaponAnim& Animation); /** play weapon animations */
	void StopWeaponAnimation(const FRangedWeaponAnim& Animation); /** stop playing weapon animations */
	FVector GetCameraAim() const; /** Get the aim of the camera */
	FVector GetMuzzleLocation() const; /** get the muzzle location of the weapon */
	FVector GetMuzzleDirection() const; /** get direction of weapon's muzzle */
	FHitResult WeaponTrace(const FVector& TraceFrom, const FVector& TraceTo) const; /** find hit */

	virtual void PostInitializeComponents() override;
	virtual void Destroyed() override;
	virtual void OnEquip(const ARangedWeapon* LastWeapon);
	virtual void OnEquipFinished();
	virtual void OnUnEquip();
	virtual void OnEnterInventory(AVertCharacter* NewOwner); // [server]
	virtual void OnLeaveInventory(); // [server]
	virtual void StartFire(); // [local + server]
	virtual void StopFire(); // [local + server]
	virtual void StartReload(bool bFromReplication = false); // [all]
	virtual void StopReload(); // [local + server]
	virtual void ReloadWeapon(); // [server]
	virtual void SimulateWeaponFire(); /** Called in network play to do the cosmetic fx for firing */
	virtual void StopSimulatingWeaponFire(); /** Called in network play to stop cosmetic fx (e.g. for a looping shot). */
	virtual void FireWeapon() PURE_VIRTUAL(ARangedWeapon::FireWeapon, ); /** [local] weapon specific fire implementation */
	virtual void OnBurstStarted(); /** [local + server] firing started */
	virtual void OnBurstFinished(); /** [local + server] firing finished */
	virtual FVector GetAdjustedAim() const; /** Get the aim of the weapon, allowing for adjustments to be made by the weapon */
	virtual void ExecuteAttack_Implementation();

	UFUNCTION(reliable, client)
	void ClientStartReload();
	
	UFUNCTION(BlueprintCallable, Category = "Game|Weapon")
	class AVertCharacter* GetPawnOwner() const;

	//////////////////////////////////////////////////////////////////////////
	// Input - server side

	UFUNCTION(reliable, server, WithValidation)
	void ServerStartFire();

	UFUNCTION(reliable, server, WithValidation)
	void ServerStopFire();

	UFUNCTION(reliable, server, WithValidation)
	void ServerStartReload();

	UFUNCTION(reliable, server, WithValidation)
	void ServerStopReload();

	//////////////////////////////////////////////////////////////////////////
	// Replication & effects

	UFUNCTION()
	void OnRep_MyPawn();

	UFUNCTION()
	void OnRep_BurstCounter();

	UFUNCTION()
	void OnRep_Reload();

	UFUNCTION(reliable, server, WithValidation) /** [server] fire & update ammo */
	void ServerHandleFiring();

protected:
	uint32 bRefiring; /** weapon is refiring */	
	EWeaponState::Type CurrentState; /** current weapon state */	
	float LastFireTime; /** time of last successful weapon fire */	
	float EquipStartedTime; /** last time when this weapon was switched to */	
	float EquipDuration; /** how much time weapon needs to be equipped */	
	FTimerHandle TimerHandle_OnEquipFinished; /** Handle for efficient management of OnEquipFinished timer */	
	FTimerHandle TimerHandle_StopReload; /** Handle for efficient management of StopReload timer */	
	FTimerHandle TimerHandle_ReloadWeapon; /** Handle for efficient management of ReloadWeapon timer */	
	FTimerHandle TimerHandle_HandleFiring; /** Handle for efficient management of HandleFiring timer */
};

