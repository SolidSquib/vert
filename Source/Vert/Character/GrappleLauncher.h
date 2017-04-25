// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "Character/GrappleHook.h"
#include "GrappleLauncher.generated.h"

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
	TSubclassOf<AGrappleHook> HookClass;

public:
	// Sets default values for this component's properties
	AGrappleLauncher();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	bool FireGrapple(const FVector& shootDirection, bool wasGamepadTriggered = false);
	bool FireGrapple(const FVector2D& shootDirection, bool wasGamepadTriggered = false);
	void ResetGrapple();

	FORCEINLINE const TWeakObjectPtr<AGrappleHook>& GetGrappleHook() const { return mGrappleHook; }

	UFUNCTION(BlueprintCallable)
	class AVertCharacter* GetCharacterOwner() const;

	UFUNCTION(BlueprintNativeEvent, Category = "Grappling")
	void OnHooked();
	
protected:
	TWeakObjectPtr<AGrappleHook> mGrappleHook = nullptr;
	TWeakObjectPtr<class AVertCharacter> mCharacterOwner = nullptr;
	TWeakObjectPtr<class UGrapplingComponent> mGrapplingComponent = nullptr;
};
