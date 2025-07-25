// Copyright Epic Games, Inc. All Rights Reserved.


#include "BounceWeaponComponent.h"
#include "BounceCharacter.h"
#include "BounceProjectile.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Animation/AnimInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"

// Sets default values for this component's properties
UBounceWeaponComponent::UBounceWeaponComponent()
{
	// Default offset from the character location for projectiles to spawn
	MuzzleOffset = FVector(100.0f, 0.0f, 10.0f);
	PrimaryComponentTick.bCanEverTick = true;
}

void UBounceWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	FireTimer -= DeltaTime;

	if (!CanShoot && FireTimer < 0.0f)
	{
		CanShoot = true;
	}
}

void UBounceWeaponComponent::Fire()
{
	if (!CanShoot) return;
	if (Character == nullptr || Character->GetController() == nullptr)
	{
		return;
	}

	// Try and fire a projectile
	if (ProjectileClass != nullptr)
	{
		UWorld* const World = GetWorld();
		if (World != nullptr)
		{
			APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
			const FRotator SpawnRotation = PlayerController->PlayerCameraManager->GetCameraRotation();
			// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
			const FVector SpawnLocation = GetOwner()->GetActorLocation() + SpawnRotation.RotateVector(MuzzleOffset);
	
			//Set Spawn Collision Handling Override
			FActorSpawnParameters ActorSpawnParams;
			ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

			// Spawn projectile for each amount in ProjectileAmount
			for (uint8 i = 0; i < ProjectileAmount; ++i)
			{
				// Spawn the projectile at the muzzle
				ABounceProjectile* tempProjectile = World->SpawnActor<ABounceProjectile>(ProjectileClass, SpawnLocation, SpawnRotation + RandDouble(-ShotInnacuracy, ShotInnacuracy), ActorSpawnParams);
				tempProjectile->SetProjectileValues(MaxBounces, Speed, Damage, Bounciness, GravAmount, LifeTime);
			}
			CanShoot = false;
			FireTimer = FireRate;

			PlayerController->AddPitchInput(-RecoilAmount);
			RandomiseValues();
		}
	}
	
	// Try and play the sound if specified
	if (FireSound != nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, Character->GetActorLocation());
	}
	
	// Try and play a firing animation if specified
	if (FireAnimation != nullptr)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Character->GetMesh1P()->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}
}

bool UBounceWeaponComponent::AttachWeapon(ABounceCharacter* TargetCharacter)
{
	Character = TargetCharacter;

	// Check that the character is valid, and has no weapon component yet
	if (Character == nullptr || Character->GetInstanceComponents().FindItemByClass<UBounceWeaponComponent>())
	{
		return false;
	}

	// Attach the weapon to the First Person Character
	FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
	AttachToComponent(Character->GetMesh1P(), AttachmentRules, FName(TEXT("GripPoint")));

	// Set up action bindings
	if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			// Set the priority of the mapping to 1, so that it overrides the Jump action with the Fire action when using touch input
			Subsystem->AddMappingContext(FireMappingContext, 1);
		}

		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
		{
			// Fire
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Triggered, this, &UBounceWeaponComponent::Fire);
		}
	}

	return true;
}

void UBounceWeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// ensure we have a character owner
	if (Character != nullptr)
	{
		// remove the input mapping context from the Player Controller
		if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
			{
				Subsystem->RemoveMappingContext(FireMappingContext);
			}
		}
	}

	// maintain the EndPlay call chain
	Super::EndPlay(EndPlayReason);
}

FRotator UBounceWeaponComponent::RandDouble(float max, float min)
{
	double pitch = static_cast<double>(FMath::FRandRange(min, max));
	double yaw = static_cast<double>(FMath::FRandRange(min, max));
	double roll = static_cast<double>(FMath::FRandRange(min, max));

	return FRotator(pitch, yaw, roll);
}

void UBounceWeaponComponent::RandomiseValues()
{
	ShotInnacuracy = FMath::FRandRange(0.0f, 10.0f);
	ProjectileAmount = (int)FMath::RandRange(1, 10);
	FireRate = FMath::FRandRange(0.0f, 3.0f);
	RecoilAmount = FMath::FRandRange(0.0f, 7.0f);
	MaxBounces = (int)FMath::RandRange(1, 10);
	Speed = FMath::FRandRange(100.0f, 10000.0f);
	Damage = FMath::FRandRange(1.0f, 50.0f);
	Bounciness = FMath::FRandRange(0.1f, 3.0f);
	GravEnabled = FMath::RandBool();
	if (GravEnabled)
	{
		GravAmount = FMath::FRandRange(0.0f, 5.0f);
	}
	else
	{
		GravAmount = 0;
	}
	LifeTime = FMath::FRandRange(0.5f, 25.0f);
}

