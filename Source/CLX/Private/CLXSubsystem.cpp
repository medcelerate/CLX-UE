// Fill out your copyright notice in the Description page of Project Settings.


#include "CLXSubsystem.h"
#include "msgpack11.h"

UCLXSubsystem::UCLXSubsystem()
    : ListenSocket(nullptr)
    , ListenIPAddress(TEXT("0.0.0.0"))
    , UDPReceiver(nullptr)
    , ListenPort(3650)
    , bSourcesFlag(false)
{
    // ensure 5 slots: 0=Control…4=Event
    SourceSelection.Init(TEXT(""), 5);
}

UCLXSubsystem::~UCLXSubsystem()
{
    StopListening();
}

void UCLXSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UCLXSubsystem::Deinitialize()
{
    StopListening();
    Super::Deinitialize();
}

bool UCLXSubsystem::StartListening()
{
    if (ListenSocket) return false;

    FIPv4Address Addr;
    if (!FIPv4Address::Parse(ListenIPAddress, Addr))
    {
        UE_LOG(LogTemp, Error, TEXT("[CLX] Invalid IP: %s"), *ListenIPAddress);
        return false;
    }

    ListenSocket = FUdpSocketBuilder(TEXT("CLXListener"))
        .AsNonBlocking()
        .AsReusable()
        .BoundToAddress(Addr)
        .BoundToPort(ListenPort)
        .WithReceiveBufferSize(2 * 1024 * 1024);

    if (!ListenSocket)
    {
        UE_LOG(LogTemp, Error, TEXT("[CLX] Failed to bind %s:%d"), *ListenIPAddress, ListenPort);
        return false;
    }

    UDPReceiver = new FUdpSocketReceiver(
        ListenSocket,
        FTimespan::FromMilliseconds(50),
        TEXT("CLXReceiver")
    );
    UDPReceiver->OnDataReceived().BindUObject(this, &UCLXSubsystem::OnDataReceived);
    UDPReceiver->Start();

    UE_LOG(LogTemp, Log, TEXT("[CLX] Listening on %s:%d"), *ListenIPAddress, ListenPort);
	Announce(); // send initial broadcast announcement
	return true;
}

void UCLXSubsystem::StopListening()
{
    if (UDPReceiver)
    {
        UDPReceiver->Stop();
        delete UDPReceiver;
        UDPReceiver = nullptr;
    }
    if (ListenSocket)
    {
        ListenSocket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ListenSocket);
        ListenSocket = nullptr;
    }
    UE_LOG(LogTemp, Log, TEXT("[CLX] Stopped listening."));
}

void UCLXSubsystem::Announce() {

	if (!ListenSocket) return;
	//Convert the ip address to a broadcast address

    FIPv4Address Addr;
    if (!FIPv4Address::Parse("255.255.255.255", Addr))
    {
        UE_LOG(LogTemp, Error, TEXT("[CLX] Invalid IP: %s"), *ListenIPAddress);
        return;
    }
    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);

    TSharedRef<FInternetAddr> InternetAddr = SocketSubsystem->CreateInternetAddr();
    InternetAddr->SetIp(Addr.Value); // Addr is your FIPv4Address
    InternetAddr->SetPort(7000);


	const char joinPacket[] = { 0x9 };
	int BytesSent = 0;
	ListenSocket->SendTo((uint8*)joinPacket, sizeof(joinPacket), BytesSent, *InternetAddr);

	UE_LOG(LogTemp, Log, TEXT("[CLX] Sent broadcast announcement."));
}

