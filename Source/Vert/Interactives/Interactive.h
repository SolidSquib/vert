// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "PooledActor.h"
#include "Interactive.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInteract);

UCLASS(ABSTRACT)
class VERT_API AInteractive : public APooledActor
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnInteract OnInteract;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool EverEnableInteraction = true;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Interaction")
	class USphereComponent* InteractionSphere;
	
public:
	virtual void Interact(const TWeakObjectPtr<class UCharacterInteractionComponent>& instigator);

	UFUNCTION(BlueprintCallable)
	void DisableInteractionDetection();

	UFUNCTION(BlueprintCallable)
	void EnableInteractionDetection();

protected:
	virtual void PostInitializeComponents() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void EVENT_OnInteract(UCharacterInteractionComponent* interactingComponent);
};
