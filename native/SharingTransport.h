#pragma once
#include <winhttp.h>
#include <mutex>
#include <atomic>
#include <memory>
#include "SharingProtocol.h"
struct SharingConfig{
    std::string url,token,server,realm,name,game;std::uint64_t guid=0;unsigned flags=0;
    bool operator==(const SharingConfig& b)const{return url==b.url&&token==b.token&&server==b.server&&realm==b.realm&&name==b.name&&game==b.game&&guid==b.guid&&flags==b.flags;}
};
struct SharingMailbox{
    std::mutex mutex;SharingConfig config;unsigned generation=0;sharing::Debounce debounce;sharing::Inbox inbox;
    std::atomic<bool> fatal{false};std::atomic<int> status{0};std::atomic<DWORD> lastPong{0};bool started=false;
};
static SharingMailbox sharingMailbox;
struct SharingHandle{HINTERNET value=nullptr;~SharingHandle(){if(value)WinHttpCloseHandle(value);}void close(){if(value){WinHttpCloseHandle(value);value=nullptr;}}};
struct SharingSocketApi{
    decltype(&WinHttpWebSocketCompleteUpgrade) upgrade=nullptr;
    decltype(&WinHttpWebSocketSend) send=nullptr;
    decltype(&WinHttpWebSocketReceive) receive=nullptr;
    bool load(){const auto h=GetModuleHandleW(L"winhttp.dll");
        upgrade=reinterpret_cast<decltype(upgrade)>(GetProcAddress(h,"WinHttpWebSocketCompleteUpgrade"));
        send=reinterpret_cast<decltype(send)>(GetProcAddress(h,"WinHttpWebSocketSend"));
        receive=reinterpret_cast<decltype(receive)>(GetProcAddress(h,"WinHttpWebSocketReceive"));
        return upgrade&&send&&receive;
    }
};
static bool sharingGeneration(unsigned epoch){std::lock_guard<std::mutex> lock(sharingMailbox.mutex);return sharingMailbox.generation==epoch;}
static std::wstring sharingWide(const std::string& s){int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),static_cast<int>(s.size()),nullptr,0);if(n<=0)return {};std::wstring out(n,0);MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),static_cast<int>(s.size()),out.data(),n);return out;}
// Asynchronous WinHTTP operations let closing a handle cancel a pending read.
// A synchronous WebSocket receive can remain blocked on Wine after close.
struct SharingAsync {
    enum : unsigned { sent=1,headers=2,read=4,written=8,failed=32 };
    HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr),closedEvent=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    std::atomic<unsigned> flags{0};DWORD length=0;WINHTTP_WEB_SOCKET_BUFFER_TYPE kind{};
    std::array<unsigned char,1024> buffer{};sharing::Bytes outgoing;
    ~SharingAsync(){if(event)CloseHandle(event);if(closedEvent)CloseHandle(closedEvent);}
};
static void CALLBACK sharingCallback(HINTERNET,DWORD_PTR context,DWORD status,void* info,DWORD length){
    auto* c=reinterpret_cast<SharingAsync*>(context);if(!c)return;unsigned flag=0;
    if(status==WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE)flag=SharingAsync::sent;
    else if(status==WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE)flag=SharingAsync::headers;
    else if(status==WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE)flag=SharingAsync::written;
    else if(status==WINHTTP_CALLBACK_STATUS_READ_COMPLETE){
        if(length!=sizeof(WINHTTP_WEB_SOCKET_STATUS)||!info)flag=SharingAsync::failed;
        else {const auto& v=*static_cast<WINHTTP_WEB_SOCKET_STATUS*>(info);c->length=v.dwBytesTransferred;c->kind=v.eBufferType;flag=SharingAsync::read;}
    }else if(status==WINHTTP_CALLBACK_STATUS_REQUEST_ERROR)flag=SharingAsync::failed;
    else if(status==WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING){SetEvent(c->closedEvent);return;}
    if(flag){c->flags.fetch_or(flag);SetEvent(c->event);}
}
static bool sharingWait(SharingAsync& c,unsigned flag,unsigned epoch,DWORD timeout=5000){
    const DWORD start=GetTickCount();
    while(sharingGeneration(epoch)&&GetTickCount()-start<timeout){const auto flags=c.flags.load();if(flags&SharingAsync::failed)return false;if(flags&flag)return true;WaitForSingleObject(c.event,50);}return false;
}
struct SharingAsyncHandle {
    HINTERNET value=nullptr;std::unique_ptr<SharingAsync> context{new SharingAsync};bool armed=false;
    bool arm(){if(!context->event||!context->closedEvent)return false;DWORD_PTR pointer=reinterpret_cast<DWORD_PTR>(context.get());armed=WinHttpSetOption(value,WINHTTP_OPTION_CONTEXT_VALUE,&pointer,sizeof(pointer))!=FALSE;return armed;}
    ~SharingAsyncHandle(){close();}
    void close(){if(!value)return;WinHttpCloseHandle(value);value=nullptr;
        if(!armed)return;
        // Never free buffers still owned by a broken OS callback implementation.
        // Fail closed and stop reconnection instead of accumulating orphan jobs.
        if(WaitForSingleObject(context->closedEvent,5000)!=WAIT_OBJECT_0){context.release();sharingMailbox.fatal=true;sharingMailbox.status=-2;}
    }
};
static bool sharingSend(const SharingSocketApi& api,SharingAsyncHandle& socket,const sharing::Bytes& bytes,unsigned epoch){
    auto& c=*socket.context;c.flags.fetch_and(~SharingAsync::written);c.outgoing=bytes;
    const auto result=api.send(socket.value,WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE,c.outgoing.data(),static_cast<DWORD>(c.outgoing.size()));
    return (result==NO_ERROR||result==ERROR_IO_PENDING)&&sharingWait(c,SharingAsync::written,epoch);
}
static sharing::Bytes sharingHello(const SharingConfig& c){auto b=sharing::message(1);sharing::put(b,c.guid,8);b.push_back(static_cast<unsigned char>(c.flags));for(const auto* s:{&c.server,&c.realm,&c.name,&c.game}){b.push_back(static_cast<unsigned char>(s->size()));b.insert(b.end(),s->begin(),s->end());}return b;}
static bool sharingConnect(const SharingSocketApi& api,const SharingConfig& c,unsigned epoch){
    auto url=sharingWide("https"+c.url.substr(3));URL_COMPONENTS parts{};parts.dwStructSize=sizeof(parts);parts.dwHostNameLength=parts.dwUrlPathLength=parts.dwExtraInfoLength=parts.dwUserNameLength=parts.dwPasswordLength=DWORD(-1);
    if(!WinHttpCrackUrl(url.c_str(),static_cast<DWORD>(url.size()),0,&parts)||parts.nScheme!=INTERNET_SCHEME_HTTPS||parts.dwUserNameLength||parts.dwPasswordLength||parts.dwExtraInfoLength)return false;
    const std::wstring host(parts.lpszHostName,parts.dwHostNameLength),path(parts.lpszUrlPath,parts.dwUrlPathLength);
    SharingHandle session{WinHttpOpen(L"SaureksCloset-Sharing/1",WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,WINHTTP_FLAG_ASYNC)};
    if(!session.value||!WinHttpSetTimeouts(session.value,5000,5000,5000,5000))return false;
    DWORD tls=WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;if(!WinHttpSetOption(session.value,WINHTTP_OPTION_SECURE_PROTOCOLS,&tls,sizeof(tls)))return false;
    SharingHandle connection{WinHttpConnect(session.value,host.c_str(),parts.nPort,0)};if(!connection.value)return false;
    SharingAsyncHandle request;request.value=WinHttpOpenRequest(connection.value,L"GET",path.c_str(),nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE);if(!request.value)return false;
    if(WinHttpSetStatusCallback(request.value,sharingCallback,WINHTTP_CALLBACK_FLAG_ALL_COMPLETIONS|WINHTTP_CALLBACK_FLAG_HANDLES,0)==WINHTTP_INVALID_STATUS_CALLBACK||!request.arm())return false;
    DWORD redirects=WINHTTP_OPTION_REDIRECT_POLICY_NEVER,disable=WINHTTP_DISABLE_COOKIES|WINHTTP_DISABLE_AUTHENTICATION;
    if(!WinHttpSetOption(request.value,WINHTTP_OPTION_REDIRECT_POLICY,&redirects,sizeof(redirects))||!WinHttpSetOption(request.value,WINHTTP_OPTION_DISABLE_FEATURE,&disable,sizeof(disable))||!WinHttpSetOption(request.value,WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET,nullptr,0))return false;
    const auto auth=sharingWide("Authorization: Bearer "+c.token+"\r\n");
    if(!sharingGeneration(epoch))return false;
    if(!WinHttpSendRequest(request.value,auth.c_str(),static_cast<DWORD>(auth.size()),nullptr,0,0,reinterpret_cast<DWORD_PTR>(request.context.get()))&&GetLastError()!=ERROR_IO_PENDING)return false;
    if(!sharingWait(*request.context,SharingAsync::sent,epoch))return false;
    if(!WinHttpReceiveResponse(request.value,nullptr)&&GetLastError()!=ERROR_IO_PENDING)return false;
    if(!sharingWait(*request.context,SharingAsync::headers,epoch))return false;
    DWORD status=0,size=sizeof(status);if(!WinHttpQueryHeaders(request.value,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&status,&size,WINHTTP_NO_HEADER_INDEX)||status!=101)return false;
    SharingAsyncHandle socket;if(!socket.context->event||!socket.context->closedEvent)return false;socket.value=api.upgrade(request.value,reinterpret_cast<DWORD_PTR>(socket.context.get()));socket.armed=socket.value!=nullptr;
    request.close();if(!socket.value||!sharingGeneration(epoch)||!sharingSend(api,socket,sharingHello(c),epoch))return false;
    sharingMailbox.lastPong=GetTickCount();auto& incoming=*socket.context;sharing::Bytes packet;bool readPending=false;
    bool first=true;unsigned sequence=0;std::vector<std::uint64_t> sentVisible;DWORD heartbeat=GetTickCount();
    while(sharingGeneration(epoch)&&GetTickCount()-sharingMailbox.lastPong.load()<90000){
        if(incoming.flags.load()&SharingAsync::failed)break;
        if(readPending&&(incoming.flags.load()&SharingAsync::read)){
            readPending=false;incoming.flags.fetch_and(~SharingAsync::read);
            if((incoming.kind!=WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE&&incoming.kind!=WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE)||incoming.length>incoming.buffer.size()||packet.size()+incoming.length>sharing::maxMessage)break;
            packet.insert(packet.end(),incoming.buffer.begin(),incoming.buffer.begin()+incoming.length);
            if(incoming.kind==WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE){
                std::lock_guard<std::mutex> lock(sharingMailbox.mutex);
                if(sharingMailbox.generation!=epoch||!sharingMailbox.inbox.accept(packet))break;
                if(packet.size()==4&&(packet[3]==129||packet[3]==133)){sharingMailbox.lastPong=GetTickCount();if(packet[3]==129)sharingMailbox.status=2;}
                packet.clear();
            }
        }
        if(!readPending){
            incoming.flags.fetch_and(~SharingAsync::read);
            const auto result=api.receive(socket.value,incoming.buffer.data(),static_cast<DWORD>(incoming.buffer.size()),nullptr,nullptr);
            if(result!=NO_ERROR&&result!=ERROR_IO_PENDING)break;readPending=true;
        }
        sharing::Bytes lookPacket,visiblePacket;
        if(sharingMailbox.status==2){
            {std::lock_guard<std::mutex> lock(sharingMailbox.mutex);if(sharingMailbox.generation!=epoch)break;
                if(first||sharingMailbox.inbox.visible!=sentVisible){sentVisible=sharingMailbox.inbox.visible;visiblePacket=sharing::visible(sentVisible);}
                if((c.flags&1)&&sharingMailbox.debounce.ready(GetTickCount(),!sharingMailbox.debounce.haveSent)){lookPacket=sharing::publish(sharingMailbox.debounce.pending,++sequence);sharingMailbox.debounce.published();}
            }
            if((!visiblePacket.empty()&&!sharingSend(api,socket,visiblePacket,epoch))||(!lookPacket.empty()&&!sharingSend(api,socket,lookPacket,epoch)))break;
            first=false;
            if(GetTickCount()-heartbeat>=30000){if(!sharingSend(api,socket,sharing::message(5),epoch))break;heartbeat=GetTickCount();}
        }
        Sleep(50);
    }
    socket.close();return false;
}
static DWORD WINAPI sharingWorker(void*){
    SharingSocketApi api;if(!api.load()){sharingMailbox.fatal=true;sharingMailbox.status=-2;return 0;}
    unsigned failures=0,previous=0;
    for(;;){SharingConfig config;unsigned epoch;
        {std::lock_guard<std::mutex> lock(sharingMailbox.mutex);config=sharingMailbox.config;epoch=sharingMailbox.generation;}
        if(sharingMailbox.fatal){sharingMailbox.status=config.flags?-2:0;Sleep(100);continue;}
        if(!config.flags||!config.guid){sharingMailbox.status=0;Sleep(100);continue;}
        if(epoch!=previous){failures=0;previous=epoch;}
        if(config.url.empty()||config.token.empty()){sharingMailbox.status=-1;Sleep(100);continue;}
        sharingMailbox.status=1;sharingConnect(api,config,epoch);
        {std::lock_guard<std::mutex> lock(sharingMailbox.mutex);if(sharingMailbox.generation==epoch){sharingMailbox.inbox.looks.clear();sharingMailbox.debounce.haveSent=false;if(sharingMailbox.status!=-2)sharingMailbox.status=3;}}
        const DWORD delay=std::min(30000u,1000u<<std::min(failures++,5u))+(GetTickCount()%500),start=GetTickCount();
        while(sharingGeneration(epoch)&&GetTickCount()-start<delay)Sleep(100);
    }
}
static bool sharingStart(){if(sharingMailbox.started)return true;HMODULE module=nullptr;
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(&sharingWorker),&module))return false;
    HANDLE thread=CreateThread(nullptr,0,sharingWorker,nullptr,0,nullptr);if(!thread)return false;CloseHandle(thread);sharingMailbox.started=true;return true;
}
