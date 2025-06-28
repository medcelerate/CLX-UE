// Fill out your copyright notice in the Description page of Project Settings.


#include "CLXBlueprintLibrary.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

UCLXSubsystem* UCLXBlueprintLibrary::GetCLXSubsystem(UObject* WorldContextObject)
{
    if (!WorldContextObject) return nullptr;
    UWorld* World = WorldContextObject->GetWorld();
    if (!World) return nullptr;
    UGameInstance* GI = World->GetGameInstance();
    if (!GI) return nullptr;
    return GI->GetSubsystem<UCLXSubsystem>();
}

