// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacter.h"
#include "Projectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/InputDeviceSubsystem.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "EventDispatcher.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/LocalPlayer.h"

DEFINE_LOG_CATEGORY(LogPlayerCharacter);

//////////////////////////////////////////////////////////////////////////
// APlayerCharacter

APlayerCharacter::APlayerCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(CapsuleRadius, CapsuleHeight);
	GetCapsuleComponent()->SetGenerateOverlapEvents(true);

	// Create coyote jump time collider (allows jumping when close to ground but not touching)
	CoyoteCapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CoyoteCollider"));
	CoyoteCapsuleComponent->InitCapsuleSize(CapsuleRadius*4.f, CapsuleRadius*4.f);
	CoyoteCapsuleComponent->SetGenerateOverlapEvents(true);
	CoyoteCapsuleComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CoyoteCapsuleComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Overlap);
	CoyoteCapsuleComponent->SetupAttachment(GetCapsuleComponent());
	CoyoteCapsuleComponent->SetRelativeLocation(FVector(0.f, 0.f, CapsuleHeight*-2.f));
		
	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, CapsuleHeight-10.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));
}

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	CharacterMovement->GetNavAgentPropertiesRef().bCanCrouch = true;
	CharacterMovement->MaxAcceleration = MoveAccelerationWalk;
	CharacterMovement->MaxWalkSpeed = MoveSpeedWalk;
	CharacterMovement->GroundFriction = MoveFrictionGround;
	CharacterMovement->AirControl = MoveFrictionAir;

	CurrentHealth = MaxHealth;
	RespawnPosition = this->GetActorLocation();

	GetCapsuleComponent()->OnComponentHit.AddDynamic(this, &APlayerCharacter::OnHit);
	CoyoteCapsuleComponent->OnComponentBeginOverlap.AddDynamic(this, &APlayerCharacter::OnCoyoteOverlapBegin);
    CoyoteCapsuleComponent->OnComponentEndOverlap.AddDynamic(this, &APlayerCharacter::OnCoyoteOverlapEnd);
}

// Called when hitting the ground
void APlayerCharacter::Landed(const FHitResult &Hit)
{
	Super::Landed(Hit);

	Bounced = false;
}

//////////////////////////////////////////////////////////////////////////// Input

void APlayerCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller)) {
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer())) {
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &APlayerCharacter::Bounce);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Look);

		// Sprinting
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &APlayerCharacter::Sprint);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopSprinting);

		// Sliding
		EnhancedInputComponent->BindAction(SlideAction, ETriggerEvent::Started, this, &APlayerCharacter::Slide);
		EnhancedInputComponent->BindAction(SlideAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopSliding);

		// Paused
		EnhancedInputComponent->BindAction(PauseAction, ETriggerEvent::Started, this, &APlayerCharacter::Pause);
	} else {
		UE_LOG(LogPlayerCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This character uses the Enhanced Input system!"), *GetNameSafe(this));
	}
}

void APlayerCharacter::Move(const FInputActionValue& Value)
{
	if (Controller == nullptr) return;

	FVector2D MovementVector = Value.Get<FVector2D>();
	AddMovementInput(GetActorForwardVector(), MovementVector.Y);
	AddMovementInput(GetActorRightVector(), MovementVector.X);
	CharacterMovement->ApplyImpartedMovementBaseVelocity();
}

void APlayerCharacter::Look(const FInputActionValue& Value)
{
	if (Controller == nullptr) return;

	FVector2D LookAxisVector = Value.Get<FVector2D>();
	AddControllerYawInput(LookAxisVector.X);
	AddControllerPitchInput(LookAxisVector.Y);
}

void APlayerCharacter::Walk()
{
	if(Controller == nullptr) return;

	Sprinting = false;
	Sliding = false;
	CharacterMovement->MaxAcceleration = MoveAccelerationWalk;
	CharacterMovement->MaxWalkSpeed = MoveSpeedWalk;
	CharacterMovement->GroundFriction = MoveFrictionGround;
	CharacterMovement->BrakingDecelerationWalking = 2048.f;
}

void APlayerCharacter::Sprint(const FInputActionValue& Value)
{
	if(Controller == nullptr) return;

	Sprinting = true;
	CharacterMovement->MaxAcceleration = MoveAccelerationSprint;
	CharacterMovement->MaxWalkSpeed = MoveSpeedSprint;
}

void APlayerCharacter::StopSprinting(const FInputActionValue& Value)
{
	if(Controller == nullptr) return;

	Sprinting = false;

	if(Sliding) {
		Slide(true);
	} else {
		Walk();
	}
}

