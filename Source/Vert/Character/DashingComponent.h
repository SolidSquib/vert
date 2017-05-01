// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "Components/ActorComponent.h"
#include "Engine/VertGlobals.h"
#include "DashingComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDashEndedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDashStartedDelegate, const FVector&, direction);

UENUM(BlueprintType)
enum class EGrappleDashBehaviour : uint8
{
	None,
	Impulse,
	Break
};

USTRUCT(BlueprintType)
struct FGrappleDashResponse 
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Response)
	EGrappleDashBehaviour Action = EGrappleDashBehaviour::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Response)
	FVector Direction = FVector::ZeroVector;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VERT_API UDashingComponent : public UActorComponent
{
	GENERATED_BODY()

	struct FCurrentDash
	{
		FVector Direction = FVector::ZeroVector;
		float DistanceTravelled = 0;
	};

public:
	UPROPERTY(EditDefaultsOnly, Category = "Speed")
	float LaunchForce = 2000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Speed", Meta = (DisplayName = "Last For (s)"))
	FVertTimer DashTimer;

	UPROPERTY(EditDefaultsOnly, Category = "Aim")
	EAimFreedom AimFreedom = EAimFreedom::Ninety;

	UPROPERTY(EditDefaultsOnly, Category = "Recharge")
	ERechargeRule RechargeMode = ERechargeRule::OnRechargeTimer;

	UPROPERTY(EditDefaultsOnly, Category = "Recharge")
	bool RecieveChargeOnGroundOnly = true;

	UPROPERTY(EditDefaultsOnly, Category = "Recharge")
	FVertTimer RechargeTimer;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Usage")
	int32 MaxDashes = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooldown")
	FVertTimer CooldownTimer;

	/* Use to apply and air friction for a given time to avoid players flying off into nothingness constantly. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooldown")
	FVertTimer AirSlowdownTimer;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooldown")
	float AirSlowdownFriction = 1.f;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnDashStartedDelegate OnDashStarted;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnDashEndedDelegate OnDashEnd;

public:	
	// Sets default values for this component's properties
	UDashingComponent();

	void OnLanded();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	FORCEINLINE const int32 GetRemainingDashes() const { return mRemainingDashes; }
	FORCEINLINE const bool IsCurrentlyDashing() const { return mIsDashing; }
	FORCEINLINE const TWeakObjectPtr<class AVertCharacter>& GetCharacterOwner() const { return mCharacterOwner; }

	UFUNCTION(BlueprintCallable, Category = "Dash")
	bool ExecuteGroundDash();

	UFUNCTION(BlueprintCallable, Category = "Dash")
	bool ExecuteGrappleDash(const FVector& hookDirection, FGrappleDashResponse& response);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	void UpdateRechargeState();
	void CooldownFinishedCallback();
	FVector FindLaunchDirection();
	FVector FindLimitedLaunchDirection();

	FORCEINLINE const bool CanDash() const { return !mIsDashing && !mOnCooldown && (mRemainingDashes > 0 || MaxDashes == -1); }

	UFUNCTION()
	void RechargeTimerEnded();

	UFUNCTION()
	void DashTimerEnded();

	UFUNCTION()
	void CooldownEnded();

	UFUNCTION()
	void AirSlowdownTimerEnded();

private:
	TWeakObjectPtr<class AVertCharacter> mCharacterOwner = nullptr;

	bool mIsDashing = false;
	bool mOnCooldown = false;
	bool mIsGroundDash = true;
	int32 mRemainingDashes = 0;	
	float mStoredGroundFriction = 0;
	float mStoredAirLateralFriction = 0;
};
