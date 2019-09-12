// Copyright Inside Out Games Ltd. 2017

#include "CatchAIController.h"
#include "AI/CatchPathFollowingComponent.h"

ACatchAIController::ACatchAIController(const FObjectInitializer& PCIP)
: Super(PCIP.SetDefaultSubobjectClass<UCatchPathFollowingComponent>(TEXT("PathFollowingComponent")))
{

}