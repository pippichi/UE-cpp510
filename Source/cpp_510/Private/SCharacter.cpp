// Fill out your copyright notice in the Description page of Project Settings.


#include "SCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInput/Public/InputActionValue.h"
#include "EnhancedInput/Public/EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInput/Public/InputTriggers.h"
#include "EnhancedInput\Public\EnhancedInputComponent.h"
#include "SInteractionComponent.h"
#include "SActionComponent.h"
#include "SAttributeComponent.h"
#include "Kismet/GameplayStatics.h"
#include "SGameModeBase.h"

// Sets default values
ASCharacter::ASCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	CameraComp = CreateDefaultSubobject<UCameraComponent>("CameraComp");
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>("SpringArmComp");
	SpringArmComp->SetupAttachment(RootComponent);
	CameraComp->SetupAttachment(SpringArmComp);

	SpringArmComp->bUsePawnControlRotation = true;
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	TurnRateGamepad = 5.f;

	InteractionComp = CreateDefaultSubobject<USInteractionComponent>("InteractionComp");

	AttributeComp = CreateDefaultSubobject<USAttributeComponent>("AttributeComp");

	ActionComp = CreateDefaultSubobject<USActionComponent>("ActionComp");

	TimeToHitParamName = "TimeToHit";

	//HandSocketName = "Muzzle_01";

}

void ASCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	AttributeComp->OnHealthChanged.AddDynamic(this, &ASCharacter::OnHealthChanged);
}

FVector ASCharacter::GetPawnViewLocation() const
{
	return CameraComp->GetComponentLocation();
}

void ASCharacter::OnHealthChanged(AActor* InstigatorActor,
	USAttributeComponent* OwningComp,
	float NewHealth,
	float Delta)
{
	if (Delta < 0.0f) {
		GetMesh()->SetScalarParameterValueOnMaterials(TimeToHitParamName, GetWorld()->TimeSeconds);
		
		// Rage added equal to damage received (Abs to turn into positive rage number)
		float RageDelta = FMath::Abs(Delta);
		AttributeComp->ApplyRage(InstigatorActor, RageDelta);
	}
	if (NewHealth <= 0.0f && Delta < 0.0f) {
		APlayerController* PlayerController = Cast<APlayerController>(GetController());
		DisableInput(PlayerController);
		SetLifeSpan(5.0f);
	}
}

void ASCharacter::OnJump(const FInputActionValue& InputActionValue)
{
	Jump();
}

void ASCharacter::OnStopJumping(const FInputActionValue& InputActionValue)
{
	StopJumping();
}

void ASCharacter::TurnAtRate(const FInputActionValue& InputActionValue)
{
	AddControllerYawInput(InputActionValue.GetMagnitude() * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

void ASCharacter::LookUpAtRate(const FInputActionValue& InputActionValue)
{
	AddControllerPitchInput(InputActionValue.GetMagnitude() * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

void ASCharacter::MoveForward(const FInputActionValue& InputActionValue)
{
	float Value = InputActionValue.GetMagnitude();

	if (Controller && Value != 0.0f) {
		 const FRotator Rotation = Controller->GetControlRotation();
		 const FRotator YawRotation(0, Rotation.Yaw, 0);

		 const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		 AddMovementInput(Direction, Value);
	}
}

void ASCharacter::MoveRight(const FInputActionValue& InputActionValue)
{
	float Value = InputActionValue.GetMagnitude();

	if (Controller && Value != 0.0f) {
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(Direction, Value);
	}
}

void ASCharacter::OnStopSpeedup()
{
	ActionComp->StopActionByName(this, "Sprint");
}

void ASCharacter::OnSpeedup()
{
	ActionComp->StartActionByName(this, "Sprint");
}

// Called to bind functionality to input
void ASCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (!ensure(PlayerInputComponent)) {
		return;
	}
	if (APlayerController* PC = CastChecked<APlayerController>(GetController())) {
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer())) {
			Subsystem->AddMappingContext(IMC_Action, 0);
			Subsystem->AddMappingContext(IMC_MoveBase, 0);
		}
	}
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent)) {
		if (IA_MoveForward) {
			EnhancedInputComponent->BindAction(IA_MoveForward, ETriggerEvent::Triggered, this, &ASCharacter::MoveForward);
		}
		if (IA_MoveRight) {
			EnhancedInputComponent->BindAction(IA_MoveRight, ETriggerEvent::Triggered, this, &ASCharacter::MoveRight);
		}
		if (IA_TurnRight) {
			EnhancedInputComponent->BindAction(IA_TurnRight, ETriggerEvent::Triggered, this, &ASCharacter::TurnAtRate);
		}
		if (IA_LookUp) {
			EnhancedInputComponent->BindAction(IA_LookUp, ETriggerEvent::Triggered, this, &ASCharacter::LookUpAtRate);
		}
		if (IA_Jump) {
			EnhancedInputComponent->BindAction(IA_Jump, ETriggerEvent::Started, this, &ASCharacter::OnJump);
			EnhancedInputComponent->BindAction(IA_Jump, ETriggerEvent::Completed, this, &ASCharacter::OnStopJumping);
		}
		if (IA_PrimaryAttack) {
			EnhancedInputComponent->BindAction(IA_PrimaryAttack, ETriggerEvent::Triggered, this, &ASCharacter::PrimaryAttack);
		}
		if (IA_Speedup) {
			EnhancedInputComponent->BindAction(IA_Speedup, ETriggerEvent::Started, this, &ASCharacter::OnSpeedup);
			EnhancedInputComponent->BindAction(IA_Speedup, ETriggerEvent::Completed, this, &ASCharacter::OnStopSpeedup);
		}
		if (IA_PrimaryInteract) {
			EnhancedInputComponent->BindAction(IA_PrimaryInteract, ETriggerEvent::Triggered, this, &ASCharacter::PrimaryInteract);
		}
		if (IA_BlackHole) {
			EnhancedInputComponent->BindAction(IA_BlackHole, ETriggerEvent::Triggered, this, &ASCharacter::BlackHole);
		}
		if (IA_Dash) {
			EnhancedInputComponent->BindAction(IA_Dash, ETriggerEvent::Triggered, this, &ASCharacter::Dash);
		}
	}
}

