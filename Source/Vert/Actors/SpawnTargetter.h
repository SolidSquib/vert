// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpawnTargetter.generated.h"

UCLASS()
class VERT_API ASpawnTargetter : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASpawnTargetter();

	UFUNCTION(BlueprintNativeEvent, Category = "Movement")
	void MoveRight(float value);

	UFUNCTION(BlueprintNativeEvent, Category = "Movement")
	void MoveUp(float value);
};
