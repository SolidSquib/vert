// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "CollisionQueryParams.h"
#include <DrawDebugHelpers.h>
#include "Kismet/KismetSystemLibrary.h"
#include "VertUtilities.generated.h"

UENUM()
enum class EAimFreedom : uint8
{
	FortyFive UMETA(DisplayName = "Snap 45d"),
	Ninety UMETA(DisplayName = "Snap 90d"),
	Horizontal UMETA(DisplayName = "Snap Horizontal"),
	Free UMETA(DisplayName = "No Snap")
};

USTRUCT(BlueprintType)
struct FVertRadialDamageEvent : public FRadialDamageEvent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knockback")
	float Knockback = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knockback")
	float KnockbackScaling = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stun")
	float StunTime = 0;

	/** ID for this class. NOTE this must be unique for all damage events. */
	static const int32 ClassID = 3;

	virtual int32 GetTypeID() const override { return FVertRadialDamageEvent::ClassID; };
	virtual bool IsOfType(int32 InID) const override { return (FVertRadialDamageEvent::ClassID == InID) || FRadialDamageEvent::IsOfType(InID); };
};

USTRUCT(BlueprintType)
struct FVertPointDamageEvent : public FPointDamageEvent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knockback")
	float Knockback;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knockback")
	float KnockbackScaling;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stun")
	float StunTime;

	FVertPointDamageEvent() : Knockback(100.f), KnockbackScaling(.2f), StunTime(0) {}
	FVertPointDamageEvent(float InDamage, float InBaseKnockback, float InKnockbackScaling, float InStunTime, struct FHitResult const& InHitInfo, FVector const& InShotDirection, TSubclassOf<class UDamageType> InDamageTypeClass)
		: FPointDamageEvent(InDamage, InHitInfo, InShotDirection, InDamageTypeClass), Knockback(InBaseKnockback), KnockbackScaling(InKnockbackScaling), StunTime(InStunTime)
	{}

	/** ID for this class. NOTE this must be unique for all damage events. */
	static const int32 ClassID = 4;

	virtual int32 GetTypeID() const override { return FVertPointDamageEvent::ClassID; };
	virtual bool IsOfType(int32 InID) const override { return (FVertPointDamageEvent::ClassID == InID) || FPointDamageEvent::IsOfType(InID); };
};

/**
*
*/
UCLASS()
class VERT_API UVertUtilities : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static bool SphereTraceSingleByChannel(const FVector& start, const FVector& end, const float radius, FHitResult& hitOut, const FCollisionQueryParams& params, ECollisionChannel traceChannel = ECC_Pawn);
	static bool SphereTraceSingleByObjectTypes(const FVector& start, const FVector& end, const float radius, FHitResult& hitOut, const FCollisionQueryParams& params, const FCollisionObjectQueryParams& objectTypes);
	static bool SphereTraceMultiByChannel(const FVector& start, const FVector& end, const float radius, TArray<FHitResult>& hitOut, const FCollisionQueryParams& params, ECollisionChannel traceChannel = ECC_Pawn);
	static bool SphereTraceMultiByObjectTypes(const FVector& start, const FVector& end, const float radius, TArray<FHitResult>& hitOut, const FCollisionQueryParams& params, const FCollisionObjectQueryParams& objectTypes);

#if ENABLE_DRAW_DEBUG
	static void DrawDebugSweptSphere(const UWorld* InWorld, FVector const& Start, FVector const& End, float Radius, FColor const& Color, bool bPersistentLines = false, float LifeTime = -1.f, uint8 DepthPriority = 0);
	static void DrawDebugSweptBox(const UWorld* InWorld, FVector const& Start, FVector const& End, FRotator const & Orientation, FVector const & HalfSize, FColor const& Color, bool bPersistentLines = false, float LifeTime = -1.f, uint8 DepthPriority = 0);
	static void DrawDebugLineTraceSingle(const UWorld* World, const FVector& Start, const FVector& End, EDrawDebugTrace::Type DrawDebugType, bool bHit, FHitResult& OutHit, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime);
	static void DrawDebugLineTraceMulti(const UWorld* World, const FVector& Start, const FVector& End, EDrawDebugTrace::Type DrawDebugType, bool bHit, const TArray<FHitResult>& OutHits, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime);
	static void DrawDebugBoxTraceSingle(const UWorld* World, const FVector& Start, const FVector& End, const FVector HalfSize, const FRotator Orientation, EDrawDebugTrace::Type DrawDebugType, bool bHit, FHitResult& OutHit, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime);
	static void DrawDebugBoxTraceMulti(const UWorld* World, const FVector& Start, const FVector& End, const FVector HalfSize, const FRotator Orientation, EDrawDebugTrace::Type DrawDebugType, bool bHit, TArray<FHitResult>& OutHits, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime);
	static void DrawDebugSphereTraceSingle(const UWorld* World, const FVector& Start, const FVector& End, float Radius, EDrawDebugTrace::Type DrawDebugType, bool bHit, FHitResult& OutHit, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime);
	static void DrawDebugSphereTraceMulti(const UWorld* World, const FVector& Start, const FVector& End, float Radius, EDrawDebugTrace::Type DrawDebugType, bool bHit, TArray<FHitResult>& OutHits, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime);
	static void DrawDebugCapsuleTraceSingle(const UWorld* World, const FVector& Start, const FVector& End, float Radius, float HalfHeight, EDrawDebugTrace::Type DrawDebugType, bool bHit, FHitResult& OutHit, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime);
	static void DrawDebugCapsuleTraceMulti(const UWorld* World, const FVector& Start, const FVector& End, float Radius, float HalfHeight, EDrawDebugTrace::Type DrawDebugType, bool bHit, TArray<FHitResult>& OutHits, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime);
