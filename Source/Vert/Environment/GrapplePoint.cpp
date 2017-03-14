// Fill out your copyright notice in the Description page of Project Settings.

#include "Vert.h"
#include "GrapplePoint.h"


// Sets default values
AGrapplePoint::AGrapplePoint()
{
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->BodyInstance.SetCollisionProfileName("GrapplePoint");
	CollisionComponent->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
	CollisionComponent->CanCharacterStepUpOn = ECB_No;

	SetRootComponent(CollisionComponent);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GrapplePointMesh"));
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetupAttachment(RootComponent);

	SnapSocket = NAME_None;
}

void AGrapplePoint::AttachGrappleHook(AGrappleHook* hook)
{
	hook->AttachToComponent(MeshComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SnapSocket);

	FScriptDelegate onUnLatchedDelegate;
	onUnLatchedDelegate.BindUFunction(this, TEXT("OnUnLatched"));
	hook->OnUnLatched.Add(onUnLatchedDelegate);

	mHookedGrapples.Add(hook, onUnLatchedDelegate);
}

void AGrapplePoint::OnUnLatched_Implementation(AGrappleHook* hook)
{
	check(mHookedGrapples.Find(hook));

	if (mHookedGrapples.Find(hook))
	{
		hook->OnUnLatched.Remove(mHookedGrapples[hook]);
		mHookedGrapples.Remove(hook);
	}
}

