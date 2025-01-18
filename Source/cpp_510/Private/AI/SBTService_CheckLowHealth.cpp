// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/SBTService_CheckLowHealth.h"
#include "SAttributeComponent.h"
#include "AIModule\Classes\AIController.h"
#include "AIModule\Classes\BehaviorTree\BlackboardComponent.h"

USBTService_CheckLowHealth::USBTService_CheckLowHealth()
{
	LowHealthFraction = 0.3f;
}

void USBTService_CheckLowHealth::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (ensureAlways(BlackboardComp) &&
		ensureAlways(AIController)) {
		APawn* AIPawn = AIController->GetPawn();
		USAttributeComponent* AttributeComp = USAttributeComponent::GetAttributes(AIPawn);
		if (ensureAlways(AttributeComp)) {
			bool bLowHealth = (AttributeComp->GetHealth() / AttributeComp->GetHealthMax()) <= LowHealthFraction;
			BlackboardComp->SetValueAsBool(LowHealthKey.SelectedKeyName, bLowHealth);
		}

	}

}
