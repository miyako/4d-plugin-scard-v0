/* --------------------------------------------------------------------------------
 #
 #  4DPlugin-SCARD.cpp
 #	source generated by 4D Plugin Wizard
 #	Project : SCARD
 #	author : miyako
 #	2022/01/26
 #  
 # --------------------------------------------------------------------------------*/

#include "4DPlugin-SCARD.h"

/*
 
 mutex protected global object to store status
 
 */

static std::mutex scardMutex;
static Json::Value scardStorage;
static std::map< std::string, std::future<void> >scardTasks;

#pragma mark -

static void scard_clear(std::string& uuid) {
    std::map< std::string, std::future<void> >::iterator it = scardTasks.find(uuid);
    if(it != scardTasks.end()) {
        scardStorage.removeMember(uuid);
        it->second.get();
        scardTasks.erase(it);
    }
}

static void scard_clear_all() {
    for( auto it = scardTasks.begin(); it != scardTasks.end() ; ++it ) {
        std::string uuid = it->first;
        scardStorage.removeMember(uuid);
        it->second.get();
    }
    scardTasks.clear();
}

#pragma mark -

void PluginMain(PA_long32 selector, PA_PluginParameters params) {
    
	try
	{
        switch(selector)
        {
			// --- SCARD
            
			case 1 :
				SCARD_Get_readers(params);
				break;
			case 2 :
				SCARD_Read_tag(params);
				break;
			case 3 :
				SCARD_Get_status(params);
				break;
                
            case kDeinitPlugin :
            case kServerDeinitPlugin :
                scard_clear_all();
                break;
        }

	}
	catch(...)
	{

	}
}

static void print_hex(const uint8_t *pbtData, const size_t szBytes, std::string &hex) {
    
    std::vector<uint8_t> buf((szBytes * 2) + 1);
    memset((char *)&buf[0], 0, buf.size());
    
    for (size_t i = 0; i < szBytes; ++i) {
        sprintf((char *)&buf[i * 2], "%02x", pbtData[i]);
    }
    
    hex = std::string((char *)&buf[0], (szBytes * 2));
}

#pragma mark -

#if VERSIONWIN
void SCARD_Get_readers(PA_PluginParameters params) {

    PA_ObjectRef options = PA_GetObjectParameter(params, 1);

    PA_ObjectRef status = PA_CreateObject();
    PA_CollectionRef readers = PA_CreateCollection();

    bool success = false;
    
    uint32_t scope = SCARD_SCOPE_USER;
    SCARDCONTEXT hContext;
    LONG lResult = SCardEstablishContext(scope, NULL, NULL, &hContext);
    
    if (lResult == SCARD_E_NO_SERVICE) {
        HANDLE hEvent = SCardAccessStartedEvent();
        DWORD dwResult = WaitForSingleObject(hEvent, DEFAULT_TIMEOUT_MS_FOR_RESOURCE_MANAGER);
        if (dwResult == WAIT_OBJECT_0) {
            lResult = SCardEstablishContext(scope, NULL, NULL, &hContext);
        }
        SCardReleaseStartedEvent();
    }
    
    if (lResult == SCARD_S_SUCCESS) {
        
        DWORD len;
        lResult = SCardListReaders(hContext, SCARD_ALL_READERS, NULL, &len);
        if (lResult == SCARD_S_SUCCESS) {
            
            std::vector<TCHAR>buf(len);
            lResult = SCardListReaders(hContext, SCARD_ALL_READERS, &buf[0], &len);
            if (lResult == SCARD_S_SUCCESS) {
                
                LPTSTR pReader = (LPTSTR)&buf[0];
                if (pReader) {
                    while ('\0' != *pReader) {
                        
                        PA_ObjectRef reader = PA_CreateObject();
                        ob_set_a(reader, L"slotName", pReader);
                        pReader = pReader + wcslen(pReader) + 1;
                        
                        PA_Variable v = PA_CreateVariable(eVK_Object);
                        PA_SetObjectVariable(&v, reader);
                        PA_SetCollectionElement(readers, PA_GetCollectionLength(readers), v);
                        PA_ClearVariable(&v);
                    }
                    success = true;
                }
                
            }
            
        }
        SCardReleaseContext(hContext);
    }

    ob_set_b(status, L"success", success);
    ob_set_c(status, L"readers", readers);
    PA_ReturnObject(params, status);
}
#endif

