// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerMotorCar.h"
#include "Gold.h"
#include "PlayerUI.h"
#include "ChaosWheeledVehicleMovementComponent.h" 
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SphereComponent.h"
#include "IDamageable.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundCue.h"

bool APlayerMotorCar::bIsPlayerDead;
bool APlayerMotorCar::bResetCamera = false;
float APlayerMotorCar::BestTime;
float APlayerMotorCar::TotalDistanceCovered;
FString APlayerMotorCar::SurvivedTimeString;
FString APlayerMotorCar::BestTimeString;
 

APlayerMotorCar::APlayerMotorCar() :
	GoldAmount(0),
	Level(0),
	bIsInvinisible(false)
{
	//killzone sphere
	KillZoneCollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Kill Zone Sphere"));
	KillZoneCollisionSphere->SetupAttachment(RootComponent);

	PlayerHealth = MAX_HEALTH;
	//TotalDistanceCovered = 0;
	bIsPlayerDead = false;
	bPlayerJustSpawn = true;

	//sfx
	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
	AudioComponent->SetupAttachment(GetRootComponent());
	AudioComponent->bAutoActivate = false;
	
	 
}

void APlayerMotorCar::BeginPlay()
{
	Super::BeginPlay();
	AGold::s_OnGoldCollected.BindUObject(this, &APlayerMotorCar::HandleGoldCollected);
	IIDamageable::OnEnemyKilledDelegate.BindUObject(this, &APlayerMotorCar::HandleRewardCollected);

	//create the Widget Ui based of the WidgetObject subclass
	PlayerUI = CreateWidget<UPlayerUI>(GetWorld(), WidgetObject);
	//add the created UI to the viewport
	PlayerUI->AddToViewport();

	//cache the spring arm component
	SpringArm = Cast<USpringArmComponent>(GetComponentByClass(USpringArmComponent::StaticClass()));

	FScriptDelegate ScriptDelegate;
	ScriptDelegate.BindUFunction(this, FName("OnOverlapEnd"));
	KillZoneCollisionSphere->OnComponentEndOverlap.Add(ScriptDelegate);

	// get player controller
	PlayerController = GetWorld()->GetFirstPlayerController();

	CameraDefaultRotation = SpringArm->GetRelativeRotation();

	//get start time
	PlayerStartTime = FPlatformTime::Seconds();

	InitialPos = GetActorLocation();
	PrevPos = InitialPos;

	// sfx
	if (AudioComponent && StartUpCue)
	{
		AudioComponent->SetSound(StartUpCue);
	}

	/*GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red,  PrevPos.ToString());
	GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red,InitialPos.ToString());
	GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, GetActorLocation().ToString());*/
}

void APlayerMotorCar::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	FlipCar();

	CalculateDistanceTraveled();

	EngineSounds();
	
	if (SpringArm && bIsPlayerDead)
	{
		FRotator NewRotation = SpringArm->GetComponentRotation() + (FRotator(20.0f, 10.0f, 0.0f) * DeltaSeconds);

		// Clamp the pitch and roll rotation 
		float MinPitch = -90.0f;
		float MaxPitch = 0.0f;
		float MinRoll = -45.0f;
		float MaxRoll = 45.0f;
		NewRotation.Pitch = FMath::Clamp(NewRotation.Pitch, MinPitch, MaxPitch);
		NewRotation.Roll = FMath::Clamp(NewRotation.Roll, MinRoll, MaxRoll);

		SpringArm->SetWorldRotation(NewRotation);
	}
	else
	{
		CameraRotation();
	}

	
	
}

void APlayerMotorCar::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Bind different actions to the desired keys in the editor	
	PlayerInputComponent->BindAction("Forward", IE_Pressed, this, &APlayerMotorCar::Accelerate);
	PlayerInputComponent->BindAction("Break", IE_Pressed, this, &APlayerMotorCar::Break);
	PlayerInputComponent->BindAxis("Steer", this, &APlayerMotorCar::Steer);
}
 
//This function increments the gold and aslo retreives the the exp value that 
//The player should get for it. It increments the level if needed and send the info forward via the delegate
//To the UI to update the progress bar
void APlayerMotorCar::HandleGoldCollected()
{
	if (!bIsPlayerDead)
	{
		GoldAmount++;
		//Play SFX sound For gold collected
		if (GoldCollectSoundCue)
		{
		float startTime = FMath::RandRange(0.f, 1.f);
		UGameplayStatics::PlaySoundAtLocation(this, GoldCollectSoundCue, GetActorLocation(), 1.f, 1.f, startTime);
		}

		if (ExpCurveFloat == nullptr) { return; }

	
		float currentValue = ExpCurveFloat->GetFloatValue(GoldAmount);
		int levelTemp = Level;
		Level = FMath::FloorToInt(currentValue);

		//if the player leveled up
		if (Level > levelTemp)
		{
			OnLevelUpDelegate.Broadcast();
		}

		float finalValue = currentValue - Level;

		OnGoldCollectedDelegate.ExecuteIfBound(finalValue);
	}

}

