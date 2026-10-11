// Lab-only native PIE regression. Never copied into the release plugin.
#pragma once
#include "CoreMinimal.h"
#include "Components/GMCMotion.h"
#include "Actors/GMCPawn.h"
#include "Ability/GMCAbility.h"
#include "GameFramework/GameModeBase.h"
#include "GMASNativeSpawnFixture.generated.h"
class UCapsuleComponent;
UCLASS()
class UGMASNativeSpawnAbility : public UGMCAbility
{
 GENERATED_BODY()
public:
 UGMASNativeSpawnAbility();
 virtual void BeginAbilityEvent_Implementation() override;
 virtual void TickEvent_Implementation(float DeltaTime) override;
 virtual void EndAbilityEvent_Implementation() override;
 double Started=0;
};
UCLASS()
class UGMASNativeSpawnMovement : public UGMCMotion
{
 GENERATED_BODY()
public:
 virtual void BindReplicationData_Implementation() override;
 virtual void PreLocalMoveExecution_Implementation(const FGMC_Move& Move) override;
 virtual void GenPredictionTick_Implementation(float DeltaTime) override;
 virtual void GenSimulationTick_Implementation(float DeltaTime) override;
 virtual void GenAncillaryTick_Implementation(float DeltaTime,bool Local,bool Combined) override;
};
UCLASS()
class AGMASNativeSpawnPawn : public AGMC_Pawn
{
 GENERATED_BODY()
public:
 AGMASNativeSpawnPawn();
 virtual void BeginPlay() override;
 UPROPERTY() TObjectPtr<UCapsuleComponent> Capsule;
 UPROPERTY() TObjectPtr<UGMASNativeSpawnMovement> Movement;
 UPROPERTY() TObjectPtr<UGMC_AbilitySystemComponent> ASC;
 TSet<int32> Begun,Ended;
 TArray<double> BeginTimes;
 double RequestClock=-1;
 int32 DeferredCount=0;
 bool RequestedAgain=false;
};
UCLASS()
class AGMASNativeSpawnMode : public AGameModeBase
{
 GENERATED_BODY()
public:
 AGMASNativeSpawnMode();
};
