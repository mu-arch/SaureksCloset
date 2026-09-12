#pragma once
#include <winhttp.h>
#include <shellapi.h>
#include "UpdateState.h"
static UpdateMailbox updateMailbox;
static DWORD updateLastStart=0;
static bool updateStarted=false;
static constexpr wchar_t updateHost[]=L"raw.githubusercontent.com";
static constexpr wchar_t updateManifestPath[]=L"/mu-arch/SaureksCloset/main/update-version.txt";
static constexpr wchar_t updateAddonPath[]=L"/mu-arch/SaureksCloset/main/addon/SaureksCloset/SaureksCloset.toc";
static constexpr wchar_t updateDllPath[]=L"/mu-arch/SaureksCloset/main/native/SaureksCloset.cpp";
struct UpdateHttpHandle {
    HINTERNET value=nullptr;
    ~UpdateHttpHandle(){if(value)WinHttpCloseHandle(value);}
};
// All WinHTTP operations, including handle cleanup, belong to this worker.
// The UI cancels via generation, never closing a synchronous handle in flight.
static unsigned fetchUpdateFile(HINTERNET connection,const wchar_t* path,unsigned ticket,DWORD started,
                               char* buffer,DWORD limit,DWORD& size){
    size=0;
    if(updateMailbox.cancelled(ticket))return ERROR_CANCELLED;
    if(GetTickCount()-started>20000)return ERROR_TIMEOUT;
    UpdateHttpHandle request{WinHttpOpenRequest(connection,L"GET",path,nullptr,WINHTTP_NO_REFERER,
                                              WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE)};
    if(!request.value)return GetLastError();
    DWORD redirects=WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
    DWORD disabled=WINHTTP_DISABLE_COOKIES|WINHTTP_DISABLE_AUTHENTICATION;
    if(!WinHttpSetOption(request.value,WINHTTP_OPTION_REDIRECT_POLICY,&redirects,sizeof(redirects))||
       !WinHttpSetOption(request.value,WINHTTP_OPTION_DISABLE_FEATURE,&disabled,sizeof(disabled)))return GetLastError();
    if(updateMailbox.cancelled(ticket))return ERROR_CANCELLED;
    if(!WinHttpSendRequest(request.value,L"Cache-Control: no-cache\r\n",-1L,WINHTTP_NO_REQUEST_DATA,0,0,0))return GetLastError();
    if(updateMailbox.cancelled(ticket))return ERROR_CANCELLED;
    if(!WinHttpReceiveResponse(request.value,nullptr))return GetLastError();
    DWORD status=0,length=sizeof(status);
    if(!WinHttpQueryHeaders(request.value,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,
                           WINHTTP_HEADER_NAME_BY_INDEX,&status,&length,WINHTTP_NO_HEADER_INDEX))return GetLastError();
    if(status!=200)return 100000+status;
    for(;;){
        if(updateMailbox.cancelled(ticket))return ERROR_CANCELLED;
        if(GetTickCount()-started>20000)return ERROR_TIMEOUT;
        char chunk[1024];DWORD received=0;
        if(!WinHttpReadData(request.value,chunk,sizeof(chunk),&received))return GetLastError();
        if(!received)break;
        if(received>limit-size)return ERROR_INSUFFICIENT_BUFFER;
        std::memcpy(buffer+size,chunk,received);size+=received;
    }
    return 0;
}
static DWORD WINAPI updateWorker(void* parameter){
    const unsigned ticket=static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(parameter));
    UpdateRelease release;unsigned failure=0;DWORD size=0;const DWORD started=GetTickCount();
    {
        UpdateHttpHandle session{WinHttpOpen(L"SaureksCloset-UpdateCheck/1",WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                             WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0)};
        if(!session.value)failure=GetLastError();
        if(!failure&&!WinHttpSetTimeouts(session.value,3000,3000,3000,3000))failure=GetLastError();
        DWORD tls=WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
        if(!failure&&!WinHttpSetOption(session.value,WINHTTP_OPTION_SECURE_PROTOCOLS,&tls,sizeof(tls)))failure=GetLastError();
        UpdateHttpHandle connection;
        if(!failure&&!updateMailbox.cancelled(ticket)){
            connection.value=WinHttpConnect(session.value,updateHost,INTERNET_DEFAULT_HTTPS_PORT,0);
            if(!connection.value)failure=GetLastError();
        }else if(!failure)failure=ERROR_CANCELLED;
        char buffer[65536];
        if(!failure)failure=fetchUpdateFile(connection.value,updateManifestPath,ticket,started,buffer,1024,size);
        if(failure==100404){
            failure=fetchUpdateFile(connection.value,updateAddonPath,ticket,started,buffer,sizeof(buffer),size);
            if(!failure&&!parsePublishedVersion(buffer,size,false,release))failure=ERROR_INVALID_DATA;
            if(!failure)failure=fetchUpdateFile(connection.value,updateDllPath,ticket,started,buffer,sizeof(buffer),size);
            if(!failure&&!parsePublishedVersion(buffer,size,true,release))failure=ERROR_INVALID_DATA;
        }else if(!failure&&!parseUpdateManifest(buffer,size,release))failure=ERROR_INVALID_DATA;
    }
    updateMailbox.finish(ticket,release,failure);
    return 0;
}
static int __fastcall setUpdateChecks(void* L){
    if(!isNumber(L,1))return result(L,-1);
    const double value=toNumber(L,1);if(value!=0&&value!=1)return result(L,-1);
    updateMailbox.enable(value==1);return result(L,1);
}
static int __fastcall startUpdateCheck(void* L){
    if(!updateMailbox.enabled)return result(L,-2);
    if(updateMailbox.running)return result(L,0);
    const DWORD now=GetTickCount();
    if(updateStarted&&now-updateLastStart<60000)return result(L,updateMailbox.status()==2?2:-3);
    unsigned ticket=0;if(!updateMailbox.begin(ticket))return result(L,0);
    // The injected renderer lives for the process lifetime; keep worker code
    // valid even if an external loader attempts to release its module handle.
    HMODULE module=nullptr;
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
                          reinterpret_cast<LPCWSTR>(&updateWorker),&module)){
        updateMailbox.finish(ticket,{},GetLastError());return result(L,-1);
    }
    HANDLE thread=CreateThread(nullptr,0,&updateWorker,reinterpret_cast<void*>(static_cast<std::uintptr_t>(ticket)),0,nullptr);
    if(!thread){updateMailbox.finish(ticket,{},GetLastError());return result(L,-1);}
    CloseHandle(thread);updateLastStart=now;updateStarted=true;return result(L,1);
}
static int __fastcall pollUpdateCheck(void* L){
    const int status=updateMailbox.status();
    if(status!=2&&status!=-1)return result(L,status);
    const auto& r=updateMailbox.release;
    const double values[]={double(status),double(r.major),double(r.minor),double(r.patch),double(r.dll),double(updateMailbox.error)};
    for(auto value:values)pushNumber(L,value);
    return 6;
}
static int __fastcall openWebsite(void* L){
    if(!isNumber(L,1))return result(L,-1);
    const double page=toNumber(L,1);
    const wchar_t* url=nullptr;
    if(page==1)url=L"https://github.com/mu-arch/SaureksCloset";
    if(page==2)url=L"https://github.com/mu-arch/SaureksCloset/releases";
    if(page==3)url=L"https://discord.gg/6mfxCdNbM6";
    if(!url)return result(L,-1);
    // Only an explicit UI click calls this; remote data cannot choose a URL.
    return result(L,reinterpret_cast<std::intptr_t>(ShellExecuteW(nullptr,L"open",url,nullptr,nullptr,SW_SHOWNORMAL))>32?1:0);
}