#pragma mark -

#if VERSIONWIN
typedef void (*get_service_t)(SCARDHANDLE hCard,
                              Json::Value *serviceCode,
                              Json::Value *returnValue);
#endif

#if VERSIONWIN
static void get_ServiceData(SCARDHANDLE hCard,
                            const char *serviceName,
                            uint8_t lo, uint8_t hi,
                            unsigned int blockSize,
                            Json::Value *servicesToRead,
                            Json::Value *returnValue) {
    
    Json::Value service(Json::objectValue);
    
    Json::Value data(Json::arrayValue);
    
    service["code"] = serviceName;
    
    uint8_t pbSendBuffer_SelectFile[7] = {
        0xFF,   /* CLA::generic */
        0xA4,   /* INS::selectFile */
        0x00,   /* P1 */
        0x01,   /* P2 */
        0x02,
        hi,   /* service Hi */
        lo    /* service Lo */
    };
    
    int cnt = 0;
    
	DWORD cbRecvLength;
	LONG lResult;

    for (int i = 0; i < blockSize; ++i) {

        uint8_t pbSendBuffer_ReadBinary[5] = {
            0xFF,   /* CLA::generic */
            0xB0,   /* INS::readBinary */
            0x00,
            0x00,   /* block */
            0x00
        };
        
        pbSendBuffer_ReadBinary[3] = i;
        
        cbRecvLength = 2048;

		BYTE pbRecvBuffer[2048];

        lResult = SCardTransmit(hCard,
            SCARD_PCI_T1,
            pbSendBuffer_SelectFile,
            sizeof(pbSendBuffer_SelectFile),
            NULL,
            pbRecvBuffer,
            &cbRecvLength);
        
        if (lResult == SCARD_S_SUCCESS) {
            cbRecvLength = 2048;

            SCARD_IO_REQUEST pci;
            pci.cbPciLength = cbRecvLength;

            lResult = SCardTransmit(hCard,
                SCARD_PCI_T1,
                pbSendBuffer_ReadBinary,
                sizeof(pbSendBuffer_ReadBinary),
                &pci,
                pbRecvBuffer,
                &cbRecvLength);

            if (lResult == SCARD_S_SUCCESS) {
                std::string hex;
                print_hex(pbRecvBuffer, 16, hex);
                data.append(hex);
            }
            
            cnt++;
            
            if(cnt == blockSize){
                
                service["data"] = data;
                returnValue->operator[]("services").append(service);
                
                if(servicesToRead->size() == 0) {

                }else{

                    Json::Value serviceCode;
                    if(servicesToRead->removeIndex(0, &serviceCode)) {
                        std::string name = serviceCode["name"].asString();
                        unsigned int code = serviceCode["code"].asInt();
                        uint8_t lo = code >> 8;
                        uint8_t hi = code & 0x00FF;
                        unsigned int size = serviceCode["size"].asInt();
                        get_ServiceData(hCard, name.c_str(), lo, hi, size, servicesToRead, returnValue);
                    }else{

                    }
                    
                }
                
            }
            
        }
        
    }

}
#endif

