// Fill out your copyright notice in the Description page of Project Settings.


#include "ZombieRunner.h"
#include "AIController.h" 
#include "Particles/ParticleSystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PlayerMotorCar.h"
#include "Components/CapsuleComponent.h"
#include "Sound/SoundCue.h"
#include "Sound/SoundWave.h"

// Sets default values
AZombieRunner::AZombieRunner()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;



	//Get Anim montages
	Hit_Montage01 = LoadObject<UAnimMontage>(nullptr, TEXT("/Script/Engine.AnimMontage'/Game/Juan_Active_Branch/Enemies/Zombie_03/Anim/Zombie_Reaction_Hit_01_Montage_Retargeted.Zombie_Reaction_Hit_01_Montage_Retargeted'"));
	Hit_Montage02 = LoadObject<UAnimMontage>(nullptr, TEXT("/Script/Engine.AnimMontage'/Game/Juan_Active_Branch/Enemies/Zombie_03/Anim/Zombie_Reaction_Hit_02_Montage_Retargeted.Zombie_Reaction_Hit_02_Montage_Retargeted'"));
	Death_Montage01 = LoadObject<UAnimMontage>(nullptr, TEXT("/Script/Engine.AnimMontage'/Game/Juan_Active_Branch/Enemies/Zombie_03/Anim/ZombieDeath_01_Montage_Retargeted.ZombieDeath_01_Montage_Retargeted'"));
	Death_Montage02 = LoadObject<UAnimMontage>(nullptr, TEXT("/Script/Engine.AnimMontage'/Game/Juan_Active_Branch/Enemies/Zombie_03/Anim/ZombieDeath_02_Montage_Retargeted.ZombieDeath_02_Montage_Retargeted'"));


	//Get particle system
	HitParticlesComponent = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("HitEffectComponent"));
	HitParticlesComponent->SetupAttachment(RootComponent);
	HitParticlesComponent->bAutoActivate = false;

	ParticleSystemTemplate = LoadObject<UParticleSystem>(nullptr, TEXT("/Script/Engine.ParticleSystem'/Game/Juan_Active_Branch/Realistic_Starter_VFX_Pack_Vol2/Particles/Blood/P_Blood_Splat_Cone.P_Blood_Splat_Cone'"));
	bIsPsPlaying = false;

	//Get audio
	SoundCueBodyFall = LoadObject<USoundCue>(nullptr, TEXT("/Script/Engine.SoundCue'/Game/Blueprints/Enemies/SFX/SC_Body_Fall.SC_Body_Fall'"));
	SoundCueHitCar = LoadObject<USoundCue>(nullptr, TEXT("/Script/Engine.SoundCue'/Game/Blueprints/Enemies/SFX/SC_Car_Hit.SC_Car_Hit'"));
}

void AZombieRunner::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// after the constructor is completed check if the components were added and start assigning
	if (GetMesh())
	{
		ZombieAnimInstance = GetMesh()->GetAnimInstance();
	}

	if (Hit_Montage01)
	{
		Hit_Montages.Add(Hit_Montage01);
	}

	if (Hit_Montage02)
	{
		Hit_Montages.Add(Hit_Montage02);
	}

	if (Death_Montage01)
	{
		Death_Montages.Add(Death_Montage01);
	}

	if (Death_Montage02)
	{
		Death_Montages.Add(Death_Montage02);
	}

}

// Called when the game starts or when spawned
void AZombieRunner::BeginPlay()
{
	Super::BeginPlay();

	ZombieController = Cast<AAIController>(GetController());

	Player = UGameplayStatics::GetPlayerPawn(this, 0);

	bIsCollidingWithPlayer = false;

	this->GetCapsuleComponent()->SetGenerateOverlapEvents(true);
	this->GetCapsuleComponent()->OnComponentHit.AddDynamic(this, &AZombieRunner::OnHit); 
	this->GetMesh()->OnComponentHit.AddDynamic(this, &AZombieRunner::OnHit);
	 
}

// Called every frame
void AZombieRunner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (Player)
	{
		ChasePlayer(Player->GetActorLocation());
	}

	/*const FVector VehicleVelocity = Player->GetVelocity();
	const float Speed = VehicleVelocity.Size();

	GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Yellow, FString::Printf(TEXT("RandomIndex: %f"), Speed));*/
}


