// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "GameFramework/Actor.h"
#include "Interactives/Interactive.h"
#include "WeaponPickup.h"
#include "BaseWeapon.generated.h"

UENUM(BlueprintType)
enum class EWeaponFiremode : uint8
{
	Automatic,
	Burst UMETA(DisplayName="Burst fire"),
	SemiAutomatic UMETA(DisplayName="Semi-Automatic")
};

UCLASS()
class VERT_API ABaseWeapon : public AActor, public IWeaponPickup, public IInteractive
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Damage")
	int32 BaseDamage = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Damage")
	float KnockbackMagnitude = 1000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|Sprite")
	class UPaperFlipbook* DefaultAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|Sprite")
	class UPaperFlipbook* AttackAnimation;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Origin", meta = (AllowPrivateAccess = "true"))
	class USceneComponent* AttachPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Sprite", meta = (AllowPrivateAccess = "true"))
	class UPaperFlipbookComponent* Sprite;

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
	// Sets default values for this actor's properties
	ABaseWeapon();

	virtual void Attack() final;
	virtual void StopAttacking() final;
	virtual void Interact(TWeakObjectPtr<class UCharacterInteractionComponent> instigator) final;

	UFUNCTION()
	void OnHit(class UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void OnBeginOverlap(class UPrimitiveComponent* overlappedComp, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool fromSweep, const FHitResult& sweepResult);

protected:
	void NativeOnThrow();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

	UFUNCTION(BlueprintNativeEvent, Category = "Weapon|Attack")
	void ExecuteAttack();

	UFUNCTION(BlueprintNativeEvent, Category = "Weapon|Collision")
	void NativeOnHit(class UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION(BlueprintNativeEvent, Category = "Weapon|Collision")
	void NativeOnBeginOverlap(class UPrimitiveComponent* overlappedComp, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool fromSweep, const FHitResult& sweepResult);

	UFUNCTION(BlueprintNativeEvent)
	void AttackAnimationEnd();

private:
	void DisableInteractionDetection();
	void EnableInteractionDetection();

public:
	static const FName scCollisionProfileName;
	static const FName scAttackingCollisionProfileName;

private:
	float mTimeOfLastAttack = 0.f;
	FTimerHandle mAttackTimer;
	FTimerHandle mDelayTimer;
};