void APlayerCharacter::Slide(const FInputActionValue& Value)
{
	if(Controller == nullptr) return;

	Sliding = true;
	CharacterMovement->MaxAcceleration = MoveAccelerationSlide;
	CharacterMovement->MaxWalkSpeed = MoveSpeedSlide;
	CharacterMovement->GroundFriction = MoveFrictionSlide;
	CharacterMovement->BrakingDecelerationWalking = 1.f;
	GetCapsuleComponent()->SetCapsuleSize(CapsuleRadius, CapsuleHalfed);
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, CapsuleHalfed-10.f));
	APlayerController* PlayerController = Cast<APlayerController>(Controller);
	FRotator rotation = FirstPersonCameraComponent->GetComponentRotation();
	FVector velocity = rotation.RotateVector(FVector(GetVelocity().Length()*0.5, 0.f, -1.f));
	if(GetVelocity().Length() > MoveSpeedSlide) return;

	CharacterMovement->AddImpulse(velocity, true);
}

void APlayerCharacter::StopSliding(const FInputActionValue& Value)
{
	if(Controller == nullptr) return;

	Sliding = false;
	GetCapsuleComponent()->SetCapsuleSize(CapsuleRadius, CapsuleHeight);
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, CapsuleHeight-10.f));

	if(Sprinting) {
		Sprint(true);
	} else {
		Walk();
	}
}

void APlayerCharacter::Bounce(const FInputActionValue& Value)
{
	if(CanJump()) {
		// Normal jumping when in contact with the ground
		Jump();
	} else if(Coyote) {
		// Coyote hang-time using the CoyoteCapsuleComponent as a collider
		CharacterMovement->AddImpulse(FVector(0.f, 0.f, CharacterMovement->JumpZVelocity), true);
		Coyote = false;
	} else if(!Bounced) {
		// Double jump with an extra bounce!
		CharacterMovement->AddImpulse(FVector(0.f, 0.f, CharacterMovement->JumpZVelocity), true);
		Bounced = true;
	}
}

void APlayerCharacter::Pause(const FInputActionValue& Value)
{
	if (Controller == nullptr) return;
	UEventDispatcher::GetEventManagerSingleton()->Event_Pause.Broadcast();
}

void APlayerCharacter::OnHealthUpdate()
{
	UEventDispatcher::GetEventManagerSingleton()->Event_HealthChange.Broadcast(MaxHealth, CurrentHealth);

	if (CurrentHealth > 0) return;
	UEventDispatcher::GetEventManagerSingleton()->Event_GameOver.Broadcast();
}

void APlayerCharacter::SetCurrentHealth(float healthValue)
{
	CurrentHealth = FMath::Clamp(healthValue, 0.f, MaxHealth);
	OnHealthUpdate();
}

float APlayerCharacter::TakeDamage(float DamageTaken, AActor* DamageCauser)
{
	float damageApplied = CurrentHealth - DamageTaken;
	SetCurrentHealth(damageApplied);
	UGameplayStatics::PlaySoundAtLocation(this, DamageSound, GetActorLocation());
	return damageApplied;
}

void APlayerCharacter::Respawn()
{
	CharacterMovement->Velocity = FVector::ZeroVector;
	this->SetActorLocation(RespawnPosition);
}

bool APlayerCharacter::IsPlayerUsingGamePad()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController) return false;

	if (UInputDeviceSubsystem* InputDeviceSubsystem = GEngine->GetEngineSubsystem<UInputDeviceSubsystem>())
	{
		const FPlatformUserId UserId = PlayerController->GetPlatformUserId();
		FHardwareDeviceIdentifier DeviceIdentifier = InputDeviceSubsystem->GetMostRecentlyUsedHardwareDevice(UserId);
		return (DeviceIdentifier.PrimaryDeviceType == EHardwareDevicePrimaryType::Gamepad);
	}
	return false;
}

void APlayerCharacter::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor == nullptr) return;
	if (OtherActor == this) return;
	if (OtherComp == nullptr) return;

	FName name = OtherComp->GetCollisionProfileName();
	if (name != "Projectile" && name != "ProjectileEnemy") return;
	
	if (AProjectile* projectile = Cast<AProjectile>(OtherActor)) {
		TakeDamage(projectile->GetProjectileDamage()+ProjDamageMultiplier-1, OtherActor);
	}
}

void APlayerCharacter::OnCoyoteOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) {
	Coyote = true;
}

void APlayerCharacter::OnCoyoteOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex) {
	Coyote = false;
}