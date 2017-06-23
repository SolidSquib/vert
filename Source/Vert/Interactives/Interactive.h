// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "Interactive.generated.h"

UCLASS(ABSTRACT)
class VERT_API AInteractive : public AActor
{
	GENERATED_UCLASS_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Interaction")
	class USphereComponent* InteractionSphere;
	
public:
	virtual void Interact(const TWeakObjectPtr<class UCharacterInteractionComponent>& instigator) PURE_VIRTUAL(AInteractive::Interact, );

protected:
	UFUNCTION(BlueprintCallable)
	void DisableInteractionDetection();

	UFUNCTION(BlueprintCallable)
	void EnableInteractionDetection();
};
