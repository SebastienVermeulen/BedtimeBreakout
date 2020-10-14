#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "Bottle_C.h"
#include "BabyCharacter_C.generated.h"

UENUM(BlueprintType)
enum class TaggingState : uint8
{
	Taggee		UMETA(DisplayName = "Taggee"),
	Tagger		UMETA(DisplayName = "Tagger"),
	Caught		UMETA(DisplayName = "Caught")
};

UENUM(BlueprintType)
enum class MovementState : uint8
{
	Idle = 0 UMETA(DisplayName = "idle"),
	Walking = 1	UMETA(DisplayName = "Walking"),
	Running = 2	UMETA(DisplayName = "Running")
};

UENUM(BlueprintType)
enum class TouchState : uint8
{
	Touch = 0	UMETA(DisplayName = "Touch"),
	Tag = 1	UMETA(DisplayName = "Tag")
};

UCLASS(BlueprintType)
class BEDTIME_BREAKOUT_API ABabyCharacter_C : public ACharacter
{
	GENERATED_BODY()

public:
	ABabyCharacter_C();

	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void AddMovement(const FVector& movement);
	void TeleportToPos(const FVector& pos);

	UFUNCTION()
		void OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	//*******
	// STATES
#pragma region GetterSetter
	void SetStance(const MovementState preferedState);
	void SetCaught(const bool isCaught);
	void SetTagger(const bool isTagger);

	bool IsPlayerTagger() const;
	bool IsPlayerCaught() const;

	TouchState GetTouchState() const;
	float GetCurrentSpeed() const;
	float GetMaxWalkingSpeed() const;
	float GetMaxCrawlingSpeed() const;
	bool GetDoubleTagPrevention() const;
#pragma endregion GetterSetter

	//*******
	// INPUTS
#pragma region Input
	void MoveForward(float axis);
	void MoveRight(float axis);
	void LookForward(float axis);
	void LookRight(float axis);

	void Tag();
	void Throw();
	void PickupBottle(ABottle_C* bottle);
#pragma endregion Input

	//********
	// EFFECTS
#pragma region Effects
	void AttachFartEffect(const FVector& offset = FVector::ZeroVector, const FRotator& dir = FRotator::ZeroRotator);
	void AttachSleepEffect(const FVector& offset = FVector::ZeroVector, const FRotator& dir = FRotator::ZeroRotator);
	void AttachHitEffect(const FVector& offset = FVector::ZeroVector, const FRotator& dir = FRotator::ZeroRotator);
	void SetSleepEffectVis(bool active);
	void SetSplatter(float percentage = 1.f);
#pragma endregion Effects

	UFUNCTION(BlueprintCallable, Category = CPPF)
	virtual int GetPlayerScore() { return m_TagScore + 3 * !m_GotTagged; }
	virtual void SetPlayerScore(int score) { m_TagScore = score; }

	//Sounds
#pragma region Sound
	UFUNCTION(BlueprintImplementableEvent, Category = CPPF)
	void PlaySleeping();
	UFUNCTION(BlueprintImplementableEvent, Category = CPPF)
	void PlayFart();
	bool m_JustAttacked = false;
	UFUNCTION(BlueprintImplementableEvent, Category = CPPF)
	void PlayCry();
	UFUNCTION(BlueprintImplementableEvent, Category = CPPF)
	void PlayAttack(bool attacked);
	UFUNCTION(BlueprintImplementableEvent, Category = CPPF)
	void PlayWalk();
	UFUNCTION(BlueprintImplementableEvent, Category = CPPF)
	void PlayCrawl();
#pragma endregion Sound

protected:
	virtual void BeginPlay() override;


private:
	class UNiagaraComponent* AttachEffect(const int id, const FName& socketName, const bool autoDel);


	//Materials
	TArray<UMaterialInstanceDynamic*> m_DynamicMaterialArray;

	FVector2D m_AxisVelMult{};