#endif

	template<typename TEnum>
	static FORCEINLINE FString GetEnumValueToString(const FString& Name, TEnum Value)
	{
		const UEnum* enumPtr = FindObject<UEnum>(ANY_PACKAGE, *Name, true);
		if (!enumPtr)
		{
			return FString("Invalid");
		}

		return enumPtr->GetNameStringByIndex((int32)Value);
	}

	UFUNCTION(BlueprintCallable, Category = "CharacterAim")
	static FVector LimitAimTrajectory(EAimFreedom mode, const FVector& vector);

	UFUNCTION(BlueprintCallable, Category = "CharacterAim")
	static FVector2D LimitAimTrajectory2D(EAimFreedom mode, const FVector2D& vector);

	UFUNCTION(BlueprintCallable, Category = "CharacterAim")
	static FVector SnapVectorToAngle(const FVector& vector, float angle);

	UFUNCTION(BlueprintCallable, Category = "CharacterAim")
	static FVector2D SnapVector2DToAngle(const FVector2D& vector, float angle);

	UFUNCTION(BlueprintCallable, Category = "Camera Frustum")
	static bool IsActorInFrustum(const UObject* worldContextObject, AActor* actor);

	UFUNCTION(BlueprintCallable, Category = "Camera Frustum")
	static float GetSquareDistanceFromFrustum(const UWorld* world, AActor* actor);

	UFUNCTION(BlueprintCallable, Category = "Animation")
	static float GetAnimationTime(class UAnimSequence* animSequence);

	UFUNCTION(BlueprintCallable, Category = "Damage")
	static float VERT_TakePointDamage(AActor* victim, float damage, TSubclassOf<UDamageType> damageType, const FVector& shotDirection, const FHitResult& hitInfo, float Knockback, float KnockbackScaling, float StunTime, class AController* eventInstigator, AActor* damageCauser);

	UFUNCTION(BlueprintCallable, Category = "Damage")
	static bool VERT_ApplyRadialDamage(const UObject* WorldContextObject, float BaseDamage, float BaseKnockback, float KnockbackScaling, float stunTime, const FVector& Origin, float DamageRadius, TSubclassOf<class UDamageType> DamageTypeClass, const TArray<AActor*>& IgnoreActors, AActor* DamageCauser = NULL, AController* InstigatedByController = NULL, bool bDoFullDamage = false, ECollisionChannel DamagePreventionChannel = ECC_Visibility);

	UFUNCTION(BlueprintCallable, Category = "Items")
	static void VERT_MakeWeaponDrop(const UObject* worldContextObject, const TArray<AActor*>& spawnLocations, int32 numberOfPods, int32 itemsPerPod, float delayTime, UParticleSystem* warningFX = nullptr, FName warningBeamEndParam = NAME_None, float warningTime = 2.f);

	UFUNCTION(BlueprintCallable, Category = "Camera")
	static void GetRatioDistanceFromCamera(const UObject* worldContextObject, const AActor* checkActor, float& outX, float& outZ, float& outHorizontalDistanceToEdge, float& outVerticalDistanceToEdge);

	UFUNCTION(BlueprintCallable, Category = "Viewport|Utilities")
	static void FadeToBlack(const UObject* worldContextObject, float duration);

	UFUNCTION(BlueprintCallable, Category = "Viewport|Utilities")
	static void FadeFromBlack(const UObject* worldContextObject, float duration);

	UFUNCTION(BlueprintCallable, Category = "Math|Utilities")
	static bool LineToLineIntersection(const FVector& fromA, const FVector& fromB, const FVector& toA, const FVector& toB, FVector& outIntersection);

private:
	static bool VERT_ApplyRadialDamageWithFalloff(const UObject* WorldContextObject, float BaseDamage, float BaseKnockback, float KnockbackScaling, float stunTime, float MinimumDamage, const FVector& Origin, float DamageInnerRadius, float DamageOuterRadius, float DamageFalloff, TSubclassOf<class UDamageType> DamageTypeClass, const TArray<AActor*>& IgnoreActors, AActor* DamageCauser, AController* InstigatedByController, ECollisionChannel DamagePreventionChannel);
};
