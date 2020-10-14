#include "BabyCharacter_C.h"
#include "Engine/Engine.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "ConstructorHelpers.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Containers/UnrealString.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "BaseGameMode_C.h"
#include "EffectsManager_C.h"
#include "BabyController_C.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"

#include <algorithm>

//*****
// CORE
ABabyCharacter_C::ABabyCharacter_C()
{
	PrimaryActorTick.bCanEverTick = true;

	// Set the class of the bottle that needs to be picked up
	static ConstructorHelpers::FObjectFinder<UClass> sBottleClassFinder(TEXT("/Game/Blueprints/BP_Bottle.BP_Bottle_C"));
	m_BottleClass = sBottleClassFinder.Object;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> sBottleMeshFinder(TEXT("StaticMesh'/Game/Objects/MilkBottle/SM_MilkBottle.SM_MilkBottle'"));
	m_pBottleMesh = sBottleMeshFinder.Object;
	auto bottle = CreateDefaultSubobject<ABottle_C>("Bottle");
	//Mesh and trigger volume
	m_pTagTrigger = CreateDefaultSubobject<UBoxComponent>("TagTrigger");
	m_pTagTrigger->SetupAttachment(RootComponent);
	m_pTouchTrigger = CreateDefaultSubobject<UBoxComponent>("TouchTrigger");
	m_pTouchTrigger->SetupAttachment(RootComponent);

	m_pBottle = CreateDefaultSubobject<UStaticMeshComponent>("BottleMesh");
	m_pBottle->SetStaticMesh(m_pBottleMesh);
	m_pBottle->SetupAttachment(RootComponent);
	m_pBottle->RelativeLocation = FVector(-40.f, 0.f, 0.f);
	m_pBottle->SetVisibility(false);

	//Timer
	m_FartTimer = fmod(m_MaxFartTimer, float(rand() % (int)m_MaxFartTimer));
}

void ABabyCharacter_C::BeginPlay()
{
	Super::BeginPlay();

	ABaseGameMode_C* pGameMode = Cast<ABaseGameMode_C>(GetWorld()->GetAuthGameMode());
	if (pGameMode != nullptr)
	{
		GetMesh()->SetSkeletalMesh(pGameMode->GetPlayerModel());
	}

	// Get all materials from the mesh, create dynamic instances from them, to use later in invisibility
	for (int i = 0; i < GetMesh()->GetMaterials().Num(); ++i)
	{
		m_DynamicMaterialArray.Add(GetMesh()->CreateAndSetMaterialInstanceDynamic(i));
	}

	m_pTagTrigger->OnComponentBeginOverlap.AddDynamic(this, &ABabyCharacter_C::OnOverlapBegin);
	m_pTouchTrigger->OnComponentBeginOverlap.AddDynamic(this, &ABabyCharacter_C::OnOverlapBegin);

	//Get the effects manager
	TArray<AActor*> temp{};
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEffectsManager_C::StaticClass(), temp);
	if (temp.Num()) //Safety
		m_pEffectsManager = Cast<AEffectsManager_C>(temp[0]);

	//AttachFartEffect();
	AttachSleepEffect();

}

void ABabyCharacter_C::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//Double tag
	if (m_DoubletagPrevention)
	{
		m_DoubleTagTimer += DeltaTime;
		if (m_DoubleTagTimer >= m_MaxDoubleTagTimer)
		{
			m_DoubleTagTimer = 0.f;
			m_DoubletagPrevention = false;
		}
	}

#pragma region Effect
	//Fart trigger
	if (TaggingState::Taggee == m_TaggingState && m_TaggingState != TaggingState::Caught)
	{
		//Increment the normal timer
		m_FartTimer += DeltaTime;
		//Activate effect
		if (m_FartTimer >= m_MaxFartTimer)
		{
			m_FartTimer = 0.f;
			AttachFartEffect();

			PlayFart();		
		}
	}
