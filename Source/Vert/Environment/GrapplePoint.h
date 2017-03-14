// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "GrapplePoint.generated.h"

UCLASS()
class VERT_API AGrapplePoint : public AActor
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Attachments")
	FName SnapSocket;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	class UStaticMeshComponent* MeshComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Collision")
	class USphereComponent* CollisionComponent;

public:	
	// Sets default values for this actor's properties
	AGrapplePoint();

	void AttachGrappleHook(class AGrappleHook* hook);

	UFUNCTION(BlueprintNativeEvent, Category = "Grappling")
	void OnUnLatched(class AGrappleHook* hook);
	
protected:
	TMap<class AGrappleHook*, FScriptDelegate> mHookedGrapples;
};
