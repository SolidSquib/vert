// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "GrapplePointComponent.generated.h"

/**
 * 
 */
UCLASS(ClassGroup = (LedgesAndGrappling), BlueprintType, meta = (BlueprintSpawnableComponent))
class VERT_API UGrapplePointComponent : public UBoxComponent
{
	GENERATED_BODY()
	
public:
	UGrapplePointComponent();	
};