void AZombieRunner::TakeDamge(float damage)
{

	HealthPoints = FMath::Max(0, HealthPoints - damage);
	// rewrote this part to use the IsAlive function since it already returns the hitpoint count
	if (IsAlive())
	{
		// playing a random index from an array that contains the animations when the zombie is hit by something
		if (Hit_Montages.Num() > 0)
		{
			int32 RandomIndex = FMath::RandRange(0, Hit_Montages.Num() - 1);
			ZombieAnimInstance->Montage_Play(Hit_Montages[RandomIndex], 1.0f);
			//  GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Yellow, FString::Printf(TEXT("Montage array size: %i"), Hit_Montages.Num()));
			//GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Yellow, FString::Printf(TEXT("RandomIndex: %i"), RandomIndex));
		}

		if (HitParticlesComponent)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Blue, TEXT("PARTICLE EFFECTS PLAY"));
			ParticleSystem();
		}
	}
	else
	{
		Death();
	}
}

void AZombieRunner::DestroyEnemy()
{
	Destroy();
}


void AZombieRunner::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor && OtherActor != this && OtherComp)
	{
		APlayerMotorCar* Car = Cast<APlayerMotorCar>(OtherActor);
		if (Car)
		{
			bIsCollidingWithPlayer = true;
			// Print collision for debugging
			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Blue, FString::Printf(TEXT("Zombie collided with Car: %s"), bIsCollidingWithPlayer ? TEXT("true") : TEXT("false")));
			TakeDamge(100.0f);
		}

	/*	if (OtherComp->GetCollisionObjectType() == ECollisionChannel::ECC_WorldDynamic)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), SoundCueBodyFall, this->GetActorLocation());
		}*/
	}
}

void AZombieRunner::ChasePlayer(const FVector& TargetLocation) const
{
	if (ZombieController && IsAlive())
	{
		ZombieController->MoveToLocation(TargetLocation);
	}
}



void AZombieRunner::Death()
{
	DisableCollision();
	// command assigned directly to the character movement component it halts the object regardless of the AIController.  
	this->GetCharacterMovement()->StopMovementImmediately();

	// added a delay to the destruction of the objects inheriting from this class to allow some room to play death animations or additional code 
	const float Delay = 4.0f;

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, this, &AZombieRunner::DestroyEnemy, Delay, false);

	if (bIsCollidingWithPlayer)
	{
		if (HitParticlesComponent)
		{
			ParticleSystem();		 
		}
		BecomeRagdoll();

		if(SoundCueBodyFall)
		{
			float Cue1duration = SoundCueHitCar->Duration;

			GetWorldTimerManager().SetTimer(TimerHandle, this, &AZombieRunner::PlaySoundCueHitGround, Cue1duration, false);

		}
		 
		
	}
	else
	{
		if (Death_Montages.Num() > 0)
		{
			const int32 RandomIndex = FMath::RandRange(0, Death_Montages.Num() - 1);
			ZombieAnimInstance->Montage_Play(Death_Montages[RandomIndex], 1.0f);
		}
	}
}

//This function was added to replay the PS every time it is called instead of waiting for it to finish its regular lifetime before calling again
void AZombieRunner::ParticleSystem()
{
	if (!bIsPsPlaying)
	{
		// add the template and play ps
		HitParticlesComponent->SetTemplate(ParticleSystemTemplate);
		HitParticlesComponent->ActivateSystem();

		// Set bool to true
		bIsPsPlaying = true;
	}
	else
	{
		// Stop the currently playing instance
		HitParticlesComponent->DeactivateSystem();

		// Spawn a new particle system instance
		HitParticlesComponent->SetTemplate(ParticleSystemTemplate);
		HitParticlesComponent->ActivateSystem();
	}
}

void AZombieRunner::DisableCollision()
{
	this->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// set the zombie to ragdoll collision type and set simulate physics to true 
void AZombieRunner::BecomeRagdoll()
{
	const ECollisionChannel CollisionObjectType = ECC_PhysicsBody;
	GetMesh()->SetCollisionObjectType(CollisionObjectType);
	GetMesh()->SetSimulatePhysics(true);
	DisableCollision();

	if (SoundCueHitCar)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), SoundCueHitCar, this->GetActorLocation());
	}
}

void AZombieRunner::PlaySoundCueHitGround()
{
	if(SoundCueBodyFall)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), SoundCueBodyFall, this->GetActorLocation());
	}
}




