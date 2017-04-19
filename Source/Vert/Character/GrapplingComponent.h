// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "Components/ActorComponent.h"
#include "Character/GrappleLauncher.h"
#include "Engine/VertGlobals.h"
#include "GrapplingComponent.generated.h"

USTRUCT()
struct FGrappleConfigRules
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Recharge")
	ERechargeRule RechargeMode;

	UPROPERTY(EditDefaultsOnly, Category = "Recharge")
	bool RecieveChargeOnGroundOnly;

	UPROPERTY(EditDefaultsOnly, Category = "Aim")
	EAimFreedom AimFreedom;

	UPROPERTY(EditDefaultsOnly, Category = "Recharge")
	FVertTimer RechargeTimer;

	FGrappleConfigRules()
	{
		RechargeMode = ERechargeRule::OnContactGround;
		RecieveChargeOnGroundOnly = false;
		AimFreedom = EAimFreedom::Free;
	}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VERT_API UGrapplingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grapple")
	FGrappleConfigRules Grapple;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple")
	FName GrappleHandSocket = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class AGrappleLauncher> GrappleClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grapple")
	int32 MaxGrapples;

public:
	UGrapplingComponent();

	bool ExecuteGrapple(const FVector& aimDirection);
	void OnLanded();
	void RegisterGrappleHookDelegates(AGrappleHook* hook);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	FORCEINLINE const int32 GetRemainingGrapples() const { return mRemainingGrapples; }
	FORCEINLINE const EGrappleState GetGrappleState() const { return mGrappleHook.IsValid() ? mGrappleHook->GetGrappleState() : EGrappleState::None; }
	FORCEINLINE const TWeakObjectPtr<class AVertCharacter>& GetCharacterOwner() const { return mCharacterOwner; }
	FORCEINLINE const TWeakObjectPtr<AGrappleLauncher>& GetGrappleLauncher() const { return mGrappleLauncher; }
	FORCEINLINE const TWeakObjectPtr<AGrappleHook>& GetGrappleHook() const { return mGrappleHook; }

	UFUNCTION(BlueprintCallable, Category = "Grappling")
	bool Reset();

	FORCEINLINE UFUNCTION(BlueprintCallable)
	AActor* GetHookedActor() const { return mGrappleHook.IsValid() ? mGrappleHook->GetHookedActor() : nullptr; }

	UFUNCTION(BlueprintCallable, Category = "Grappling", meta = (DisplayName = "Get Grapple Hook"))
	FORCEINLINE AGrappleHook* GetGrappleHookBP() const { return mGrappleHook.Get(); }

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintNativeEvent, Category = "Grappling")
	void OnHooked();

	UFUNCTION(BlueprintNativeEvent, Category = "Grappling")
	void OnFired();

	UFUNCTION(BlueprintNativeEvent, Category = "Grappling")
	void OnReturned();

	UFUNCTION(BlueprintNativeEvent, Category = "Grappling")
	void OnLatched(class AGrappleHook* hook);

	UFUNCTION(BlueprintNativeEvent, Category = "Grappling")
	void OnUnLatched(class AGrappleHook* hook);

	UFUNCTION(BlueprintNativeEvent, Category = "Recharging")
	void OnGrappleRechargeTimerFinished();
	
private:
	void UpdateRechargeSate();

private:
	TWeakObjectPtr<class AVertCharacter> mCharacterOwner = nullptr;
	TWeakObjectPtr<AGrappleLauncher> mGrappleLauncher = nullptr;
	TWeakObjectPtr<AGrappleHook> mGrappleHook = nullptr;
	int32 mRemainingGrapples = 0;
};
