// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"

// Sets default values
APlayerCharacter::APlayerCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

}

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}


	ActorsToIgnore.Add(GetOwner());
	ActorsToIgnore.Add(this);
	ActorsToIgnore.Add(nullptr);
}

void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Debugging
	if (CurrentTarget)
	{
		DrawDebugPoint(GetWorld(), CurrentTarget->GetActorLocation(), DebugTargetPointSize, FColor::Blue);
	}

	if (PossibleTarget)
	{
		DrawDebugPoint(GetWorld(), PossibleTarget->GetActorLocation(), DebugTargetPointSize, FColor::Green);
	}

	// Slerps player location towards Target every frame if attacking
	if (bAttacking && CurrentTarget)
	{
		float DistanceToTarget = FVector::Distance(GetActorLocation(), CurrentTarget->GetActorLocation());

		// Player reached target destination
		if (DistanceToTarget < AttackRadius)
		{
			bAttacking = false;
		}

		FVector LerpedLocation = MoveTowards(GetActorLocation(),
			CurrentTarget->GetActorLocation(), (DistanceToTarget * AttackSpeed) * DeltaTime);

		SetActorLocation(LerpedLocation);
	}
	else
	{
		PossibleTarget = UpdatePossibleTarget();
	}
}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Look);

		// Primary Action
		EnhancedInputComponent->BindAction(PrimaryAction, ETriggerEvent::Started, this, &APlayerCharacter::Primary);
		EnhancedInputComponent->BindAction(PrimaryAction, ETriggerEvent::Completed, this, &APlayerCharacter::PrimaryRelease);

	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void APlayerCharacter::Move(const FInputActionValue& Value)
{
	if (bAttacking)
	{
		return;
	}

	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void APlayerCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void APlayerCharacter::Primary()
{
	if (PossibleTarget)
	{
		bAttacking = true;
		SetActorRotation(UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), PossibleTarget->GetActorLocation()));
		CurrentTarget = PossibleTarget;
		ActorsToIgnore.Pop();
		ActorsToIgnore.Add(CurrentTarget);
	}

}

AActor* APlayerCharacter::UpdatePossibleTarget()
{
	FVector FacingDir = FollowCamera->GetForwardVector();
	FacingDir.Z = 0;

	float FacingDotProduct = 0;
	float DotThreshold = DuelDotThreshold;

	if (CurrentTarget)
	{
		// Calculates how much the player is facing the current target
		// 1 = exactly facing | 0 = perpendicular | -1 = exactly facing away
		FacingDotProduct = FVector::DotProduct
		(
			UKismetMathLibrary::Normal(FacingDir),
			UKismetMathLibrary::Normal
			(
				UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), CurrentTarget->GetActorLocation()).Vector()
			)
		);

		// DotThreshold is then affected by distance. The further out the player is to the target,
		// the higher the dot threshold will be.
		float PlayerTargetDistance = FVector::Distance
		(
			UKismetMathLibrary::Normal(GetActorLocation()),
			UKismetMathLibrary::Normal(CurrentTarget->GetActorLocation())
		);

		FMath::Clamp(PlayerTargetDistance, 0, 1);
		DotThreshold += PlayerTargetDistance * DuelDotDistanceScalar;
	}

	// Player is not facing Target
	if (FacingDotProduct < DotThreshold)
	{
		FVector EndLocation = GetActorLocation() + FacingDir * SphereCastDistance;

		FHitResult OutHit;
		const bool Hit = UKismetSystemLibrary::SphereTraceSingle
		(
			GetWorld(), GetActorLocation(), EndLocation,SphereCastRadius,
			UEngineTypes::ConvertToTraceType(ECC_Camera), false,ActorsToIgnore,
			EDrawDebugTrace::None, OutHit,true, FLinearColor::Gray,
			FLinearColor::Green, 0.f
		);

		if (Hit)
		{
			return OutHit.GetActor();
		}
	}

	return nullptr;
}

FVector APlayerCharacter::MoveTowards(FVector Current, FVector Target, float MaxDelta)
{
	FVector A = Target - Current;
	float Mag = A.Size();

	if (Mag <= MaxDelta || Mag == 0.f)
	{
		return Target;
	}

	return Current + A / Mag * MaxDelta;
}
