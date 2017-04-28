// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "Components/ActorComponent.h"
#include "Engine/VertGlobals.h"
#include "DashingComponent.generated.h"

UENUM()
enum class EDashAimMode : uint8
{
	PlayerDirection UMETA(DisplayName = "Player Direction (Left Stick)"),
	AimDirection UMETA(DisplayName = "Aim Direction (Right Stick)")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDashEndedDelegate);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VERT_API UDashingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Launch")
	uint8 UseMomentum : 1;

	UPROPERTY(EditDefaultsOnly, Category = "Launch", Meta = (EditCondition = "UseMomentum", DisplayName = "Override Horizontal Velocity"))
	uint8 OverrideXY : 1;

	UPROPERTY(EditDefaultsOnly, Category = "Launch", Meta = (EditCondition = "UseMomentum", DisplayName = "Override Vertical Velocity"))
	uint8 OverrideZ : 1;

	UPROPERTY(EditDefaultsOnly, Category = "Speed", Meta = (EditCondition = "UseMomentum"))
	float LaunchForce = 2000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Launch", Meta = (EditCondition = "UseMomentum", DisplayName = "Last For (s)"))
	FVertTimer DashTimer;

	UPROPERTY(EditDefaultsOnly, Category = "Speed", Meta = (EditCondition = "!UseMomentum"))
	float DashLength = 20.f;

	UPROPERTY(EditDefaultsOnly, Category = "Speed", Meta = (EditCondition = "!UseMomentum"))
	float LinearSpeed = 20.f;

	UPROPERTY(EditDefaultsOnly, Category = "Aim")
	EDashAimMode AimMode = EDashAimMode::PlayerDirection;

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

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnDashEndedDelegate OnDashEnd;

public:	
	// Sets default values for this component's properties
	UDashingComponent();

	bool ExecuteDash();
	void OnLanded();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	FORCEINLINE const int32 GetRemainingDashes() const { return mRemainingDashes; }
	FORCEINLINE const bool IsCurrentlyDashing() const { return mIsDashing; }
	FORCEINLINE const TWeakObjectPtr<class AVertCharacter>& GetCharacterOwner() const { return mCharacterOwner; }

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintNativeEvent, Category = "Dash|Recharging")
	void OnDashRechargeTimerFinished();

private:
	void UpdateRechargeSate();
	void EndDash();

private:
	TWeakObjectPtr<class AVertCharacter> mCharacterOwner = nullptr;

	bool mIsDashing = false;
	int32 mRemainingDashes = 0;
	FVector mDirectionOfTravel = FVector::ZeroVector;
	float mDistanceTravelled = 0.f;
};
