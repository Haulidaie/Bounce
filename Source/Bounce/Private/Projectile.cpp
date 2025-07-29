// Fill out your copyright notice in the Description page of Project Settings.

#include "Projectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SphereComponent.h"
#include "ProjectileValues.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
AProjectile::AProjectile()
{
 	// Use a sphere as a simple collision representation
	CollisionComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	CollisionComponent->SetRelativeScale3D(FVector(Scale));
	CollisionComponent->BodyInstance.SetCollisionProfileName("Projectile");
	CollisionComponent->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);

	// Players can't walk on it
	CollisionComponent->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
	CollisionComponent->CanCharacterStepUpOn = ECB_No;

	// Set as root component
	RootComponent = CollisionComponent;

	// Use a ProjectileMovementComponent to govern this projectile's movement
	MovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("MovementComponent"));
	MovementComponent->UpdatedComponent = CollisionComponent;
	MovementComponent->InitialSpeed = 3000.f;
	MovementComponent->MaxSpeed = 3000.f;
	MovementComponent->bRotationFollowsVelocity = true;
	MovementComponent->bShouldBounce = true;

	// Die after 10 seconds by default
	InitialLifeSpan = 10.0f;
}

float AProjectile::GetProjectileDamage()
{
	return Damage;
}

FProjectileValues AProjectile::GetProjectileValues()
{
	return FProjectileValues(Damage, Bounces, MovementComponent->MaxSpeed, MovementComponent->Bounciness, MovementComponent->ProjectileGravityScale, InitialLifeSpan, Scale);
}

void AProjectile::SetProjectileValues(FProjectileValues projectileValues)
{
	Damage = projectileValues.Damage;
	Bounces = projectileValues.Bounces;
	Scale = projectileValues.Scale;

	MovementComponent->InitialSpeed = projectileValues.Speed;
	MovementComponent->MaxSpeed = projectileValues.Speed;
	MovementComponent->Bounciness = projectileValues.Bounciness;
	MovementComponent->ProjectileGravityScale = projectileValues.Gravity;
	SetLifeSpan(projectileValues.Lifetime);
	CollisionComponent->SetRelativeScale3D(projectileValues.Scale);

	ElderlyBounce = Bounces - ElderlyBounce;

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, this, &AProjectile::ChangeProjCollision, HitPlayerDelay, false);
}

void AProjectile::ChangeProjCollision()
{
	FName name = CollisionComponent->GetCollisionProfileName();
	if (name == "ProjectileIgnorePlayer") CollisionComponent->SetCollisionProfileName("Projectile");
	if (Bounces == ElderlyBounce) CollisionComponent->SetCollisionProfileName("OldProjectile");
}

void AProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (BounceSound == nullptr) return;
	Bounces--;
	if (Bounces <= 0) {
		Destroy();
		return;
	}

	ChangeProjCollision();

	if (OtherActor == nullptr) return;
	if (OtherActor == this) return;
	if (OtherComp == nullptr) return;

	FName name = OtherComp->GetCollisionProfileName();

	if (CollisionComponent->GetCollisionProfileName() != "OldProjectile" && (name == "Target" || name == "Pawn" || name == "ChaseTarget")) {
		Destroy();
		return;
	}

	// Play the firing sound
	UGameplayStatics::PlaySoundAtLocation(this, BounceSound, GetActorLocation());
}