// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "GameFramework/Actor.h"
#include "Interactives/Interactive.h"
#include "WeaponPickup.h"
#include "BaseWeapon.generated.h"

UCLASS()
class VERT_API ABaseWeapon : public AActor, public IWeaponPickup, public IInteractive
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Damage")
	int32 BaseDamage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|Sprite")
	class UPaperFlipbook* DefaultAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|Sprite")
	class UPaperFlipbook* AttackAnimation;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Origin", meta = (AllowPrivateAccess = "true"))
	class USceneComponent* AttachPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Sprite", meta = (AllowPrivateAccess = "true"))
	class UPaperFlipbookComponent* Sprite;

public:	
	// Sets default values for this actor's properties
	ABaseWeapon();

	virtual void Tick(float DeltaTime) override;
	virtual void Attack() override;
	virtual void Interact(TWeakObjectPtr<class UCharacterInteractionComponent> instigator) override;

protected:
	virtual void BeginPlay() override;
	virtual void Throw();

	UFUNCTION(BlueprintNativeEvent, Category = "Weapon|Collision")
	void OnHit(class UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION(BlueprintNativeEvent, Category = "Weapon|Collision")
	void OnBeginOverlap(class UPrimitiveComponent* overlappedComp, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool fromSweep, const FHitResult& sweepResult);
};
