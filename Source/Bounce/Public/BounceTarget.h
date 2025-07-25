// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BounceTarget.generated.h"

class ATargetSpawner;
class UBoxComponent;

UCLASS()
class BOUNCE_API ABounceTarget : public AActor
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Target")
	float MaxHealth;

	UPROPERTY(BlueprintReadWrite, Category = "Target")
	int Score = 1;

	UPROPERTY()
	float CurrentHealth;

	UPROPERTY(EditDefaultsOnly, Category = "Target")
	float CorpseTime = 5.f;

	UPROPERTY(BlueprintReadOnly, Category = "Target")
	ATargetSpawner* Spawner;

	UPROPERTY(EditDefaultsOnly, Category = "Target")
	float RagdollForce = 1000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Target")
	float RagdollRandomOffset = 1.1f;

public:	
	/** Box collision component */
	UPROPERTY(BlueprintReadWrite, Category = Target)
	UBoxComponent* CollisionComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* InnerRingMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* OuterRingMesh;

	UPROPERTY(BlueprintReadWrite, Category = Target)
	bool IsDead = false;

	/** Sound to play when we die */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Weapon)
	USoundBase* KillSound;

	/** Sound to play when we die */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Weapon)
	USoundBase* HitSound;

public:
	// Sets default values for this actor's properties
	ABounceTarget();
protected:
	virtual void BeginPlay() override;
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void Death(AActor* OtherActor);
	void Death_Implementation(AActor* OtherActor);
public:
	UFUNCTION()
	void SetSpawnerRef(ATargetSpawner* NewSpawner);

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};
