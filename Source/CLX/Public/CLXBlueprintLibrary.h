// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CLXSubsystem.h"
#include "CLXBlueprintLibrary.generated.h"

/**
 * 
 */
UCLASS()
class CLX_API UCLXBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
    /**
     *  Get the CLXSubsystem from any Blueprint.
     *  WorldContext is used under the hood to find the GameInstance.
     */
    UFUNCTION(BlueprintCallable, Category = "CLX", meta = (WorldContext = "WorldContextObject"))
    static UCLXSubsystem* GetCLXSubsystem(UObject* WorldContextObject);
	
};
