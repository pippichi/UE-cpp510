// Fill out your copyright notice in the Description page of Project Settings.


#include "SItemChest.h"
#include "Components\StaticMeshComponent.h"
#include "Net\UnrealNetwork.h"

// Sets default values
ASItemChest::ASItemChest()
{

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>("BaseComp");
	RootComponent = BaseMesh;
	LidMesh = CreateDefaultSubobject<UStaticMeshComponent>("LidComp");
	LidMesh->SetupAttachment(BaseMesh);

	TargetPitch = 110.0f;

	SetReplicates(true);

}

void ASItemChest::Interact_Implementation(APawn* InstigatorPawn)
{
	bLidOpened = !bLidOpened;
	OnRep_LidOpened(); // ����������Ҫ�ֶ���һ������������ͻ��˲���Ҫ���ͻ��˻��Զ�ִ���������
}

void ASItemChest::OnActorLoaded_Implementation()
{
	OnRep_LidOpened();
}

void ASItemChest::OnRep_LidOpened()
{
	float CurrPitch = bLidOpened ? TargetPitch : 0.0f;
	LidMesh->SetRelativeRotation(FRotator(CurrPitch, 0, 0));
}

void ASItemChest::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASItemChest, bLidOpened);
}
