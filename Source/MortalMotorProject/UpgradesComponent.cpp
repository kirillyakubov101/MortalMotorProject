 // Fill out your copyright notice in the Description page of Project Settings.


#include "UpgradesComponent.h"
#include "Minigun.h"

// Sets default values for this component's properties
UUpgradesComponent::UUpgradesComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	
}


// Called when the game starts
void UUpgradesComponent::BeginPlay()
{
	Super::BeginPlay();

	TArray<AActor*> ChildActors;
	GetOwner()->GetAllChildActors(ChildActors);

	//Here you can get all the actors that are children of the Player
	for (AActor* ChildActor : ChildActors)
	{
		if (ChildActor->IsA(AMinigun::StaticClass()))
		{
			m_Minigun = Cast<AMinigun>(ChildActor);
		}
	}
}


// Called every frame
void UUpgradesComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

void UUpgradesComponent::EnableMinigun()
{
	if(m_Minigun != nullptr)
	{
		m_Minigun->ShowMinigun();
		m_Minigun->SetActorTickEnabled(true);
	}
}