#if VERSIONWIN
static void get_m(SCARDHANDLE hCard,
                  const char *mName,
                  const uint8_t *bytes,
                  unsigned int len,
	Json::Value *servicesToRead,
                  Json::Value *returnValue,
                  get_service_t next_call) {
    
    BYTE pbRecvBuffer[2048];
    DWORD cbRecvLength;
    BYTE SW1 = 0;
    BYTE SW2 = 0;
    
    cbRecvLength = 2048;
    LONG lResult = SCardTransmit(hCard,
                            SCARD_PCI_T1,
                            bytes,
                            len,
                            NULL,
                            pbRecvBuffer,
                            &cbRecvLength);
    
    switch(lResult)
    {
        case SCARD_S_SUCCESS:
            SW1 = pbRecvBuffer[cbRecvLength - 2];
            SW2 = pbRecvBuffer[cbRecvLength - 1];
            if ( SW1 != 0x90 || SW2 != 0x00 )
            {
                if ( SW1 == 0x63 && SW2 == 0x00 )
                {
                    /* data is not available */
                }
            }
            else
            {
                std::string m;
                print_hex((const uint8_t *)pbRecvBuffer, 8, m);
                
                returnValue->operator[](mName) = m;
                
                if(next_call == NULL) {
                    
                    if(servicesToRead->size() == 0) {

                    }else{
                       
                        Json::Value serviceCode;
                        if(servicesToRead->removeIndex(0, &serviceCode)) {
                            std::string name = serviceCode["name"].asString();
                            unsigned int code = serviceCode["code"].asInt();
                            uint8_t lo = code >> 8;
                            uint8_t hi = code & 0x00FF;
                            unsigned int size = serviceCode["size"].asInt();
                            get_ServiceData(hCard, name.c_str(), lo, hi, size, servicesToRead, returnValue);
                        }else{

                        }

                    }
                }else{

                    (*next_call)(hCard, servicesToRead, returnValue);
                }
            }
            break;
        case 0x458:
            lResult = SCARD_W_REMOVED_CARD;
            break;
        case 0x16:
            lResult = SCARD_E_INVALID_PARAMETER;
            break;
        default:
            break;
    }
    
}
#endif

#if VERSIONWIN
static void get_PMm(SCARDHANDLE hCard,
                    Json::Value *servicesToRead,
                    Json::Value *returnValue) {
    
    uint8_t bytes[5] = {
        0xFF,   /* CLA::generic */
        0xCA,   /* INS::getData */
        0x01,   /* P1::getPMm */
        0x00,   /* P2::none */
        0x00    /* LE::maxLength */
    };
    
    get_m(hCard, "PMm", bytes, 5, servicesToRead, returnValue, NULL);
}

static void get_IDm(SCARDHANDLE hCard,
                    Json::Value *servicesToRead,
                    Json::Value *returnValue) {
    
    uint8_t bytes[5] = {
        0xFF,   /* CLA::generic */
        0xCA,   /* INS::getData */
        0x00,   /* P1::getIDm */
        0x00,   /* P2::none */
        0x00    /* LE::maxLength */
    };
    
    get_m(hCard, "IDm", bytes, 5, servicesToRead, returnValue, get_PMm);
    
}
#endif

static void generateUuid(std::string &uuid) {
    
#if VERSIONWIN
    RPC_WSTR str;
    UUID uid;
    if (UuidCreate(&uid) == RPC_S_OK) {
        if (UuidToString(&uid, &str) == RPC_S_OK) {
            size_t len = wcslen((const wchar_t *)str);
            std::vector<wchar_t>buf(len+1);
            memcpy(&buf[0], str, len * sizeof(wchar_t));
            _wcsupr((wchar_t *)&buf[0]);
            CUTF16String wstr = CUTF16String((const PA_Unichar *)&buf[0], len);
            u16_to_u8(wstr, uuid);
            RpcStringFree(&str);
        }
    }
#else
    NSString *u = [[[NSUUID UUID]UUIDString]stringByReplacingOccurrencesOfString:@"-" withString:@""];
    uuid = [u UTF8String];
#endif
}

