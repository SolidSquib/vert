// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "GameFramework/Actor.h"
#include "VertPlayerCameraActor.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogVertPlayerCamera, Log, All);

UCLASS()
class VERT_API AVertPlayerCameraActor : public AActor
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	float InterpSpeed;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* SideViewCameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

public:	
	// Sets default values for this actor's properties
	AVertPlayerCameraActor();

	UFUNCTION(BlueprintCallable, Category = "PlayerPosition")
	void RegisterPlayerPawn(class APawn* pawnToFollow);

	UFUNCTION(BlueprintCallable, Category = "PlayerPosition")
	void UnregisterPlayerPawn(class APawn* pawnToFollow);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	TArray<class APawn*> mPawnsToFollow;
};
