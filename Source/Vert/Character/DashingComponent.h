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

USTRUCT()
struct FDashConfigRules
{
	GENERATED_USTRUCT_BODY()
		
	UPROPERTY(EditDefaultsOnly, Category = "LaunchOptions")
	uint8 UseMomentum : 1;

	UPROPERTY(EditDefaultsOnly, Category = "Speed", Meta = (EditCondition = "UseMomentum"))
	float LaunchForce;

	UPROPERTY(EditDefaultsOnly, Category = "LaunchOptions", Meta = (EditCondition = "UseMomentum", DisplayName = "Override Horizontal Velocity"))
	uint8 OverrideXY : 1;

	UPROPERTY(EditDefaultsOnly, Category = "LaunchOptions", Meta = (EditCondition = "UseMomentum", DisplayName = "Override Vertical Velocity"))
	uint8 OverrideZ : 1;

	UPROPERTY(EditDefaultsOnly, Category = "LaunchOptions", Meta = (EditCondition = "UseMomentum", DisplayName = "Last For (s)"))
	float TimeToDash;

	UPROPERTY(EditDefaultsOnly, Category = "Speed", Meta = (EditCondition = "!UseMomentum"))
	float DashLength;

	UPROPERTY(EditDefaultsOnly, Category = "Speed", Meta = (EditCondition = "!UseMomentum"))
	float LinearSpeed;

	UPROPERTY(EditDefaultsOnly, Category = "Aim")
	EDashAimMode AimMode;

	UPROPERTY(EditDefaultsOnly, Category = "Aim")
	EAimFreedom AimFreedom;

	UPROPERTY(EditDefaultsOnly, Category = "Recharge")
	ERechargeRule RechargeMode;

	UPROPERTY(EditDefaultsOnly, Category = "Recharge")
	bool RecieveChargeOnGroundOnly;

	UPROPERTY(EditDefaultsOnly, Category = "Recharge")
	FVertTimer RechargeTimer;

	uint8 IsDashing : 1;
	float DistanceTravelled;
	FVector DirectionOfTravel;
	float Timer;
	FDashConfigRules()
	{
		UseMomentum = true;
		LaunchForce = 2000.f;
		OverrideXY = true;
		OverrideZ = true;
		TimeToDash = 0.5;
		DashLength = 20.f;
		LinearSpeed = 20.f;
		AimMode = EDashAimMode::PlayerDirection;
		AimFreedom = EAimFreedom::Free;
		RechargeMode = ERechargeRule::OnContactGround;
		RecieveChargeOnGroundOnly = false;

		IsDashing = false;
		DistanceTravelled = 0.f;
		DirectionOfTravel = FVector::ZeroVector;
		Timer = 0.f;
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDashEndedDelegate);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VERT_API UDashingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash")
	FDashConfigRules Dash;

	UPROPERTY(BlueprintAssignable, Category = "Dash|Events")
	FOnDashEndedDelegate OnDashEnd;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash")
	int32 MaxDashes;

public:	
	// Sets default values for this component's properties
	UDashingComponent();

	bool ExecuteDash();
	void OnLanded();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	FORCEINLINE const int32 GetRemainingDashes() const { return mRemainingDashes; }
	FORCEINLINE const bool IsCurrentlyDashing() const { return Dash.IsDashing; }
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
	int32 mRemainingDashes = 0;
};
