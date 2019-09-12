// Copyright Inside Out Games Limited 2017

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/WorldSettings.h"
#include "VertWorldSettings.generated.h"

/**
 * 
 */
UCLASS()
class VERT_API AVertWorldSettings : public AWorldSettings
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PlayerColours")
	TArray<struct FPlayerColours> PlayerColours;

public:
	AVertWorldSettings();
};
