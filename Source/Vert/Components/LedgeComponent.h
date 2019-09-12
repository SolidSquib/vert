// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "LedgeComponent.generated.h"

/**
 * 
 */
UCLASS(ClassGroup = (LedgesAndGrappling), BlueprintType, meta = (BlueprintSpawnableComponent))
class VERT_API ULedgeComponent : public UBoxComponent
{
	GENERATED_BODY()
	
public:
	ULedgeComponent();

public:
	void GrabbedBy(class ULedgeGrabbingComponent* ledgeGrabber);
	void DroppedBy(ULedgeGrabbingComponent* ledgeGrabber);

private:
	TWeakObjectPtr<ULedgeGrabbingComponent> mCharacterHolding = nullptr;
};