void UCLXSubsystem::OnDataReceived(const FArrayReaderPtr& ArrayReaderPtr, const FIPv4Endpoint& EndPt)
{
    int32 Num = ArrayReaderPtr->Num();
    if (Num < 1) return;

    // copy
    TArray<uint8> Buf;
    Buf.Append((uint8*)ArrayReaderPtr->GetData(), Num);

    // discover sender
    FString Sender = EndPt.Address.ToString();
    int32 Before = DiscoveredSourceSet.Num();
    DiscoveredSourceSet.Add(Sender);
    if (DiscoveredSourceSet.Num() != Before)
    {
        bSourcesFlag = true;
        DiscoveredSources = DiscoveredSourceSet.Array();
    }

    uint8 Header = Buf[0];

    // Cases 0,1,2,4 → msgpack
    if (Header <= 2 || Header == 4)
    {
        std::string Err;
        auto MP = msgpack11::MsgPack::parse(
            reinterpret_cast<char*>(Buf.GetData() + 1),
            Num - 1,
            Err
        );
        if (!Err.empty())
        {
            UE_LOG(LogTemp, Warning, TEXT("[CLX] MsgPack error: %s"), *FString(Err.c_str()));
            return;
        }

        switch (Header)
        {
        case 0:
        {
            FControlPacket C;
            if (ConvertMsgPackToFControl(MP, C))
            {
                    LastControl = C;
                    OnControlMessageReceived.Broadcast(C);
              
            }
            break;
        }
        case 1:
        {
            FDeckPacket D;
            if (ConvertMsgPackToFDeck(MP, D))
            {
                int32 idx = D.DeckIndex;
                OnDeckMessageReceived.Broadcast(D);
            }
            break;
        }
        case 2:
        {
            FMetaPacket M1;
            if (ConvertMsgPackToFMeta(MP, M1))
            {
                int32 idx = M1.Deck;
                OnMetaMessageReceived.Broadcast(M1);
            }
            break;
        }
        case 4:
        {
            FEventPacket E;
            if (ConvertMsgPackToFEvent(MP, E))
            {
                    OnEventMessageReceived.Broadcast(E);
            }
            break;
        }
        }
        return;
    }

    // Case 3: waveform fragmentation
    if (Header == 3)
    {
        // skip in Production
        if (LastControl.AppState == TEXT("Production")) return;
        if (Num < 45) return;

        // 32-byte hash
        char HashBuf[33] = { 0 };
        FMemory::Memcpy(HashBuf, Buf.GetData() + 1, 32);
        FString Hash = ANSI_TO_TCHAR(HashBuf);

        // total (uint64 BE)
        uint64 Total = ReadBigEndian<uint64>(Buf.GetData() + 33);
        // order (uint32 BE)
        uint32 Order = ReadBigEndian<uint32>(Buf.GetData() + 41);
        // fragment
        int32 FragSize = Num - 45;
        TArray<uint8> Frag;
        Frag.Append(Buf.GetData() + 45, FragSize);

        auto& Store = PacketStoreMap.FindOrAdd(Hash);
        if (Store.Fragments.Num() == 0)
        {
            Store.Total = Total;
            Store.Current = 0;
        }
        bool bFound = false;
        for (auto& P : Store.Fragments)
            if (P.Key == Order) { bFound = true; break; }
        if (!bFound)
        {
            Store.Fragments.Add({ Order, Frag });
            Store.Current += Frag.Num();
        }

        if (Store.Current == Store.Total)
        {
            Store.Fragments.Sort([](auto& A, auto& B) { return A.Key < B.Key; });
            TArray<uint8> Full;
            Full.Reserve(Store.Total);
            for (auto& P : Store.Fragments)
                Full.Append(P.Value);

            // write to Saved/ParsingViewer/cache/<hash>
            FString Dir = FPaths::ProjectSavedDir() / TEXT("ParsingViewer/cache");
            IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
            PF.CreateDirectoryTree(*Dir);
            FString Path = Dir / Hash;
            FFileHelper::SaveArrayToFile(Full, *Path);

            PacketStoreMap.Remove(Hash);
            OnWaveformComplete.Broadcast(Path);
        }
    }
}

// … other includes …

bool UCLXSubsystem::ConvertMsgPackToFControl(const msgpack11::MsgPack& msg, FControlPacket& Out)
{
    // msg["UpfaderA"].float64_value() etc.
    Out.UpfaderA = msg["UpfaderA"].float64_value();
    Out.UpfaderB = msg["UpfaderB"].float64_value();
    Out.Crossfader = msg["Crossfader"].float64_value();
    Out.Active = msg["Active"].int32_value();
    Out.AppState = UTF8_TO_TCHAR(msg["AppState"].string_value().c_str());
    return true;
}

bool UCLXSubsystem::ConvertMsgPackToFDeck(const msgpack11::MsgPack& msg, FDeckPacket& Out)
{
    Out.Pitch = msg["Pitch"].float64_value();
    Out.Position = msg["Position"].float64_value();
    Out.Position2 = msg["Position2"].float64_value();
    Out.NormalizedPosition = msg["NormalizedPosition"].float64_value();
    Out.BPM = msg["BPM"].float64_value();
    Out.Length = msg["Length"].float64_value();
    Out.EQLow = msg["EQLow"].float32_value();
    Out.EQMid = msg["EQMid"].float32_value();
    Out.EQHigh = msg["EQHigh"].float32_value();
    Out.DeckIndex = msg["Deck"].int32_value();
    Out.Beat = msg["Beat"].int32_value();    // make sure your FDeckPacket has an 'int32 Beat' UPROPERTY
    return true;
}

bool UCLXSubsystem::ConvertMsgPackToFMeta(const msgpack11::MsgPack& msg, FMetaPacket& Out)
{
    Out.Deck = msg["Deck"].int32_value();
    Out.Title = UTF8_TO_TCHAR(msg["Title"].string_value().c_str());
    Out.Artist = UTF8_TO_TCHAR(msg["Artist"].string_value().c_str());
    Out.Album = UTF8_TO_TCHAR(msg["Album"].string_value().c_str());
    return true;
}

bool UCLXSubsystem::ConvertMsgPackToFEvent(const msgpack11::MsgPack& msg, FEventPacket& Out)
{
    Out.EventName = UTF8_TO_TCHAR(msg["Event"].string_value().c_str());
    Out.Value = msg["Value"].uint8_value();
    return true;
}