#if VERSIONWIN
void SCARD_Read_tag(PA_PluginParameters params) {
    
    PA_ObjectRef status = PA_CreateObject();
    PA_ObjectRef args = PA_GetObjectParameter(params, 1);
    
    std::string slotName;
    
    Json::Value serviceCodes(Json::arrayValue);
    
    if (args) {
        CUTF8String stringValue;
        if (ob_get_s(args, L"slotName", &stringValue)) {
            slotName = (const char *)stringValue.c_str();
        }
        
        PA_CollectionRef collectionValue = ob_get_c(args, L"services");
        if(collectionValue) {
            PA_long32 len = PA_GetCollectionLength(collectionValue);
            for(PA_long32 i = 0; i < len; ++i) {
                PA_Variable v = PA_GetCollectionElement(collectionValue, i);
                if(PA_GetVariableKind(v) == eVK_Object) {
                    PA_ObjectRef o = PA_GetObjectVariable(v);
                    if(o) {
                        CUTF8String stringValue;
                        if(ob_get_s(o, L"code", &stringValue)) {
                            std::string name = (const char *)stringValue.c_str();
                            if(strlen((const char *)name.c_str()) == 4) {
                                int code = (int)strtol(name.c_str(), NULL, 16);
                                unsigned int size = ob_get_n(o, L"size");
                                if(size) {
                                    Json::Value serviceCode(Json::objectValue);
                                    serviceCode["name"] = name;
                                    serviceCode["code"] = code;
                                    serviceCode["size"] = size;
                                    serviceCodes.append(serviceCode);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    std::string uuid;
    generateUuid(uuid);
    
    Json::Value threadCtx(Json::objectValue);
    
    if(1) {
        
        std::lock_guard<std::mutex> lock(scardMutex);
        
        threadCtx["complete"] = false;
        threadCtx["success"] = false;
        threadCtx["errorMessage"] = "";
        threadCtx["slotName"] = "";
        threadCtx["protocol"] = "";
        threadCtx["IDm"] = "";
        threadCtx["PMm"] = "";
        
        scardStorage[uuid] = threadCtx;
    }
    
    auto func = [](std::string uuid, std::string slotName, Json::Value serviceCodes) {
        
        std::string errorMessage;
        
        Json::Value returnValue(Json::objectValue);
        Json::Value servicesToRead(Json::arrayValue);
        
        servicesToRead.copy(serviceCodes);
        
        LPTSTR lpszReaderName = NULL;
        CUTF16String name;
        DWORD protocols = SCARD_PROTOCOL_T0|SCARD_PROTOCOL_T1;
        DWORD mode = SCARD_SHARE_SHARED;
        DWORD scope = SCARD_SCOPE_USER;
        
        u8_to_u16(slotName, name);
        
        lpszReaderName = (LPTSTR)name.c_str();
        
        if(name.length()) {
            
            SCARDCONTEXT hContext;
            LONG lResult = SCardEstablishContext(scope, NULL, NULL, &hContext);
            
            if (lResult == SCARD_E_NO_SERVICE) {
                HANDLE hEvent = SCardAccessStartedEvent();
                DWORD dwResult = WaitForSingleObject(hEvent, DEFAULT_TIMEOUT_MS_FOR_RESOURCE_MANAGER);
                if (dwResult == WAIT_OBJECT_0) {
                    lResult = SCardEstablishContext(scope, NULL, NULL, &hContext);
                }else{
                    errorMessage = "SCardAccessStartedEvent() failed";
                }
                SCardReleaseStartedEvent();
            }else{
                errorMessage = "SCardEstablishContext() failed";
            }
            
            if (lResult == SCARD_S_SUCCESS) {
                SCARD_READERSTATE readerState;
                readerState.szReader = lpszReaderName;
                readerState.dwCurrentState = SCARD_STATE_UNAWARE;
                /* return immediately; check state */
                lResult = SCardGetStatusChange(hContext, 0, &readerState, 1);
                if (lResult == SCARD_S_SUCCESS) {
                    
                    int is_card_present = 0;
                    
                    time_t startTime = time(0);
                    time_t anchorTime = startTime;
                    
                    bool isPolling = true;
                    
                    while (isPolling) {
                        
                        time_t now = time(0);
                        time_t elapsedTime = abs(startTime - now);
                        
                        if(elapsedTime > 0)
                        {
                            startTime = now;
                        }
                        
                        elapsedTime = abs(anchorTime - now);
                        
                        if(elapsedTime < SCARD_API_TIMEOUT) {
                            
                            if (readerState.dwEventState & SCARD_STATE_EMPTY) {
                                lResult = SCardGetStatusChange(hContext, LIBPCSC_API_TIMEOUT, &readerState, 1);
                            }
                            
                            if (readerState.dwEventState & SCARD_STATE_UNAVAILABLE) {
                                isPolling = false;
                            }
                            
                            if (readerState.dwEventState & SCARD_STATE_PRESENT) {
                                is_card_present = 1;
                                isPolling = false;
                            }
                            
                        }else{
                            /* timeout */
                            isPolling = false;
                        }
                        
                    }
                    
                    if(is_card_present) {
                        
                        SCARDHANDLE hCard;
                        DWORD dwActiveProtocol;
                        DWORD dwProtocol;
                        DWORD dwAtrSize;
                        DWORD dwState;
                        
                        BYTE atr[256];
                        
                        lResult = SCardConnect(hContext,
                                               lpszReaderName,
                                               mode,
                                               protocols,
                                               &hCard,
                                               &dwActiveProtocol);
                        switch (lResult)
                        {
                            case (LONG)SCARD_W_REMOVED_CARD:
                                /* SCARD_W_REMOVED_CARD */
                                break;
                            case SCARD_S_SUCCESS:
                                lResult = SCardStatus(hCard, NULL, NULL, &dwState, &dwProtocol, atr, &dwAtrSize);
                                if (lResult == SCARD_S_SUCCESS) {
                                    
                                    Json::Value services(Json::arrayValue);
                                    
                                    returnValue["services"] = services;
                                    
                                    get_IDm(hCard, &servicesToRead, &returnValue);
                                    
                                }/* SCardStatus */
                                SCardDisconnect(hCard, SCARD_LEAVE_CARD);
                                break;
                            default:
                                break;
                        }
                    }
                }else{
                    errorMessage = "SCardGetStatusChange() failed";
                }
                SCardReleaseContext(hContext);
            }else{
                errorMessage = "SCardEstablishContext() failed";
            }
        }
        
        if(1) {
            
            std::lock_guard<std::mutex> lock(scardMutex);
            
            if(scardStorage[uuid].isObject()) {
                if((returnValue.isMember("IDm")) && (returnValue.isMember("PMm"))) {
                    scardStorage[uuid]["IDm"] = returnValue["IDm"];
                    scardStorage[uuid]["PMm"] = returnValue["PMm"];
                    scardStorage[uuid]["services"] = returnValue["services"];
                    scardStorage[uuid]["success"] = true;
                }
                scardStorage[uuid]["errorMessage"] = errorMessage;
                scardStorage[uuid]["slotName"] = slotName;
                scardStorage[uuid]["complete"] = true;
            }
        }
    };
    
    scardTasks.insert(std::map< std::string,
                      std::future<void> >::value_type(uuid,
                                                      std::async(std::launch::async,
                                                                 func,
                                                                 uuid,
                                                                 slotName,
                                                                 serviceCodes)));
    
    ob_set_s(status, L"uuid", uuid.c_str());
    PA_ReturnObject(params, status);
}
#endif

void SCARD_Get_status(PA_PluginParameters params) {

    PA_ObjectRef status = PA_CreateObject();
    
    PA_Unistring *ustr = PA_GetStringParameter(params, 1);
    CUTF16String u16 = CUTF16String(ustr->fString, ustr->fLength);

    std::string uuid;
    u16_to_u8(u16, uuid);
    
    bool complete = false;
    bool success = false;
    
    std::string slotName, errorMessage, IDm, PMm;
    
    Json::Value threadCtx = scardStorage[uuid];
    
    if(threadCtx.isObject()) {
        
        std::lock_guard<std::mutex> lock(scardMutex);
        
        complete = threadCtx["complete"].asBool();
        success = threadCtx["success"].asBool();
        
        if(complete) {
            slotName = threadCtx["slotName"].asString();
            ob_set_s(status, L"slotName", slotName.c_str());
        }
        
        ob_set_s(status, L"uuid", uuid.c_str());
        ob_set_b(status, L"success", success);
        ob_set_b(status, L"complete", complete);
                
        if(success) {
            
            IDm = threadCtx["IDm"].asString();
            PMm = threadCtx["PMm"].asString();
            
            ob_set_s(status, L"IDm", IDm.c_str());
            ob_set_s(status, L"PMm", PMm.c_str());
            
            Json::Value services = threadCtx["services"];
            
            Json::StreamWriterBuilder writer;
            writer["indentation"] = "";
            std::string json = Json::writeString(writer, services);
            
            if(json.length() != 0) {

                CUTF16String jsonString;
                u8_to_u16(json, jsonString);
                                    
                PA_Variable params[2], result;
                PA_CollectionRef collection = NULL;
                PA_Unistring string;
                
                string = PA_CreateUnistring((PA_Unichar *)jsonString.c_str());
                PA_SetStringVariable(&params[0], &string);
                PA_SetLongintVariable(&params[1], eVK_Collection);
                result = PA_ExecuteCommandByID(1218, params, 2);    // JSON Parse
                collection = PA_GetCollectionVariable(result);
                PA_DisposeUnistring(&string);
                
                ob_set_c(status, "services", collection);
            }
            
        }

        errorMessage = threadCtx["errorMessage"].asString();
        
        if(errorMessage.length() != 0) {
            ob_set_s(status, L"errorMessage", errorMessage.c_str());
        }
        
    }
    
    PA_ReturnObject(params, status);
    
    if(complete) {
        scard_clear(uuid);
    }
    
}

#pragma mark -

static void u8_to_u16(std::string& u8, CUTF16String& u16) {
    
#ifdef _WIN32
    int len = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)u8.c_str(), u8.length(), NULL, 0);
    
    if(len){
        std::vector<uint8_t> buf((len + 1) * sizeof(PA_Unichar));
        if(MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)u8.c_str(), u8.length(), (LPWSTR)&buf[0], len)){
            u16 = CUTF16String((const PA_Unichar *)&buf[0]);
        }
    }else{
        u16 = CUTF16String((const PA_Unichar *)L"");
    }
    
#else
    CFStringRef str = CFStringCreateWithBytes(kCFAllocatorDefault, (const UInt8 *)u8.c_str(), u8.length(), kCFStringEncodingUTF8, true);
    if(str){
        CFIndex len = CFStringGetLength(str);
        std::vector<uint8_t> buf((len+1) * sizeof(PA_Unichar));
        CFStringGetCharacters(str, CFRangeMake(0, len), (UniChar *)&buf[0]);
        u16 = CUTF16String((const PA_Unichar *)&buf[0]);
        CFRelease(str);
    }
#endif
}

static void u16_to_u8(CUTF16String& u16, std::string& u8) {
    
#ifdef _WIN32
    int len = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)u16.c_str(), u16.length(), NULL, 0, NULL, NULL);
    
    if(len){
        std::vector<uint8_t> buf(len + 1);
        if(WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)u16.c_str(), u16.length(), (LPSTR)&buf[0], len, NULL, NULL)){
            u8 = std::string((const char *)&buf[0]);
        }
    }else{
        u8 = std::string((const char *)"");
    }

#else
    CFStringRef str = CFStringCreateWithCharacters(kCFAllocatorDefault, (const UniChar *)u16.c_str(), u16.length());
    if(str){
        size_t size = CFStringGetMaximumSizeForEncoding(CFStringGetLength(str), kCFStringEncodingUTF8) + sizeof(uint8_t);
        std::vector<uint8_t> buf(size);
        CFIndex len = 0;
        CFStringGetBytes(str, CFRangeMake(0, CFStringGetLength(str)), kCFStringEncodingUTF8, 0, true, (UInt8 *)&buf[0], size, &len);
        u8 = std::string((const char *)&buf[0], len);
        CFRelease(str);
    }
#endif
}
