// Copyright Inside Out Games Ltd. 2017

#include "ItemDropPod.h"

// Sets default values
AItemDropPod::AItemDropPod()
	: BoxCollision(CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision")))
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	BoxCollision->SetCollisionObjectType(ECC_WorldDynamic);
}

// Called when the game starts or when spawned
void AItemDropPod::BeginPlay()
{
	Super::BeginPlay();
	
}

void AItemDropPod::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}
