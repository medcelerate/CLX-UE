// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Delegates/DelegateCombinations.h"   // ← this line!
#include "Structs.h"
#include "msgpack11.h"

#include "Subsystems/GameInstanceSubsystem.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Networking.h"
#include "IPAddress.h"
#include "CLXSubsystem.generated.h"

// Delegates for cases 0,1,2,4
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnControlMessageReceived, FControlPacket, Control);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeckMessageReceived, FDeckPacket, Deck);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMetaMessageReceived, FMetaPacket, Meta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEventMessageReceived, FEventPacket, Event);

// Delegate for completed waveform reassembly
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWaveformComplete, const FString&, FilePath);

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class CLX_API UCLXSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UCLXSubsystem();
    virtual ~UCLXSubsystem();

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** Bind IP (default "0.0.0.0") */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CLX|Config")
    FString ListenIPAddress;

    /** Bind Port (default 7777) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CLX|Config")
    int32 ListenPort;

    /** [0..4] source filter: 0=Control,1=Deck,2=Meta,3=Waveform,4=Event */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CLX|Config")
    TArray<FString> SourceSelection;

    /** All discovered sender IPs */
    UPROPERTY(BlueprintReadOnly, Category = "CLX|State")
    TArray<FString> DiscoveredSources;

    /** True if DiscoveredSources was updated (reset with ResetSourcesFlag) */
    UPROPERTY(BlueprintReadOnly, Category = "CLX|State")
    bool bSourcesFlag;

    /** Reset the sources flag */
    UFUNCTION(BlueprintCallable, Category = "CLX|API")
    void ResetSourcesFlag() { bSourcesFlag = false; }

    /** Start UDP listening */
    UFUNCTION(BlueprintCallable, Category = "CLX|API")
    bool StartListening();

    /** Stop UDP listening */
    UFUNCTION(BlueprintCallable, Category = "CLX|API")
    void StopListening();

	UFUNCTION(BlueprintCallable, Category = "CLX|API")
    void Announce();

    // ─── Events ───────────────────────────────────────────────────────────

    UPROPERTY(BlueprintAssignable, Category = "CLX|Events")
    FOnControlMessageReceived  OnControlMessageReceived;

    UPROPERTY(BlueprintAssignable, Category = "CLX|Events")
    FOnDeckMessageReceived     OnDeckMessageReceived;

    UPROPERTY(BlueprintAssignable, Category = "CLX|Events")
    FOnMetaMessageReceived     OnMetaMessageReceived;

    UPROPERTY(BlueprintAssignable, Category = "CLX|Events")
    FOnEventMessageReceived    OnEventMessageReceived;

    UPROPERTY(BlueprintAssignable, Category = "CLX|Events")
    FOnWaveformComplete        OnWaveformComplete;

private:
    FSocket* ListenSocket;
    FUdpSocketReceiver* UDPReceiver;

    /** Last received control (for skipping waveform in Production) */
    FControlPacket LastControl;

    /** For discovery/filter */
    TSet<FString> DiscoveredSourceSet;

    /** For waveform fragments */
    struct FPacketStore
    {
        uint64 Total = 0;
        uint64 Current = 0;
        TArray<TPair<uint32, TArray<uint8>>> Fragments;
    };
    TMap<FString, FPacketStore> PacketStoreMap;

    /** Big-endian reader */
    template<typename T>
    static T ReadBigEndian(const uint8* Data)
    {
        T Val = 0;
        for (int i = 0; i < sizeof(T); ++i)
            Val = (Val << 8) | Data[i];
        return Val;
    }

    /** UDP callback */
    void OnDataReceived(const FArrayReaderPtr& ArrayReaderPtr, const FIPv4Endpoint& EndPt);

    /** MsgPack converters */
    bool ConvertMsgPackToFControl(const msgpack11::MsgPack& Root, FControlPacket& Out);
    bool ConvertMsgPackToFDeck(const msgpack11::MsgPack& Root, FDeckPacket& Out);
    bool ConvertMsgPackToFMeta(const msgpack11::MsgPack& Root, FMetaPacket& Out);
    bool ConvertMsgPackToFEvent(const msgpack11::MsgPack& Root, FEventPacket& Out);
};
