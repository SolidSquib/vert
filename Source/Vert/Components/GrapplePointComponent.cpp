// Copyright Inside Out Games Ltd. 2017

#include "GrapplePointComponent.h"

UGrapplePointComponent::UGrapplePointComponent()
{
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_SphereTracer, ECR_Overlap);
	BoxExtent.X = 30.f;
	BoxExtent.Y = 30.f;
	BoxExtent.Z = 30.f;
}


