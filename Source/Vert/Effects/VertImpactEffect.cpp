// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "VertImpactEffect.h"
#include "Vert.h"

AVertImpactEffect::AVertImpactEffect(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bAutoDestroyWhenFinished = true;
}

void AVertImpactEffect::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	UPhysicalMaterial* HitPhysMat = SurfaceHit.PhysMaterial.Get();
	EPhysicalSurface HitSurfaceType = UPhysicalMaterial::DetermineSurfaceType(HitPhysMat);

	// show particles
	UParticleSystem* ImpactFX = GetImpactFX(HitSurfaceType);
	if (ImpactFX)
	{
		UGameplayStatics::SpawnEmitterAtLocation(this, ImpactFX, GetActorLocation(), GetActorRotation());
	}

	// play sound
	USoundCue* ImpactSound = GetImpactSound(HitSurfaceType);
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}

	if (DefaultDecal.DecalMaterial)
	{
		FRotator RandomDecalRotation = SurfaceHit.ImpactNormal.Rotation();
		RandomDecalRotation.Roll = FMath::FRandRange(-180.0f, 180.0f);

		UGameplayStatics::SpawnDecalAttached(DefaultDecal.DecalMaterial, FVector(1.0f, DefaultDecal.DecalSize, DefaultDecal.DecalSize),
			SurfaceHit.Component.Get(), SurfaceHit.BoneName,
			SurfaceHit.ImpactPoint, RandomDecalRotation, EAttachLocation::KeepWorldPosition,
			DefaultDecal.LifeSpan);
	}
}

UParticleSystem* AVertImpactEffect::GetImpactFX(TEnumAsByte<EPhysicalSurface> SurfaceType) const
{
	UParticleSystem* ImpactFX = NULL;

	switch (SurfaceType)
	{
	case VERT_SURFACE_Concrete:		ImpactFX = ConcreteFX; break;
	case VERT_SURFACE_Dirt:			ImpactFX = DirtFX; break;
	case VERT_SURFACE_Water:		ImpactFX = WaterFX; break;
	case VERT_SURFACE_Metal:		ImpactFX = MetalFX; break;
	case VERT_SURFACE_Wood:			ImpactFX = WoodFX; break;
	case VERT_SURFACE_Grass:		ImpactFX = GrassFX; break;
	case VERT_SURFACE_Glass:		ImpactFX = GlassFX; break;
	case VERT_SURFACE_Flesh:		ImpactFX = FleshFX; break;
	default:						ImpactFX = DefaultFX; break;
	}

	return ImpactFX;
}

USoundCue* AVertImpactEffect::GetImpactSound(TEnumAsByte<EPhysicalSurface> SurfaceType) const
{
	USoundCue* ImpactSound = NULL;

	switch (SurfaceType)
	{
	case VERT_SURFACE_Concrete:		ImpactSound = ConcreteSound; break;
	case VERT_SURFACE_Dirt:			ImpactSound = DirtSound; break;
	case VERT_SURFACE_Water:		ImpactSound = WaterSound; break;
	case VERT_SURFACE_Metal:		ImpactSound = MetalSound; break;
	case VERT_SURFACE_Wood:			ImpactSound = WoodSound; break;
	case VERT_SURFACE_Grass:		ImpactSound = GrassSound; break;
	case VERT_SURFACE_Glass:		ImpactSound = GlassSound; break;
	case VERT_SURFACE_Flesh:		ImpactSound = FleshSound; break;
	default:						ImpactSound = DefaultSound; break;
	}

	return ImpactSound;
}
