#pragma once

#include "GameFramework/Actor.h"
#include "VertTimer.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTimerFinish);

USTRUCT()
struct FVertTimer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Timer")
	float EndTime;

	UPROPERTY(BlueprintAssignable, Category = "Timer")
	FOnTimerFinish OnFinish;

	FVertTimer()
	{
		EndTime = 2.f;
	}

	FORCEINLINE void BindAlarm(UObject* object, FName functionName)
	{
		FScriptDelegate scriptDelegate;
		scriptDelegate.BindUFunction(object, functionName);
		OnFinish.Add(scriptDelegate);
	}

	FORCEINLINE void Start()
	{
		if(!IsFinished())
			timerActive = true;
	}

	FORCEINLINE void Stop()
	{
		timerActive = false;
	}

	FORCEINLINE bool IsRunning() const 
	{
		return timerActive;
	}

	FORCEINLINE bool IsFinished() const 
	{
		return currentTime >= EndTime;
	}

	FORCEINLINE void Reset()
	{
		currentTime = 0.f;
	}

	FORCEINLINE int32 PopAlarmBacklog()
	{
		int32 backlog = alarmsTriggered;
		alarmsTriggered = 0;
		return backlog;
	}

	FORCEINLINE int32 GetAlarmBacklog() const
	{
		return alarmsTriggered;
	}

	FORCEINLINE float GetProgressRatio()
	{
		return currentTime / EndTime;
	}

	FORCEINLINE float GetProgressPercent()
	{
		return GetProgressRatio() * 100;
	}

	FORCEINLINE void TickTimer(float deltaTime)
	{
		if (timerActive)
		{
			currentTime += deltaTime;
			if (currentTime >= EndTime)
			{
				alarmsTriggered++;
				OnFinish.Broadcast();
				Stop();
			}	
		}
	}

private:
	float currentTime = 0.f;
	int32 alarmsTriggered = 0;
	bool timerActive = false;
};