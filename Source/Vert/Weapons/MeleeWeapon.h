// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "GameFramework/Actor.h"
#include "WeaponPickup.h"
#include "Interactives/Interactive.h"
#include "MeleeWeapon.generated.h"

UCLASS()
class VERT_API AMeleeWeapon : public AActor, public IWeaponPickup, public IInteractive
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|Sprite")
	class UPaperFlipbook* DefaultAnimation;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Sprite", meta = (AllowPrivateAccess = "true"))
	class UPaperFlipbookComponent* Sprite;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Collision", meta = (AllowPrivateAccess = "true"))
	class UBoxComponent* CollisionComponent;

public:	
	// Sets default values for this actor's properties
	AMeleeWeapon();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual void Attack() override;
	virtual void Interact(class APawn* instigator) override;
};
