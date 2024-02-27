// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "PlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UENUM(BlueprintType)
enum class EAttackType : uint8
{
	Short,
	Far
};

UCLASS(config = Game)
class APlayerCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	/** Primary Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* PrimaryAction;
	
	UPROPERTY(EditAnywhere, Category = "SphereTrace")
	float SphereCastDistance = 5;
		
	UPROPERTY(EditAnywhere, Category = "SphereTrace")
	float SphereCastRadius = 5;

	UPROPERTY(EditAnywhere, Category = "Debug")
	float DebugTargetPointSize = 10;

	TArray<AActor*> ActorsToIgnore;

	AActor* CurrentTarget = nullptr;
	AActor* PossibleTarget = nullptr;

	UPROPERTY(EditAnywhere, Category = "Attack")
	float AttackRadius = 100.f;

	/// <summary> Min dot product to be considered facing an enemy. </summary>
	UPROPERTY(EditAnywhere, Category = "Attack")
	float DuelDotThreshold = .7f;
	
	/// <summary> How much distance affects DuelDotThreshold.</summary>
	UPROPERTY(EditAnywhere, Category = "Attack")
	float DuelDotDistanceScalar = 1.f;

	// Lerp
	float LerpTimeElapsed = 0;
	FVector LerpOrigin;

public:
	// Sets default values for this character's properties
	APlayerCharacter();

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/** Called for Primary input */
	void Primary();
	void PrimaryRelease() {};
	void PrimaryComplete();

	/// <summary> Assigns Possible target depending on Player facing.</summary>
	AActor* UpdatePossibleTarget();

	void MoveTowards(FVector Origin, FVector Target, float Duration, float DeltaTime);

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// To add mapping context
	virtual void BeginPlay();

	virtual void Tick(float DeltaTime);

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const {return FollowCamera;}

	UPROPERTY(BlueprintReadWrite)
	bool bAttacking = false;

	UPROPERTY(BlueprintReadOnly)
	EAttackType AttackType = EAttackType::Short;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	float AttackDuration = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	FTimerHandle AttackTimer;
};
