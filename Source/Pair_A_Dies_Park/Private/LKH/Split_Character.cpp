// Copyright Epic Games, Inc. All Rights Reserved.

#include "Split_Character.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemGlobals.h"
#include "CharacterFunctionLibrary.h"
#include "JSW/Weapon/BaseWeapon.h"
#include "Net/UnrealNetwork.h"

//////////////////////////////////////////////////////////////////////////
// ASplit_Character

ASplit_Character::ASplit_Character()
{
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

		// ASC 생성 (Pawn 소유)
	AbilitySystemComp = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComp"));
	AbilitySystemComp->SetIsReplicated(true);
	AbilitySystemComp->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
}

UAbilitySystemComponent* ASplit_Character::GetAbilitySystemComponent() const
{
	return AbilitySystemComp;
}

void ASplit_Character::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// EquippedWeapon 변수를 네트워크 동기화하겠다고 등록
	DOREPLIFETIME(ASplit_Character, EquippedWeapon);

	DOREPLIFETIME(ASplit_Character, bIsAiming);
}

void ASplit_Character::EquipWeapon(ABaseWeapon* NewWeapon)
{
	if (NewWeapon)
	{
		EquippedWeapon = NewWeapon;

		EquippedWeapon->SetOwner(this);

		FName SocketName = TEXT("WeaponSocket");

		if (GetMesh()->DoesSocketExist(SocketName))
		{
			EquippedWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
		}
		else
		{
			EquippedWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("hand_r"));
		}
	}
}

void ASplit_Character::BeginPlay()
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

	if (HasAuthority()) // 서버에서만 Ability 부여
	{
		InitializeAbilities();
	}
	if (FollowCamera)
	{
		DefaultFOV = FollowCamera->FieldOfView; // 원래 설정된 값 가져오기
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void ASplit_Character::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASplit_Character::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASplit_Character::Look);
		// 공격
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &ASplit_Character::Input_Attack);
		// 줌인, 줌아웃
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &ASplit_Character::Input_Aim_Start);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &ASplit_Character::Input_Aim_Stop);
	}
}

void ASplit_Character::Move(const FInputActionValue& Value)
{
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

void ASplit_Character::Look(const FInputActionValue& Value)
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

void ASplit_Character::Server_SetAiming_Implementation(bool bNewState)
{
	bIsAiming = bNewState;

	OnRep_IsAiming();
}

void ASplit_Character::OnRep_IsAiming()
{
	UpdateAimingState();
}

void ASplit_Character::UpdateAimingState()
{
	if (bIsAiming)
	{
		bUseControllerRotationYaw = true;

		GetCharacterMovement()->bOrientRotationToMovement = false;
	}
	else
	{
		bUseControllerRotationYaw = false;

		GetCharacterMovement()->bOrientRotationToMovement = true;
	}
}

// Ability 초기화 함수
void ASplit_Character::InitializeAbilities()
{
	if (bAbilitiesGranted || !AbilitySystemComp) return;

	for (TSubclassOf<UGameplayAbility> AbilityClass : DefaultAbilities)
	{
		if (!AbilityClass) continue;

		FGameplayAbilitySpec Spec(AbilityClass, 1, INDEX_NONE, this);
		AbilitySystemComp->GiveAbility(Spec);
	}

	bAbilitiesGranted = true;
}

void ASplit_Character::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	AbilitySystemComp->InitAbilityActorInfo(this, this);
}

void ASplit_Character::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	AbilitySystemComp->InitAbilityActorInfo(this, this);
}

void ASplit_Character::Input_Attack()
{
	// 무기가 있으면 무기 발사
	if (EquippedWeapon)
	{
		EquippedWeapon->Fire();
	}
}

void ASplit_Character::Input_Aim_Start()
{
	if (EquippedWeapon)
	{
		bIsAiming = true;
		UpdateAimingState();
		Server_SetAiming(true);
	}
}

void ASplit_Character::Input_Aim_Stop()
{
	bIsAiming = false;
	UpdateAimingState();
	Server_SetAiming(false);
}

void ASplit_Character::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (FollowCamera)
	{
		// 목표 FOV 설정
		float TargetFOV = bIsAiming ? AimFOV : DefaultFOV;

		// 현재 FOV에서 목표 FOV로 보간
		float NewFOV = FMath::FInterpTo(FollowCamera->FieldOfView, TargetFOV, DeltaTime, ZoomInterpSpeed);

		FollowCamera->SetFieldOfView(NewFOV);
	}
}

float ASplit_Character::GetAO_Pitch()
{
	FRotator AO_Rot = GetBaseAimRotation();

	float Pitch = AO_Rot.Pitch;

	if (Pitch > 100.0f)
	{
		Pitch -= 360;
	}
	return FMath::Clamp(Pitch, -90.0f, 90.0f);
}