#pragma endregion Effect

	//Bottle reset
	if (m_BottleRefilTimer >= 0.0f)
	{
		m_BottleRefilTimer += DeltaTime;
	}
	if (m_BottleRefilTimer >= m_MaxBottleRefilTimer)
	{
		m_BottleRefilTimer = -1.f;
	}

	//Tagging
	if (m_TouchState == TouchState::Tag)
	{
		m_ExtendTagTimer += DeltaTime;
		if (m_ExtendTagTimer >= m_MaxExtendTagTimer)
		{
			m_TouchState = TouchState::Touch;

			if (!m_JustAttacked)
			{
				m_JustAttacked = false;
			}
			PlayAttack(m_JustAttacked);
			m_JustAttacked = false;
		}
	}
	else
	{
		if (m_ExtendCoolDown < m_MaxExtendCoolDown)
		{
			m_ExtendCoolDown += DeltaTime;
		}
	}

	//Opacity
#pragma region opacity
	// Fix player invisbility glitch
	float movementSpeed = GetVelocity().Size();

	m_Invisibility = FMath::Lerp(m_Invisibility, 1 - movementSpeed / m_MaxCrawlingSpeed, m_CurrentTimeDelta);
	m_CurrentTimeDelta += DeltaTime;

	if (m_CurrentTimeDelta > 1.f)
	{
		m_CurrentTimeDelta = 0;
	}

	if (m_PositionHistory.Num() == m_MaxPositionHistory)
	{
		m_PositionHistory.RemoveAt(0);
	}

	m_CurrentPosition = GetTransform().GetLocation();

	m_PositionHistory.Add(m_CurrentPosition);


	if (FVector::Dist2D(UKismetMathLibrary::GetVectorArrayAverage(m_PositionHistory), m_CurrentPosition) < m_PositionDelta)
	{
		m_Invisibility = std::max(m_Invisibility, 0.7f);
	}

	if (m_TaggingState == TaggingState::Taggee)
	{
		// Set the oppacity of the materials of the mesh
		for (UMaterialInstanceDynamic* pMaterial : m_DynamicMaterialArray)
		{
			if (!pMaterial)
				continue;
			
			if (m_MovementState == MovementState::Idle)
			{
				pMaterial->SetScalarParameterValue("Opacity", 1);
			}
			else
			{
				pMaterial->SetScalarParameterValue("Opacity", m_Invisibility);
			}
		}
	}

	// Update the splatter, if one is present
	if (m_CurrentSplatterPercentage > 0)
	{
		for (UMaterialInstanceDynamic* pMaterial : m_DynamicMaterialArray)
		{
			if (!pMaterial)
				continue;
			
			pMaterial->SetScalarParameterValue("AddedOpacity", m_CurrentSplatterPercentage);
		}
		m_SplatterTimeRemaining -= DeltaTime;

		m_CurrentSplatterPercentage = m_SplatterTimeRemaining / m_MaxSplatterDuration;
	}
#pragma endregion opacity

	//Update movement
#pragma region movement
	if (std::abs(m_AxisVelMult.X) > 0.6f || std::abs(m_AxisVelMult.Y) > 0.6f)
	{
		SetStance(MovementState::Running);

		PlayWalk();
	}
	else if (std::abs(m_AxisVelMult.X) > 0.01f || std::abs(m_AxisVelMult.Y) > 0.01f)
	{
		SetStance(MovementState::Walking);

		PlayCrawl();
	}
	else
		SetStance(MovementState::Idle);

	AddMovement(FVector(m_AxisVelMult, 0.f));

	//Rotation setup
	if (std::abs(m_TempMovementDir.X > 0.f) || std::abs(m_TempMovementDir.Y) > 0.f)
	{
		m_LatestMovementDir = m_TempMovementDir;
	}
	else if (std::abs(m_AxisVelMult.X) > 0.f || std::abs(m_AxisVelMult.Y) > 0.f)
	{
		m_LatestMovementDir = FVector(m_AxisVelMult, 0.f);
	}
	this->SetActorRotation(m_LatestMovementDir.ToOrientationQuat());
#pragma endregion movement
}

void ABabyCharacter_C::AddMovement(const FVector& movement)
{
	AddMovementInput(FVector(1.f, 0.f, 0.f), movement.X);
	AddMovementInput(FVector(0.f, 1.f, 0.f), movement.Y);
}
void ABabyCharacter_C::TeleportToPos(const FVector& pos)
{
	TeleportTo(pos, FRotator(0.f, 0.f, 0.f), false, true);
}

void ABabyCharacter_C::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