void APlayerMotorCar::HandleRewardCollected(int Reward)
{
	if (!bIsPlayerDead)
	{
		GoldAmount += Reward;

		if (ExpCurveFloat == nullptr) { return; }

		float currentValue = ExpCurveFloat->GetFloatValue(GoldAmount);
		int levelTemp = Level;
		Level = FMath::FloorToInt(currentValue);

		//if the player leveled up
		if (Level > levelTemp)
		{
			OnLevelUpDelegate.Broadcast();
		}

		float finalValue = currentValue - Level;

		OnGoldCollectedDelegate.ExecuteIfBound(finalValue);
	}
}

// this function will allow the player to rotate the camera without re adjusting to a default position
void APlayerMotorCar::CameraRotation()
{
    FRotator DefaultRotation;
    float LerpSpeed = 0.4f;
    float MouseX, MouseY;

    // Get the mouse movement delta from the player's input  
    GetWorld()->GetFirstPlayerController()->GetInputMouseDelta(MouseX, MouseY);

    // Get the current rotation of the SpringArm
    FRotator CurrentRotation;
    if (SpringArm)
    {
	    CurrentRotation = SpringArm->GetRelativeRotation();
    }

    // Update the Z rotation based on the mouse movement  
    float RotationSpeed = 0.8f;
    CurrentRotation.Yaw += MouseX * RotationSpeed;

    // Set the new rotation of the SpringArm
    SpringArm->SetRelativeRotation(CurrentRotation);

	if(bResetCamera)
	{
		if(SpringArm)
		{
			SpringArm->SetRelativeRotation(CameraDefaultRotation);
			bResetCamera = false;
		}
		
	}
	
}

void APlayerMotorCar::FlipCar()
{
	 
	FVector UpVector = GetActorUpVector();
 
	FVector CarLocation = GetActorLocation();  

	FVector StartLocation = CarLocation;  
	FVector EndLocation = CarLocation - FVector(0, 0, 50); 

	FHitResult HitResult; 

	FCollisionQueryParams TraceParams;
	TraceParams.bTraceComplex = false;  
	TraceParams.AddIgnoredActor(this);  

	// Perform the line trace
	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility, TraceParams);

	if ( UpVector.Z < 0 && !bHit)
	{

		GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Black, TEXT("We are flipped!"));
		this->GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		FVector CurrentPos = GetActorLocation();
		FVector NewPos = CurrentPos + FVector(0, 0, 150000);
		FRotator NewRot = FRotator::ZeroRotator;

		this->SetActorRotation(NewRot,ETeleportType::ResetPhysics);
	}

	else
	{
		this->GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
	
}

void APlayerMotorCar::EngineSounds()
{
	if (AudioComponent)
	{
		int32 index;
		int32 thottleInput = GetVehicleMovementComponent()->GetThrottleInput();
		int32 speed = GetVehicleMovementComponent()->GetForwardSpeedMPH() * 1.60934;

		if (thottleInput == 0 && speed < 30)
			index = 1;

		if (thottleInput == 0 && speed > 30)
			index = 3;

		if (thottleInput == 1)
			index = 2;

		if (thottleInput == 1 && speed > 50)
			index = 3;

		if (GetVehicleMovement()->GetBrakeInput() == 1 && speed > 10)
			index = 4;

	 

		 


		/*if (speed == 0 && bPlayerJustSpawn)
		{
			index = 0;
			bPlayerJustSpawn = false;
		}

		if (speed < 20 && GetVehicleMovementComponent()->GetThrottleInput() == 0)
			index = 1;
			

		if (speed > 0 && GetVehicleMovementComponent()->GetThrottleInput() == 1 && speed < 40)
			index = 2;

		if (speed > 40 || speed > 20 && GetVehicleMovementComponent()->GetThrottleInput() == 0)
			index = 3;*/

		AudioComponent->SetIntParameter("indexCue", index);
		//GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, FString::Printf(TEXT("%i"), index));
		//GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, FString::Printf(TEXT("%i"), thottleInput));
		//GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, FString::Printf(TEXT("%s"), AudioComponent->IsPlaying() ? TEXT("true") : TEXT("false")));
		
		if (!AudioComponent->IsPlaying())
		{
			AudioComponent->Play();
		}
		 
	}
}

