#include "VentilationEntrance_C.h"
#include "Components/BoxComponent.h"
#include "BabyCharacter_C.h"
#include "GameFramework/Controller.h"
#include "Containers/Array.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "VentilationExit_C.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "EffectsManager_C.h"
#include "BabyController_C.h"

AVentilationEntrance_C::AVentilationEntrance_C()
{
	PrimaryActorTick.bCanEverTick = true;

	//Change the root to a scene component
	USceneComponent* sceneComponent = CreateDefaultSubobject<USceneComponent>("Root");
	RootComponent = sceneComponent;
	m_pStaticMesh = CreateDefaultSubobject<UStaticMeshComponent>("MeshComponent");
	m_pStaticMesh->SetupAttachment(RootComponent);

	//These triggers handle functionality
	m_pSuctionTrigger = CreateDefaultSubobject<UBoxComponent>("SuctionTrigger");
	m_pSuctionTrigger->SetupAttachment(RootComponent);
	m_pEntranceTrigger = CreateDefaultSubobject<UBoxComponent>("EntranceTrigger");
	m_pEntranceTrigger->SetupAttachment(RootComponent);
}
void AVentilationEntrance_C::OnConstruction(const FTransform& Transform)
{

}

void AVentilationEntrance_C::BeginPlay()
{
	Super::BeginPlay();

	//Gather all exits
	TArray<AActor*> temp;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AVentilationExit_C::StaticClass(), temp);
	if (m_TeleportPoints.Num() != temp.Num())
	{
		m_TeleportPoints.Empty();
		for (int idx{}; idx < temp.Num(); idx++)
		{
			m_TeleportPoints.Add(Cast<AVentilationExit_C>(temp[idx]));
		}
	}

	//Assign all functions for the trigger overlaps
	m_pSuctionTrigger->OnComponentBeginOverlap.AddDynamic(this, &AVentilationEntrance_C::OnOverlapBegin);
	m_pSuctionTrigger->OnComponentEndOverlap.AddDynamic(this, &AVentilationEntrance_C::OnOverlapEnd);
	m_pEntranceTrigger->OnComponentBeginOverlap.AddDynamic(this, &AVentilationEntrance_C::OnOverlapBegin);

	//Get the effects manager
	temp.Empty();
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEffectsManager_C::StaticClass(), temp);
	if (temp.Num()) //Safety
	{
		m_pEffectsManager = Cast<AEffectsManager_C>(temp[0]);
	}

	//Setup the effects
	AttachSuctionEffect();
}

void AVentilationEntrance_C::Tick(float deltaTime)
{
	Super::Tick(deltaTime);

	//Go over all effected players
	for (int idx{}; idx < m_TeleportedBabies.Num(); idx++)
	{
		ABabyCharacter_C* pBaby = m_TeleportedBabies[idx].pBaby;
		//Suck player closer to teleport
		if (m_TeleportedBabies[idx].releaseTime < 0.f)
		{
			FVector direction = this->GetTransform().GetLocation() - pBaby->GetTransform().GetLocation();
			direction.Normalize();
			pBaby->AddMovement(direction * m_SuctionPower);
		}
		else
		{
			//Increase the teleport timer
			m_TeleportedBabies[idx].IncrementTimer(deltaTime);
			//Teleport to random location
			if (m_TeleportedBabies[idx].timer > m_TeleportedBabies[idx].releaseTime)
			{
				int randomSpawnLocIdx = std::rand() % m_TeleportPoints.Num();
				FVector spawnLoc = m_TeleportPoints[randomSpawnLocIdx]->GetTransform().GetLocation();
				m_TeleportedBabies[idx].releaseTime = 0;
				pBaby->TeleportToPos(spawnLoc);
				m_TeleportedBabies.RemoveAt(idx);
			}
		}
	}
}

void AVentilationEntrance_C::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Other Actor is the actor that triggered the event. Check that is not ourself or nullptr.  
	if (OtherActor && (OtherActor != this) && OtherComp)
		return;

	ABabyCharacter_C* pBaby = Cast<ABabyCharacter_C>(OtherActor);
	if (!pBaby)
		return;

	//Pull player closer
	if (OverlappedComp == m_pSuctionTrigger)
	{
		FTeleportedBaby temp = FTeleportedBaby(pBaby, 0.f, -1.f);
		m_TeleportedBabies.Add(temp);
	}
	//Suck the player into vents
	if (OverlappedComp == m_pEntranceTrigger)
	{
		for (int idx{}; idx < m_TeleportedBabies.Num(); idx++)
		{
			if (m_TeleportedBabies[idx].pBaby == pBaby)
			{
				m_TeleportedBabies.Add(FTeleportedBaby(pBaby, 0.f, m_ReleaseTime));
				m_TeleportedBabies.RemoveAt(idx);
				AttachTeleportEffect();
			}
		}

		//Player gets transported for a few sec
		FVector tempPos{ 0.f,0.f,-1000.f };
		pBaby->TeleportToPos(tempPos);
		Cast<ABabyController_C>(pBaby->GetController())->GiveHapticFeedBack();
	}
}

void AVentilationEntrance_C::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	// Other Actor is the actor that triggered the event. Check that is not ourself.  
	if ((OtherActor != nullptr) && (OtherActor != this) && (OtherComp != nullptr))
		return;
	ABabyCharacter_C* pBaby = Cast<ABabyCharacter_C>(OtherActor);
	if (!pBaby) 
		return;
		
	for (int idx{}; idx < m_TeleportedBabies.Num(); idx++)
	{
		if (m_TeleportedBabies[idx].pBaby == pBaby)
		{
			if (m_TeleportedBabies[idx].releaseTime < 0.f)
			{
				m_TeleportedBabies.RemoveAt(idx);
			}
			break;
		}
	}
}

UNiagaraComponent* AVentilationEntrance_C::AttachEffect(const int id, const FName& socketName, const bool autoDel)
{
	UNiagaraSystem* pNiagaraSys{ m_pEffectsManager->GetParticleByID(id) };
	const UStaticMeshSocket* pSocket{ m_pStaticMesh->GetSocketByName(socketName) };
	FTransform transform;
	pSocket->GetSocketTransform(transform, m_pStaticMesh);

	return UNiagaraFunctionLibrary::SpawnSystemAttached(
		pNiagaraSys, m_pStaticMesh, socketName
		, transform.GetLocation()
		, FRotator::ZeroRotator
		, EAttachLocation::Type::SnapToTargetIncludingScale
		, autoDel);
}

void AVentilationEntrance_C::AttachTeleportEffect()
{
	//Farting ID = 4, text = Teleport
	m_TeleportActor = AttachEffect(4, "Center_Socket", true);
	m_TeleportActor->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator(0.f, 0.f, 0.f));
}

void AVentilationEntrance_C::AttachSuctionEffect()
{
	if (!m_ChooseSuctionEffect)
	{
		//Farting ID = 5, text = Suction
		m_SuctionActor = AttachEffect(5, "Center_Socket", false);
	}
	else 
	{
		//Farting ID = 6, text = Suction
		m_SuctionActor = AttachEffect(7, "Center_Socket", false);
	}
	m_SuctionActor->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator(0.f, 0.f, 0.f));
}