//*******
// EVENTS
void ABabyCharacter_C::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (m_TaggingState != TaggingState::Tagger)
		return;

	// Other Actor is the actor that triggered the event. Check that is not ourself or nullptr.  
	if (OtherActor && (OtherActor != this) && OtherComp)
		break;

	if (OtherComp->ComponentHasTag("Trigger"))
		return;

	if (OverlappedComp == m_pTagTrigger && m_TouchState == TouchState::Tag)
	{
		ABabyCharacter_C* pBabyChar = Cast<ABabyCharacter_C>(OtherActor);
		if (pBabyChar)
		{
			if (pBabyChar->GetDoubleTagPrevention())
			{
				return;
			}

			m_TagScore++;
			FRotator direction = FRotator(180.f, 0.f, 0.f);
			FVector offset = FVector(-30,0,-50);
			AttachHitEffect(offset, direction);
			pBabyChar->SetCaught(true);

			Cast<ABabyController_C>(pBabyChar->GetController())->GiveHapticFeedBack();
			Cast<ABabyController_C>(this->GetController())->GiveHapticFeedBack();

			m_JustAttacked = true;
			m_DoubletagPrevention = true;
		}
	}
	else if (OverlappedComp == m_pTouchTrigger && m_TouchState == TouchState::Touch)
	{
		ABabyCharacter_C* pBabyChar = Cast<ABabyCharacter_C>(OtherActor);
		if (pBabyChar)
		{
			if (pBabyChar->GetDoubleTagPrevention())
			{
				return;
			}

			m_TagScore++;
			pBabyChar->SetCaught(true);

			Cast<ABabyController_C>(pBabyChar->GetController())->GiveHapticFeedBack();
			Cast<ABabyController_C>(this->GetController())->GiveHapticFeedBack();

			m_JustAttacked = true;
			m_DoubletagPrevention = true;
		}
	}
}

//*******
// STATES
void ABabyCharacter_C::SetStance(const MovementState preferedState)
{
	m_MovementState = preferedState;
	if (m_MovementState == MovementState::Walking || m_MovementState == MovementState::Idle)
		GetCharacterMovement()->MaxWalkSpeed = m_MaxWalkingSpeed;
	else
		GetCharacterMovement()->MaxWalkSpeed = m_MaxCrawlingSpeed;
}

void ABabyCharacter_C::SetCaught(const bool isCaught)
{
	ABaseGameMode_C* pGameModeCast = Cast<ABaseGameMode_C>(GetWorld()->GetAuthGameMode()); 
	
	//Safety to prevent tagger from getting caught
	if (m_TaggingState == TaggingState::Taggee && isCaught)
	{
		m_TaggingState = TaggingState::Caught;
		// Make sure the baby is visible when it's caught
		for (UMaterialInstanceDynamic* pMaterial : m_DynamicMaterialArray)
		{
			if (!pMaterial)
				continue;
				
			pMaterial->SetScalarParameterValue("Opacity", 1);
		}

		pGameModeCast->HandleTag(this, true);
		
		if (pGameModeCast->GetGameType() == GameType::Cap) 
		{
			//Activate the sleeping effect
			SetSleepEffectVis(true);
			PlaySleeping();
		}
	}
	else if (!isCaught)
	{
		m_TaggingState = TaggingState::Taggee;

		if (pGameModeCast->GetGameType() == GameType::Cap)
		{
			//Deactivate the sleeping effect
			SetSleepEffectVis(false);
		}
	}
}
void ABabyCharacter_C::SetTagger(const bool isTagger)
{
	m_GotTagged = true;

	if (isTagger)
	{
		m_TaggingState = TaggingState::Tagger;

		Cast<UStaticMeshComponent>(GetDefaultSubobjectByName("PillowMesh"))->SetVisibility(true);
		Cast<UStaticMeshComponent>(GetDefaultSubobjectByName("Pacifier"))->SetVisibility(true);
	}
	else 
	{
		m_TaggingState = TaggingState::Taggee;

		Cast<UStaticMeshComponent>(GetDefaultSubobjectByName("PillowMesh"))->SetVisibility(false);
		Cast<UStaticMeshComponent>(GetDefaultSubobjectByName("Pacifier"))->SetVisibility(false);
	}
}

bool ABabyCharacter_C::IsPlayerTagger() const
{
	if (m_TaggingState == TaggingState::Tagger)
	{
		return true;
	}
	return false;
}
bool ABabyCharacter_C::IsPlayerCaught() const
{
	if (m_TaggingState == TaggingState::Caught)
	{
		return true;
	}
	return false;
}

