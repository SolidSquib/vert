// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "VertUtilities.generated.h"

UENUM()
enum class EAimFreedom : uint8
{
	FortyFive UMETA(DisplayName = "Snap 45d"),
	Ninety UMETA(DisplayName = "Snap 90d"),
	Horizontal UMETA(DisplayName = "Snap Horizontal"),
	Free UMETA(DisplayName = "No Snap")
};

/**
*
*/
UCLASS()
class VERT_API UVertUtilities : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	template<typename TEnum>
	static FORCEINLINE FString GetEnumValueToString(const FString& Name, TEnum Value)
	{
		const UEnum* enumPtr = FindObject<UEnum>(ANY_PACKAGE, *Name, true);
		if (!enumPtr)
		{
			return FString("Invalid");
		}

		return enumPtr->GetEnumName((int32)Value);
	}

	UFUNCTION(BlueprintCallable, Category = "CharacterAim")
	static FVector LimitAimTrajectory(EAimFreedom mode, const FVector& vector);

	UFUNCTION(BlueprintCallable, Category = "CharacterAim")
	static FVector2D LimitAimTrajectory2D(EAimFreedom mode, const FVector2D& vector);

	UFUNCTION(BlueprintCallable, Category = "CharacterAim")
	static FVector SnapVectorToAngle(const FVector& vector, float angle);

	UFUNCTION(BlueprintCallable, Category = "CharacterAim")
	static FVector2D SnapVector2DToAngle(const FVector2D& vector, float angle);
};
