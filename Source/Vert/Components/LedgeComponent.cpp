// Copyright Inside Out Games Ltd. 2017

#include "LedgeComponent.h"
#include "Character/LedgeGrabbingComponent.h"

ULedgeComponent::ULedgeComponent()
{
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_SphereTracer, ECR_Overlap);
	SetCollisionResponseToChannel(ECC_LedgeTracer, ECR_Block);
	BoxExtent.X = 50.f;
	BoxExtent.Y = 85.f;
	BoxExtent.Z = 50.f;
}

//************************************
// Method:    GrabbedBy
// FullName:  ULedgeComponent::GrabbedBy
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: ULedgeGrabbingComponent * ledgeGrabber
//************************************
void ULedgeComponent::GrabbedBy(ULedgeGrabbingComponent* ledgeGrabber)
{
	if (mCharacterHolding.IsValid() && ledgeGrabber != nullptr)
	{
		mCharacterHolding->TransitionLedge(ELedgeTransition::DropFromGrabbedLedge);
	}

	mCharacterHolding = ledgeGrabber;
}

//************************************
// Method:    DroppedBy
// FullName:  ULedgeComponent::DroppedBy
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: ULedgeGrabbingComponent * ledgeGrabber
//************************************
void ULedgeComponent::DroppedBy(ULedgeGrabbingComponent* ledgeGrabber)
{
	if (mCharacterHolding.IsValid() && ledgeGrabber == mCharacterHolding.Get())
	{
		mCharacterHolding = nullptr;
	}
}