TouchState ABabyCharacter_C::GetTouchState() const
{
	return m_TouchState;
}
float ABabyCharacter_C::GetCurrentSpeed() const
{
	return GetVelocity().Size();
}
float ABabyCharacter_C::GetMaxWalkingSpeed() const
{
	return m_MaxWalkingSpeed;
}
float ABabyCharacter_C::GetMaxCrawlingSpeed() const
{
	return m_MaxCrawlingSpeed;
}

bool ABabyCharacter_C::GetDoubleTagPrevention() const
{
	return m_DoubletagPrevention;
}

//*******
// INPUTS
void ABabyCharacter_C::MoveForward(float axis)
{
	m_AxisVelMult.X = axis;
}
void ABabyCharacter_C::MoveRight(float axis)
{
	m_AxisVelMult.Y = axis;
}
void ABabyCharacter_C::LookForward(float axis)
{
	m_TempMovementDir.X = axis;
}
void ABabyCharacter_C::LookRight(float axis)
{
	m_TempMovementDir.Y = axis;
}
void ABabyCharacter_C::Tag()
{
	if (m_ExtendCoolDown < m_MaxExtendCoolDown && m_TaggingState != TaggingState::Tagger)
		return;

	if (m_TouchState == TouchState::Touch)
	{
		m_TouchState = TouchState::Tag;
		m_ExtendTagTimer = { 0.f };
		m_ExtendCoolDown = { 0.f };

		m_JustAttacked = true;
	}
}
void ABabyCharacter_C::Throw()
{
	if (m_HasBottle && m_TaggingState == TaggingState::Tagger)
	{
		FActorSpawnParameters spawnParams;
		spawnParams.bNoFail = true;
		auto bottle = GetWorld()->SpawnActor<ABottle_C>(m_BottleClass, GetTransform().GetLocation() - FVector{ 100.f, 0.f, 0.f }, FRotator::ZeroRotator, spawnParams);
		bottle->Launch(m_LatestMovementDir);
		m_pBottle->SetVisibility(false);
		m_HasBottle = false;
	}
}
void ABabyCharacter_C::PickupBottle(ABottle_C* bottle)
{
	if (!m_HasBottle)
	{
		m_pBottle->SetVisibility(true);
		m_HasBottle = true;
	}
}

//********
// EFFECTS
UNiagaraComponent* ABabyCharacter_C::AttachEffect(const int id, const FName& socketName, const bool autoDel)
{
	USkeletalMeshComponent* pMesh{ this->GetMesh() };
	UNiagaraSystem* pNiagaraSys{ m_pEffectsManager->GetParticleByID(id) };
	const USkeletalMeshSocket* pSocket{ this->GetMesh()->GetSocketByName(socketName) };

	return UNiagaraFunctionLibrary::SpawnSystemAttached(
		pNiagaraSys, pMesh, socketName,
		pSocket->GetSocketLocation(this->GetMesh()),
		FRotator::ZeroRotator,
		EAttachLocation::Type::SnapToTargetIncludingScale,
		autoDel);
}
void ABabyCharacter_C::AttachFartEffect(const FVector& offset, const FRotator& dir)
{
	//Farting ID = 0, text = Fart
	m_FartActor = AttachEffect(0, "Body_Fart_Effect_Socket", true);
	m_FartActor->SetRelativeLocationAndRotation(offset, dir);
}
void ABabyCharacter_C::AttachSleepEffect(const FVector& offset, const FRotator& dir)
{
	//Farting ID = 1, text = Sleeping
	m_SleepActor = AttachEffect(1, "Head_Sleep_Effect_Socket", false);
	m_SleepActor->SetRelativeLocationAndRotation(offset, dir);

	SetSleepEffectVis(false);
}
void ABabyCharacter_C::AttachHitEffect(const FVector& offset, const FRotator& dir)
{
	//Farting ID = 6, text = Puff
	m_SleepActor = AttachEffect(6, "Socket_Palm", true);
	m_SleepActor->SetRelativeLocationAndRotation(offset, dir);
}
void ABabyCharacter_C::SetSleepEffectVis(bool active)
{
	m_SleepActor->SetVisibility(active);
}
void ABabyCharacter_C::SetSplatter(float percentage)
{
	m_CurrentSplatterPercentage = percentage;
	m_SplatterTimeRemaining = m_MaxSplatterDuration;
}
