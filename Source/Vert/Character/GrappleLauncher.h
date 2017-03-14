// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "Character/GrappleHook.h"
#include "GrappleLauncher.generated.h"

USTRUCT()
struct FHookshotGrappleConfig
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Rope")
	float MaxLength;

	UPROPERTY(EditDefaultsOnly, Category = "Speed")
	float LineSpeed;

	UPROPERTY(EditDefaultsOnly, Category = "Speed")
	float ReturnSpeed;

	UPROPERTY(EditDefaultsOnly, Category = "Speed")
	float ReelSpeed;

	UPROPERTY(EditDefaultsOnly, Category = "Collision")
	bool HookGrapplePointsOnly;

	UPROPERTY(EditDefaultsOnly, Category = "Collision", Meta = (EditCondition = "HookGrapplePointsOnly"))
	FName GrapplePointCollisionProfileName;

	FHookshotGrappleConfig()
	{
		MaxLength = 500.f;
		LineSpeed = 3000.f;
		ReturnSpeed = 35.f;
		ReelSpeed = 3500;
		HookGrapplePointsOnly = false;
		GrapplePointCollisionProfileName = "GrapplePoint";
	}
};

UCLASS()
class VERT_API AGrappleLauncher : public AActor
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Debug, meta = (AllowPrivateAccess = "true"))
	bool ShowDebug;

public:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Grappling")
	class UCableComponent* Cable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grappling")
	FHookshotGrappleConfig GrappleConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grappling")
	TSubclassOf<AGrappleHook> HookClass;

public:
	// Sets default values for this component's properties
	AGrappleLauncher();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void FireGrapple(const FVector& shootDirection);
	void ResetGrapple();

	class AVertCharacter* GetOwningCharacter() const;

	FORCEINLINE AGrappleHook* GetGrappleHook() const { return mGrappleHook.Get(); }

	UFUNCTION(BlueprintNativeEvent, Category = "Grappling")
	void OnHooked();

protected:
	bool MoveGrappleLine();
	void ReturnGrappleToLauncher();
	bool IsGrappleLineOutOfRope();
	void ReelIn();

protected:
	TWeakObjectPtr<AGrappleHook> mGrappleHook = nullptr;
};
