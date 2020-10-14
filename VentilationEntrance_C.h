// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VentilationEntrance_C.generated.h"

USTRUCT(BlueprintType)
struct FTeleportedBaby 
{
	GENERATED_BODY()

	FTeleportedBaby() 
	{
		pBaby = nullptr;
		timer = 0;
		releaseTime = 0;
	}
	FTeleportedBaby(class ABabyCharacter_C* pNewBaby, const float newTimer, const float newReleaseTime)
	{
		pBaby = pNewBaby;
		timer = newTimer;
		releaseTime = newReleaseTime;
	}
	
	void IncrementTimer(const float elapsedTime) {timer += elapsedTime;}

	void SetReleaseTime(const float newReleaseTime) {releaseTime = newReleaseTime;}

	class ABabyCharacter_C* pBaby;
	float timer;
	float releaseTime;
};

UCLASS()
class BEDTIME_BREAKOUT_API AVentilationEntrance_C : public AActor
{
	GENERATED_BODY()

public:
	AVentilationEntrance_C();

	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
		void OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
		void OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	class UNiagaraComponent* AttachEffect(const int id, const FName& socketName, const bool autoDel);
	void AttachTeleportEffect();
	void AttachSuctionEffect();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

private:
	//Effects
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CPPVariables, DisplayName = EffectsManager, meta = (AllowPrivateAccess = "true"))
		class AEffectsManager_C* m_pEffectsManager;
	class UNiagaraComponent* m_TeleportActor = nullptr;
	class UNiagaraComponent* m_SuctionActor = nullptr;

	//Mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = CPPVariables, DisplayName = MeshComponent, meta = (AllowPrivateAccess = "true"))
		class UStaticMeshComponent* m_pStaticMesh;

	//Triggers
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CPPVariables, DisplayName = SuctionBoxTrigger, meta = (AllowPrivateAccess = "true"))
		class UBoxComponent* m_pSuctionTrigger;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CPPVariables, DisplayName = EntranceBoxTrigger, meta = (AllowPrivateAccess = "true"))
		class UBoxComponent* m_pEntranceTrigger;

	//Teleport points
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CPPVariables, DisplayName = TeleportPoints, meta = (AllowPrivateAccess = "true"))
		TArray<class AVentilationExit_C*> m_TeleportPoints;

	//Teleporting babys
	TArray<FTeleportedBaby> m_TeleportedBabies;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CPPVariables, DisplayName = ReleaseTime, meta = (AllowPrivateAccess = "true"))
		float m_ReleaseTime{0.5f};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CPPVariables, DisplayName = SuctionPower, meta = (AllowPrivateAccess = "true"))
		float m_SuctionPower{ 1.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CPPVariables, DisplayName = ChooseSuctionEffect, meta = (AllowPrivateAccess = "true"))
	bool m_ChooseSuctionEffect;
};