	//Effects
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CPPVariables, DisplayName = EffectsManager, meta = (AllowPrivateAccess = "true"))
		class AEffectsManager_C* m_pEffectsManager;

	//Triggers
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CPPVariables, DisplayName = TouchTrigger, meta = (AllowPrivateAccess = "true"))
		UShapeComponent* m_pTouchTrigger;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CPPVariables, DisplayName = TagBoxTrigger, meta = (AllowPrivateAccess = "true"))
		UShapeComponent* m_pTagTrigger;
	
	//Movement
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CPPVariables, DisplayName = MaxWalkingSpeed, meta = (AllowPrivateAccess = "true"))
		float m_MaxWalkingSpeed{ 500.f };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CPPVariables, DisplayName = MaxRunningSpeed, meta = (AllowPrivateAccess = "true"))
		float m_MaxCrawlingSpeed{ 600.f };

	//States
#pragma region StateVars
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CPPVariables, DisplayName = CharacterMovementState, meta = (AllowPrivateAccess = "true"))
		MovementState m_MovementState {MovementState::Idle};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CPPVariables, DisplayName = CharacterTaggingState, meta = (AllowPrivateAccess = "true"))
		TaggingState m_TaggingState {TaggingState::Taggee};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CPPVariables, DisplayName = CharacterTouchState, meta = (AllowPrivateAccess = "true"))
		TouchState m_TouchState{TouchState::Touch};
#pragma endregion StateVars

	// Invisiblility
#pragma region InvisVars
	// Invisiblility glitch fixand rotation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CPPVariables, DisplayName = PositionDelta, meta = (AllowPrivateAccess = "true"))
		float m_PositionDelta{ 20.f };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CPPVariables, DisplayName = MaxPositionHistory, meta = (AllowPrivateAccess = "true"))
		float m_MaxPositionHistory{ 10.f };

	float m_Invisibility; // 0-1;
#pragma endregion InvisVars

	TArray<FVector> m_PositionHistory;
	FVector m_CurrentPosition;
	FVector m_LatestMovementDir;
	FVector m_TempMovementDir;

	//Effects
	class UNiagaraComponent* m_FartActor = nullptr;
	class UNiagaraComponent* m_SleepActor = nullptr;
	class UNiagaraComponent* m_HitActor = nullptr;

	//Timing variables
#pragma region TimerVars
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CPPVariables, DisplayName = MaxExtendTagTimer, meta = (AllowPrivateAccess = "true"))
		float m_MaxExtendTagTimer{1.f};
	float m_ExtendTagTimer{0.f};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CPPVariables, DisplayName = MaxExtendCoolDown, meta = (AllowPrivateAccess = "true"))
		float m_MaxExtendCoolDown{1.f};
	float m_ExtendCoolDown{0.f};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CPPVariables, DisplayName = MaxBottleRefilTimer, meta = (AllowPrivateAccess = "true"))
		float m_MaxBottleRefilTimer{ 30.f };
	float m_BottleRefilTimer{ -1.f }; 
	float m_CurrentTimeDelta;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CPPVariables, DisplayName = MaxFartTimer, meta = (AllowPrivateAccess = "true"))
		float m_MaxFartTimer{30.f};
	float m_FartTimer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CPPVariables, DisplayName = MaxDoubleTagTimer, meta = (AllowPrivateAccess = "true"))
		float m_MaxDoubleTagTimer{ 3.f };
	float m_DoubleTagTimer{ 0.f };
#pragma endregion TimerVars

	bool m_DoubletagPrevention{false};

#pragma region Bottle
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = CPPVariables, DisplayName = BottleClass, meta = (AllowPrivateAccess = "true"))
		TSubclassOf<ABottle_C> m_BottleClass;
	UStaticMeshComponent* m_pBottle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CPPVariables, DisplayName = BottleMesh, meta = (AllowPrivateAccess = "true"))
		UStaticMesh* m_pBottleMesh;

	bool m_HasBottle;
#pragma endregion Bottle

#pragma region Splatter
	float m_CurrentSplatterPercentage{};
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = CPPVariables, DisplayName = MaxSplatterDuration, meta = (AllowPrivateAccess = "true"))
		float m_MaxSplatterDuration{ 5 };

	float m_SplatterTimeRemaining{};

#pragma endregion Splatter

	int m_TagScore{ 0 };
	bool m_GotTagged{ false };
};
