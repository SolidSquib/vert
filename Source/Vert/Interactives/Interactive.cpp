// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "Interactive.h"

AInteractive::AInteractive(const class FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	InteractionSphere = CreateOptionalDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetCollisionObjectType(ECC_Interactive);
	InteractionSphere->SetCollisionProfileName(TEXT("InteractiveItem"));
	InteractionSphere->InitSphereRadius(20.f);
	InteractionSphere->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
}

void AInteractive::DisableInteractionDetection()
{
	InteractionSphere->SetCollisionResponseToChannel(ECC_InteractionTrace, ECR_Ignore);
}

void AInteractive::EnableInteractionDetection()
{
	InteractionSphere->SetCollisionResponseToChannel(ECC_InteractionTrace, ECR_Block);
}