// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "GameFramework/Actor.h"
#include "Interactives/Interactive.h"
#include "BaseWeapon.generated.h"

UENUM(BlueprintType)
enum class EWeaponFiremode : uint8
{
	Automatic,
	Burst UMETA(DisplayName="Burst fire"),
	SemiAutomatic UMETA(DisplayName="Semi-Automatic")
};

UCLASS(BlueprintType, Abstract)
class VERT_API ABaseWeapon : public AActor, public IInteractive
{
	GENERATED_UCLASS_BODY()
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Damage")
	float BaseDamage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Damage")
	float KnockbackMagnitude = 1000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|Damage")
	TSubclassOf<UDamageType> DamageType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Collision")
	class USphereComponent* InteractiveCollisionComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|Sprite")
	class UPaperFlipbook* DefaultAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|Sprite")
	class UPaperFlipbook* AttackAnimation;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Origin", meta = (AllowPrivateAccess = "true"))
	class USceneComponent* AttachPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Sprite", meta = (AllowPrivateAccess = "true"))
	class UPaperFlipbookComponent* Sprite;

	/* #MI_TODO: Replace sprites with meshes!
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Mesh", meta = (AllowPrivateAccess = "true"))
	class USkeletalMeshComponent* Mesh; */

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Usage")
	EWeaponFiremode FiringMode = EWeaponFiremode::SemiAutomatic;

	/* The time delay between attacks/shots (not strictly rate of fire but simpler to track and visualise) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Usage")
	float RateOfFire = 0.5f;

	/* How many shots before this weapon breaks (0 means infinite usage) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Usage")
	int32 Ammunition = 0;

	/* How long between pressing the attack button and executing the attack */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Usage")
	float WindUpTime = 0;

public:
	virtual void NotifyAttackCommand();
	virtual void NotifyStopAttacking();
	virtual void Interact(TWeakObjectPtr<class UCharacterInteractionComponent> instigator) final;

protected:
	void NativeOnThrow();
	bool ShouldDealDamage(AActor* TestActor) const;
	void DealDamage(const FHitResult& Impact, const FVector& ShootDir);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon|Throw")
	void OnCatch();

	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon|Throw")
	void OnThrow();

	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon|Throw")
	void OnImpact();

	UFUNCTION(BlueprintNativeEvent, Category = "Weapon|Attack")
	void ExecuteAttack();
	//virtual void ExecuteAttack_Implementation();

	UFUNCTION(BlueprintNativeEvent, Category = "Weapon|Collision")
	void OnHit(class UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	//virtual void OnHit_Implementation(class UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION(BlueprintNativeEvent, Category = "Weapon|Collision")
	void OnBeginOverlap(class UPrimitiveComponent* overlappedComp, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool fromSweep, const FHitResult& sweepResult);
	//virtual void OnBeginOverlap_Implementation(class UPrimitiveComponent* overlappedComp, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool fromSweep, const FHitResult& sweepResult);

	UFUNCTION(BlueprintNativeEvent)
	void AttackAnimationEnd();
	//virtual void AttackAnimationEnd_Implementation();

private:
	void DisableInteractionDetection();
	void EnableInteractionDetection();

public:
	static const FName scCollisionProfileName;
	static const FName scAttackingCollisionProfileName;

protected:
	bool mIsAttacking = false;
	int32 mChargeLevel = 0;
	TWeakObjectPtr<class UCharacterInteractionComponent> mCharacterInteractionOwner = nullptr;

private:
	FTimerHandle mAttackTimer;
	FTimerHandle mDelayTimer;
};
