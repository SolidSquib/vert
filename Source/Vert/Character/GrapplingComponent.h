// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "Components/ActorComponent.h"
#include "Character/GrappleLauncher.h"
#include "Engine/VertGlobals.h"
#include "GrapplingComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VERT_API UGrapplingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Launcher")
	FName GrappleHandSocket = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Launcher")
	TSubclassOf<class AGrappleLauncher> GrappleClass = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grapple|Usage")
	int32 MaxGrapples = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Grapple|Usage")
	ERechargeRule RechargeMode = ERechargeRule::OnContactGround;

	UPROPERTY(EditDefaultsOnly, Category = "Grapple|Usage")
	bool RecieveChargeOnGroundOnly = false;

	UPROPERTY(EditDefaultsOnly, Category = "Grapple|Usage")
	EAimFreedom AimFreedom = EAimFreedom::Free;

	/* Should we ignore all the fancy stuff and just use LaunchCharacter? */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Grapple|Line")
	bool UseSimpleGrapple = false;

	/* Determines how stiff or springy the cable is; higher values mean a stiffer cable and vice-versa */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Grapple|Line", meta = (EditCondition = "!UseSimpleGrapple", DisplayName = "Stiffness (k)"))
	float LineSpringCoefficient = 100000.f;

	/* Determines how quickly the effects of the cable's spring physics 'wear down'. A Higher value means the spring will come to a stop quicker, but values that are too high will likely cause issues... */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Grapple|Line", meta = (EditCondition = "!UseSimpleGrapple", DisplayName = "Spring Damping (b)"))
	float LineDampingCoefficient = 1.f;

	/* Set this to true to make the grapple pull the character as soon as it contacts a surface, instead of waiting for a second activation. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Grapple|Line", meta = (EditCondition = "!UseSimpleGrapple"))
	bool PullOnContact = false;

	/* Whether we should move the character up slightly when leaving a grapple to help with getting over ledges. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grapple|Line", meta = (EditCondition = "!UseSimpleGrapple"))
	bool OffsetCharacterHeightOnCutLine = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Grapple|Line", meta = (EditCondition = "!UseSimpleGrapple"))
	float LineCutLength = 20.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Grapple|Line")
	float MaxLineLength = 500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Grapple|Line")
	float LaunchSpeed = 3000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Grapple|Line")
	float PullSpeed = 700.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Grapple|Line")
	float ReelSpeed = 3500.f;

	/* Determines whether the line should behave as a string or a spring. Strings will only correct themselves if they are too long, springs will bounce in a more uniform manner. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Grapple|Line")
	bool StringContraint = true;

public:
	UGrapplingComponent();

	void OnLanded();
	void RegisterGrappleHookDelegates(AGrappleHook* hook);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	FORCEINLINE const TWeakObjectPtr<class AVertCharacter>& GetCharacterOwner() const { return mCharacterOwner; }
	FORCEINLINE const TWeakObjectPtr<AGrappleLauncher>& GetGrappleLauncher() const { return mGrappleLauncher; }
	FORCEINLINE const TWeakObjectPtr<AGrappleHook>& GetGrappleHook() const { return mGrappleHook; }

	UFUNCTION(BlueprintCallable, Category = "Grappling")
	bool Reset();

	UFUNCTION(BlueprintCallable, Category = "Grappling")
	bool ExecuteGrapple(FVector& aimDirection);

	UFUNCTION(BlueprintCallable, Category = "Grappling")
	bool StartPulling();

	FORCEINLINE UFUNCTION(BlueprintCallable)
	AActor* GetHookedActor() const { return mGrappleHook.IsValid() ? mGrappleHook->GetHookedActor() : nullptr; }

	FORCEINLINE UFUNCTION(BlueprintCallable)
	UPrimitiveComponent* GetHookedPrimitive() const { return mGrappleHook.IsValid() ? mGrappleHook->GetHookedPrimitive() : nullptr; }

	FORCEINLINE UFUNCTION(BlueprintCallable, Category = "Grappling")
	float GetLineLength() const { return mGrappleHook.IsValid() ? mGrappleHook->GetCurrentDistanceFromLauncher() : 0.f; }

	UFUNCTION(BlueprintCallable, Category = "Grappling")
	FORCEINLINE FVector GetLineDirection() const { return (mGrappleHook->GetActorLocation() - mGrappleLauncher->GetActorLocation()).GetSafeNormal(); }

	UFUNCTION(BlueprintCallable, Category = "Usage")
	FORCEINLINE int32 GetRemainingGrapples() const { return mRemainingGrapples; }

	UFUNCTION(BlueprintCallable, Category = "States")
	FORCEINLINE EGrappleState GetGrappleState() const { return mGrappleHook.IsValid() ? mGrappleHook->GetGrappleState() : EGrappleState::None; }

	UFUNCTION(BlueprintCallable, Category = "States")
	FORCEINLINE bool IsGrappleDeployed() const { return mGrappleHook->GetGrappleState() == EGrappleState::HookDeployed || mGrappleHook->GetGrappleState() == EGrappleState::HookDeployedAndReturning; }

	UFUNCTION(BlueprintCallable, Category = "Grappling", meta = (DisplayName = "Get Grapple Hook"))
	AGrappleHook* GetGrappleHookBP() const;

	FORCEINLINE UFUNCTION(BlueprintCallable, Category = "Grappling")
	float GetRechargePercent() const { return mRechargeTimer.GetProgressPercent(); }

	FORCEINLINE UFUNCTION(BlueprintCallable, Category = "Grappling")
	bool IsRecharging() const { return mRechargeTimer.IsRunning(); }

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintNativeEvent, Category = "Recharging")
	void OnGrappleRechargeTimerFinished();
	
private:
	void UpdateRechargeSate();
	
private:
	TWeakObjectPtr<class AVertCharacter> mCharacterOwner = nullptr;
	TWeakObjectPtr<AGrappleLauncher> mGrappleLauncher = nullptr;
	TWeakObjectPtr<AGrappleHook> mGrappleHook = nullptr;
	
	int32 mRemainingGrapples = 0;
	bool mPullCharacter = false;
	FVertTimer mRechargeTimer;
};