void APlayerMotorCar::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor)
	{
		IIDamageable* Enemy = Cast<IIDamageable>(OtherActor);
		if (Enemy)
		{
			Enemy->TakeDamge(1000.f);
		}
	}
}

void APlayerMotorCar::Accelerate()
{
    GetVehicleMovementComponent()->SetThrottleInput(1);
}

void APlayerMotorCar::ActivateShield(bool state)
{
	bIsInvinisible = state;
}

void APlayerMotorCar::CalculateDistanceTraveled()
{
	//static FVector PrevPos;
	if (!bIsPlayerDead)
	{
		FVector CurrentPos = GetActorLocation();
		//static FVector PrevPos = CurrentPos;  // static
		//PrevPos = CurrentPos;
		
		float DistanceTraveled = FVector::Dist(PrevPos, CurrentPos);
		TotalDistanceCovered += DistanceTraveled;

		//float DistanceInMeters = TotalDistanceCovered / 100.0f;

		//GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, FString::Printf(TEXT("%.2f meters"), DistanceInMeters));

		PrevPos = CurrentPos;
	}

	else
	{
		TotalDistanceCovered = 0.0f;
	}

 
 
	
}

void APlayerMotorCar::Break()
{
    GetVehicleMovement()->SetBrakeInput(1);
}

void APlayerMotorCar::Steer(float x)
{
    GetVehicleMovement()->SetSteeringInput(x);
}

float APlayerMotorCar::GetPlayerAliveTime()
{
	return FPlatformTime::Seconds() - PlayerStartTime;
	
}

FString APlayerMotorCar::GetFormattedAliveTime()
{
	float TotalSeconds = GetPlayerAliveTime();
	int32 Hours = FMath::FloorToInt(TotalSeconds / 3600);
	int32 Minutes = FMath::FloorToInt((TotalSeconds - (Hours * 3600)) / 60);
	int32 Seconds = FMath::FloorToInt(TotalSeconds - (Hours * 3600) - (Minutes * 60));

	FString FormattedTime = FString::Printf(TEXT("%02d:%02d:%02d"), Hours, Minutes, Seconds);

	SurvivedTimeString = FormattedTime;

	if (TotalSeconds > BestTime)
	{
		BestTime = TotalSeconds;
	}

	int32 HoursBest = FMath::FloorToInt(BestTime / 3600);
	int32 MinutesBest = FMath::FloorToInt((BestTime - (Hours * 3600)) / 60);
	int32 SecondsBest = FMath::FloorToInt(BestTime - (Hours * 3600) - (Minutes * 60));

	FString FormattedBestTime = FString::Printf(TEXT("%02d:%02d:%02d"), HoursBest, MinutesBest, SecondsBest);

	BestTimeString = FormattedBestTime;

	//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("%f"), BestTime));
	return FormattedTime;
}

void APlayerMotorCar::Health(float damage)
{
	if (bIsInvinisible) { return; }

	PlayerHealth = FMath::Max(0, PlayerHealth - damage);

	if (PlayerUI)
	{
		PlayerUI->UpdateHPBar(PlayerHealth / 100);
	}

	if (PlayerHealth == 0)
	{
		PlayerDead();
	}
	 
	//GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Green, FString::Printf(TEXT("HP: %f"), PlayerHealth));
}

void APlayerMotorCar::Heal(float amount)
{
	float healAmount = amount * MAX_HEALTH;
	PlayerHealth = FMath::Min(MAX_HEALTH, PlayerHealth + healAmount);

	if (PlayerUI)
	{
		PlayerUI->UpdateHPBar(PlayerHealth / 100);
	}
}

void APlayerMotorCar::PlayerDead()
{
	if(!bIsPlayerDead)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::White, GetFormattedAliveTime());
		
		
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("PLAYER IS KAPUT"));
		UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.15f);

		DeathWidgetInstance = CreateWidget<UUserWidget>(GetWorld(), DeathWidgetClass);

		if (DeathWidgetInstance)
		{
			// Add the death widget to the viewport
			DeathWidgetInstance->AddToViewport();
		}
		// Disable player input

		PlayerController->DisableInput(PlayerController);

		PlayerController->SetShowMouseCursor(true);
	 
		bIsPlayerDead = true;
	}
	
}
