// Fill out your copyright notice in the Description page of Project Settings.


#include "SPowerup_HealthPotion.h"
#include "SAttributeComponent.h"
#include "SPlayerState.h"
#include "Components/SphereComponent.h"

#define LOCTEXT_NAMESPACE "InteractableActors"

// Sets default values
ASPowerup_HealthPotion::ASPowerup_HealthPotion()
{
	CreditsCost = 50;
}

void ASPowerup_HealthPotion::Interact_Implementation(APawn* InstigatorPawn)
{
	if (!ensure(InstigatorPawn)) {
		return;
	}

	USAttributeComponent* AttributeComp = USAttributeComponent::GetAttributes(InstigatorPawn);
	// Check if not already at max health
	if (ensure(AttributeComp) && !AttributeComp->IsFullHealth()) {
		if (ASPlayerState* PS = InstigatorPawn->GetPlayerState<ASPlayerState>()) {
			if (PS->RemoveCredits(CreditsCost) &&
				AttributeComp->ApplyHealthChange(this, AttributeComp->GetHealthMax())) {
				// Only activate if healed successfully
				HideAndCooldownPowerup();
			}
		}
	}
	
}

FText ASPowerup_HealthPotion::GetInteractText_Implementation(APawn* InstigatorPawn)
{
	TObjectPtr<USAttributeComponent> AttributeComp = USAttributeComponent::GetAttributes(InstigatorPawn);
	if (AttributeComp && AttributeComp->IsFullHealth()) {
		// Namespace Localization Text���ı����ػ���
		// ��һ�����������ƿռ䣻�ڶ���������key���������������ı�����
		return LOCTEXT("HealthPotion_FullHealthWarning", "Already at full health");
	}

	return FText::Format(LOCTEXT("HealthPotion_InteractMessage", "Cost {0} Credits.Restores health to maxinum."), CreditsCost);
}

#undef LOCTEXT_NAMESPACE