// Copyright Inside Out Games Ltd. 2017

#include "Interactive.h"

AInteractive::AInteractive(const class FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	InteractionSphere = CreateOptionalDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetCollisionObjectType(ECC_Interactive);
	InteractionSphere->SetCollisionProfileName(TEXT("InteractiveItem"));
	InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionSphere->InitSphereRadius(20.f);
	InteractionSphere->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
}

void AInteractive::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (!EverEnableInteraction)
	{
		DisableInteractionDetection();
	}
}

void AInteractive::DisableInteractionDetection()
{
	InteractionSphere->SetCollisionResponseToChannel(ECC_InteractionTrace, ECR_Ignore);
}

void AInteractive::EnableInteractionDetection()
{
	if (!EverEnableInteraction)
		return;

	InteractionSphere->SetCollisionResponseToChannel(ECC_InteractionTrace, ECR_Block);
}

void AInteractive::Interact(const TWeakObjectPtr<class UCharacterInteractionComponent>& instigator)
{
	EVENT_OnInteract(instigator.Get());
	OnInteract.Broadcast();
}