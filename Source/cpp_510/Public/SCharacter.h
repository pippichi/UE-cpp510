// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UInputMappingContext;
class UInputAction;
class USInteractionComponent;
struct FInputActionValue;
class USAttributeComponent;
class USActionComponent;

UCLASS()
class CPP_510_API ASCharacter : public ACharacter
{
	GENERATED_BODY()

protected:

	/* VisibleAnywhere = read-only, still useful to view in-editor and enforce a convention. */
	UPROPERTY(VisibleAnywhere, Category = "Effects")
	FName TimeToHitParamName;

	//UPROPERTY(VisibleAnywhere, Category = "Effects")
	//FName HandSocketName;

	UPROPERTY(VisibleAnywhere)
	UCameraComponent* CameraComp;

	UPROPERTY(VisibleAnywhere)
	USpringArmComponent* SpringArmComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USAttributeComponent* AttributeComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USActionComponent* ActionComp;

	//FTimerHandle TimerHandle_PrimaryAttack;

	//FTimerHandle TimerHandle_Dash;

	//FTimerHandle TimerHandle_BlackHole;

	// Input
	void OnJump(const FInputActionValue& InputActionValue);
	void OnStopJumping(const FInputActionValue& InputActionValue);
	void TurnAtRate(const FInputActionValue& InputActionValue);
	void LookUpAtRate(const FInputActionValue& InputActionValue);
	void MoveForward(const FInputActionValue& InputActionValue);
	void MoveRight(const FInputActionValue& InputActionValue);
	void OnSpeedup();
	void OnStopSpeedup();

	// Attack
	//void SpawnProjectile(TSubclassOf<AActor> ClassToSpawn);

	void PrimaryInteract();

	void PrimaryAttack();
	//void PrimaryAttack_TimerElapsed();

	void Dash();
	//void Dash_TimerElapsed();

	void BlackHole();
	//void BlackHole_TimerElapsed();

	//void StartAttackEffects();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Input)
	float TurnRateGamepad;

	//UPROPERTY(EditAnywhere, Category = "Attack")
	//TSubclassOf<AActor> ProjectileClass;

	//UPROPERTY(EditAnywhere, Category = "Projectiles")
	//TSubclassOf<AActor> DashProjectileClass;

	//UPROPERTY(EditAnywhere, Category = "Projectiles")
	//TSubclassOf<AActor> BlackHoleProjectileClass;

	//UPROPERTY(EditAnywhere, Category = "Attack")
	//UAnimMontage* AttackAnim;

	//UPROPERTY(EditAnywhere, Category = "Attack")
	//UParticleSystem* CastingEffect;

	UPROPERTY(VisibleAnywhere)
	USInteractionComponent* InteractionComp;

	UFUNCTION()
	void OnHealthChanged(AActor* InstigatorActor, USAttributeComponent* OwningComp, float NewHealth, float Delta);

	virtual void PostInitializeComponents() override;

	virtual FVector GetPawnViewLocation() const override;

public:	

	// Sets default values for this character's properties
	ASCharacter();

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(EXEC)
	void HealSelf(float Amount = 100);

private:
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "EnhancedInput | Context", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> IMC_Action;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "EnhancedInput | Context", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> IMC_MoveBase;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "EnhancedInput | Context", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_MoveForward;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "EnhancedInput | Context", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_MoveRight;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "EnhancedInput | Context", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_TurnRight;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "EnhancedInput | Context", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_LookUp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "EnhancedInput | Context", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_Jump;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "EnhancedInput | Context", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_PrimaryAttack;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "EnhancedInput | Context", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_Speedup;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "EnhancedInput | Context", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_PrimaryInteract;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "EnhancedInput | Context", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_BlackHole;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "EnhancedInput | Context", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_Dash;
};
