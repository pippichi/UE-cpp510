// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/SBTTask_HealSelf.h"
#include "AIModule\Classes\AIController.h"
#include "SAttributeComponent.h"

EBTNodeResult::Type USBTTask_HealSelf::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (ensureAlways(AIController)) {
		APawn* AIPawn = AIController->GetPawn();
		if (AIPawn == nullptr) {
			return EBTNodeResult::Failed;
		}
		USAttributeComponent* AttributeComp = USAttributeComponent::GetAttributes(AIPawn);
		if (ensureAlways(AttributeComp)) {
			return AttributeComp->ApplyHealthChange(AIPawn, AttributeComp->GetHealthMax()) ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
		}
	}
	return EBTNodeResult::Failed;
}