void ASCharacter::HealSelf(float Amount /* = 100*/)
{
	AttributeComp->ApplyHealthChange(this, Amount);
}

//void ASCharacter::SpawnProjectile(TSubclassOf<AActor> ClassToSpawn)
//{
//	if (ensureAlways(ClassToSpawn)) {
//		const FVector HandLocation = GetMesh()->GetSocketLocation(HandSocketName);
//
//		FVector TraceDirection = GetControlRotation().Vector();
//
//		FVector TraceStart = CameraComp->GetComponentLocation();
//		//FVector TraceStart = GetPawnViewLocation() + (TraceDirection * 20.0f);
//		// endpoint far into the look-at distance (not too far, still adjust somewhat towards crosshair on a miss)
//		FVector TraceEnd = TraceStart + (TraceDirection * 5000);
//
//		FCollisionObjectQueryParams ObjParams;
//		ObjParams.AddObjectTypesToQuery(ECC_WorldDynamic);
//		ObjParams.AddObjectTypesToQuery(ECC_WorldStatic);
//		ObjParams.AddObjectTypesToQuery(ECC_Pawn);
//
//		FCollisionShape Shape;
//		Shape.SetSphere(20.0f);
//
//		// Ignore Player
//		FCollisionQueryParams Params;
//		Params.AddIgnoredActor(this);
//
//		FHitResult Hit;
//		// return true if we got to a blocking hit
//		if (GetWorld()->SweepSingleByObjectType(Hit, TraceStart, TraceEnd, FQuat::Identity, ObjParams, Shape, Params)) {
//			TraceEnd = Hit.ImpactPoint;
//		}
//
//		// find new direction/rotation from Hand pointing to impact point in world
//		FRotator ProjRotation = FRotationMatrix::MakeFromX(TraceEnd - HandLocation).Rotator();
//		FTransform SpawnTM = FTransform(ProjRotation, HandLocation);
//		
//		FActorSpawnParameters SpawnParameters;
//		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
//		SpawnParameters.Instigator = this;
//		GetWorld()->SpawnActor<AActor>(ClassToSpawn, SpawnTM, SpawnParameters);
//	}
//}


void ASCharacter::PrimaryAttack()
{
	//PlayAnimMontage(AttackAnim);

	//GetWorldTimerManager().SetTimer(TimerHandle_PrimaryAttack, this, &ASCharacter::PrimaryAttack_TimerElapsed, 0.2f);
	////GetWorldTimerManager().ClearTimer(TimerHandle_PrimaryAttack);
	ActionComp->StartActionByName(this, "PrimaryAttack");
}

//void ASCharacter::PrimaryAttack_TimerElapsed()
//{
//	//if (ensureAlways(ProjectileClass)) {
//	//	FVector HandLocation = GetMesh()->GetSocketLocation(HandSocketName);
//
//	//	FTransform SpawnTM = FTransform(GetControlRotation(), HandLocation);
//	//	/*FTransform SpawnTM = FTransform(GetControlRotation(), GetActorLocation());*/
//
//	//	FActorSpawnParameters SpawnParams;
//	//	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
//	//	SpawnParams.Instigator = this;
//
//	//	GetWorld()->SpawnActor<AActor>(ProjectileClass, SpawnTM, SpawnParams);
//	//}
//	SpawnProjectile(ProjectileClass);
//}

void ASCharacter::Dash()
{
	/*PlayAnimMontage(AttackAnim);

	GetWorldTimerManager().SetTimer(TimerHandle_Dash, this, &ASCharacter::Dash_TimerElapsed, 0.2f);*/
	ActionComp->StartActionByName(this, "Dash");
}

//void ASCharacter::Dash_TimerElapsed()
//{
//	SpawnProjectile(DashProjectileClass);
//}

void ASCharacter::BlackHole()
{
	/*PlayAnimMontage(AttackAnim);

	GetWorldTimerManager().SetTimer(TimerHandle_BlackHole, this, &ASCharacter::BlackHole_TimerElapsed, 0.2f);*/
	ActionComp->StartActionByName(this, "BlackHole");
}

//void ASCharacter::BlackHole_TimerElapsed()
//{
//	SpawnProjectile(BlackHoleProjectileClass);
//}

//void ASCharacter::StartAttackEffects()
//{
//	PlayAnimMontage(AttackAnim);
//
//	UGameplayStatics::SpawnEmitterAttached(CastingEffect, GetMesh(), HandSocketName, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget);
//}

void ASCharacter::PrimaryInteract()
{
	InteractionComp->PrimaryInteract();
}

