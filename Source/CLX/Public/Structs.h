// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Structs.generated.h"

USTRUCT(BlueprintType)
struct FControlPacket
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "CLX|Control") double UpfaderA;
    UPROPERTY(BlueprintReadWrite, Category = "CLX|Control") double UpfaderB;
    UPROPERTY(BlueprintReadWrite, Category = "CLX|Control") double Crossfader;
    UPROPERTY(BlueprintReadWrite, Category = "CLX|Control") int32  Active;
    UPROPERTY(BlueprintReadWrite, Category = "CLX|Control") FString AppState;

    FControlPacket()
        : UpfaderA(0), UpfaderB(0), Crossfader(0), Active(0), AppState(TEXT(""))
    {
    }
};

/** Case 1: Deck packet */
USTRUCT(BlueprintType)
struct FDeckPacket
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "CLX|Deck") double Pitch;
    UPROPERTY(BlueprintReadWrite, Category = "CLX|Deck") double Position;
    UPROPERTY(BlueprintReadWrite, Category = "CLX|Deck") double Position2;
    UPROPERTY(BlueprintReadWrite, Category = "CLX|Deck") double NormalizedPosition;
    UPROPERTY(BlueprintReadWrite, Category = "CLX|Deck") double BPM;

    UPROPERTY(BlueprintReadWrite, Category = "CLX|Deck") int32 Beat;
    UPROPERTY(BlueprintReadWrite, Category = "CLX|Deck") double Length;

    UPROPERTY(BlueprintReadWrite, Category = "CLX|Deck") float  EQLow;
    UPROPERTY(BlueprintReadWrite, Category = "CLX|Deck") float  EQMid;
    UPROPERTY(BlueprintReadWrite, Category = "CLX|Deck") float  EQHigh;
    UPROPERTY(BlueprintReadWrite, Category = "CLX|Deck") int32  DeckIndex;

    FDeckPacket()
        : Pitch(0), Position(0), Position2(0),
        NormalizedPosition(0), BPM(0), Length(0),
        EQLow(0), EQMid(0), EQHigh(0), DeckIndex(0)
    {
    }
};

/** Case 2: Meta packet */
USTRUCT(BlueprintType)
struct FMetaPacket
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "CLX|Meta") int32  Deck;
    UPROPERTY(BlueprintReadWrite, Category = "CLX|Meta") FString Title;
    UPROPERTY(BlueprintReadWrite, Category = "CLX|Meta") FString Artist;
    UPROPERTY(BlueprintReadWrite, Category = "CLX|Meta") FString Album;

    FMetaPacket()
        : Deck(0), Title(TEXT("")), Artist(TEXT("")), Album(TEXT(""))
    {
    }
};

/** Case 4: Event packet */
USTRUCT(BlueprintType)
struct FEventPacket
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "CLX|Event") FString EventName;
    UPROPERTY(BlueprintReadWrite, Category = "CLX|Event") uint8   Value;

    FEventPacket()
        : EventName(TEXT("")), Value(0)
    {
    }
};