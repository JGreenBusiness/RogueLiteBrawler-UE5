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

	if (Attacking)
	{
		float DistanceToTarget = FVector::Distance(GetActorLocation(), CurrentTargetLocation);

		if (DistanceToTarget < AttackRadius)
		{
			Attacking = false;

		}

		FVector LerpedLocation = MoveTowards(GetActorLocation(),
			CurrentTargetLocation, (DistanceToTarget * AttackSpeed) * DeltaTime);

		SetActorLocation(LerpedLocation);
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
	if (Attacking)
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

	FVector CameraDir = FollowCamera->GetForwardVector();
	CameraDir.Z = 0;

	FVector StartLocation = GetActorLocation();
	float Dot = FVector::DotProduct(UKismetMathLibrary::Normal(CameraDir)
		, UKismetMathLibrary::Normal(UKismetMathLibrary::FindLookAtRotation(StartLocation,CurrentTargetLocation).Vector()));


	float DotThreshold = DuelDotThreshold;
	
	if (DuelDotDistanceScalar > 1 || DuelDotDistanceScalar < 1)
	{
		float Dist = FVector::Distance(UKismetMathLibrary::Normal(GetActorLocation()), UKismetMathLibrary::Normal(CurrentTargetLocation));
		FMath::Clamp(Dist, 0, 1);
		DotThreshold += Dist * DuelDotDistanceScalar;
		UE_LOG(LogTemp, Warning, TEXT("Dot = %f | DotThresh = %f | Dist = %f"), Dot,DotThreshold, Dist);
	}

	if (Dot > DotThreshold && CurrentTargetLocation != FVector(0))
	{
		return;
	}

	FVector EndLocation = GetActorLocation() + CameraDir * SphereCastDistance;

	FHitResult OutHit;

	const bool DidHit = UKismetSystemLibrary::SphereTraceSingle(GetWorld(), StartLocation, EndLocation,
		SphereCastRadius, UEngineTypes::ConvertToTraceType(ECC_Camera), false, 
		ActorsToIgnore,EDrawDebugTrace::ForDuration, OutHit,
		true, FLinearColor::Gray, FLinearColor::Green, 5.0f);

	if (DidHit)
	{
		Attacking = true;
		AActor* Target = OutHit.GetActor();
		SetActorRotation(UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), Target->GetActorLocation()));
		CurrentTargetLocation = Target->GetActorLocation();
		ActorsToIgnore.Pop();
		ActorsToIgnore.Add(Target);

	}

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
