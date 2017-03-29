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
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Origin", meta = (AllowPrivateAccess = "true"))
	class USceneComponent* AttachPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Sprite", meta = (AllowPrivateAccess = "true"))
	class UPaperFlipbookComponent* Sprite;

public:	
	// Sets default values for this actor's properties
	AMeleeWeapon();

	// Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual void Attack() override;
	virtual void Interact(TWeakObjectPtr<class UCharacterInteractionComponent> instigator) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	void Throw();
};
