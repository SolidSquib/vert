// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "GameFramework/SpectatorPawn.h"
#include "VertSpectator.generated.h"

/**
 * 
 */
UCLASS()
class VERT_API AVertSpectator : public ASpectatorPawn
{
	GENERATED_BODY()
		
private:
	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class USkeletalMeshComponent* Mesh;

public:
	AVertSpectator();

	virtual void Tick(float DeltaSeconds) override;
	virtual void PostInitializeComponents() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;

	void UpdateAnimation();
	void UpdateCharacter();
	void MoveRight(float Value);
	void MoveUp(float Value);

	FORCEINLINE class USkeletalMeshComponent* GetMesh() const { return Mesh; }
};
