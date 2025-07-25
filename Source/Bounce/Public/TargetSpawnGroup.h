// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TargetSpawner.h"
#include "TargetSpawnGroup.generated.h"

class UBoxComponent;

UCLASS()
class BOUNCE_API ATargetSpawnGroup : public AActor
{
	GENERATED_BODY()

protected:
	/*UPROPERTY(EditAnywhere, Category = "Spawners")
	int InitialSpawnAmnt = 10;*/

	UPROPERTY(VisibleDefaultsOnly, Category = Spawners)
	UBoxComponent* PlayerCheckAreaBox;

	UPROPERTY(VisibleDefaultsOnly, Category = Spawners)
	bool PlayerInArea = false;

	UPROPERTY(EditAnywhere, Category = "Spawners")
	int MaxTargets = 15;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawners")
	int StopKillCount = 20;

	int CurrentTargets = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Spawners")
	int KilledTargets = 0;

	UPROPERTY(EditAnywhere, Category = "Spawners")
	float InitialSpawnDelay = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Spawners")
	float SpawnRate = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Spawners")
	float SpawnTimer = InitialSpawnDelay;

	UPROPERTY(EditAnywhere, Category = "Spawners")
	float ResetDelay = 180.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Spawners")
	float ResetTimer;

	UPROPERTY(BlueprintReadOnly, Category = "Spawners")
	bool ResetTimerActive;

	UPROPERTY(EditAnywhere, Category = "Spawners")
	TArray<ATargetSpawner*> TargetSpawners;

public:
	
public:	
	// Sets default values for this actor's properties
	ATargetSpawnGroup();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void SpawnTarget();

	/*UFUNCTION()
	void SpawnInitialTargets();*/

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void ResetSpawners();

	void ResetSpawners_Implementation();

	int GetRandomIndexFromArray(const TArray<ATargetSpawner*>& Array);

	UFUNCTION()
	void OverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void TargetKillHandler();

	void TargetKillHandler_Implementation();
};
