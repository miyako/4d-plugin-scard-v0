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

#if VERSIONMAC
static bool checkAccess(PA_ObjectRef status) {
        
    bool entitlement = false;
    
    /*
     
     https://developer.apple.com/documentation/bundleresources/entitlements/com_apple_security_smartcard
     
     */
    
    SecTaskRef sec = SecTaskCreateFromSelf(kCFAllocatorMalloc);
    CFErrorRef err = nil;
    CFBooleanRef boolValue = (CFBooleanRef)SecTaskCopyValueForEntitlement(sec,
                                                                          CFSTR("com.apple.security.smartcard"),
                                                                          &err);
    if(!err) {
        if(boolValue) {
            entitlement = CFBooleanGetValue(boolValue);
        }
    }
    
    CFRelease(sec);
        
    return entitlement;
}
#endif

static void print_hex(const uint8_t *pbtData, const size_t szBytes, std::string &hex) {
    
    std::vector<uint8_t> buf((szBytes * 2) + 1);
    memset((char *)&buf[0], 0, buf.size());
    
    for (size_t i = 0; i < szBytes; ++i) {
        sprintf((char *)&buf[i * 2], "%02x", pbtData[i]);
    }
    
    hex = std::string((char *)&buf[0], (szBytes * 2));
}

#pragma mark -

#if VERSIONMAC
void SCARD_Get_readers(PA_PluginParameters params) {

    PA_ObjectRef options = PA_GetObjectParameter(params, 1);

    PA_ObjectRef status = PA_CreateObject();
    PA_CollectionRef readers = PA_CreateCollection();

    if(!checkAccess(status)) {
        ob_set_s(status, L"warning", "com.apple.security.smartcard is missing in app entitlement");
    }
    
    std::string errorMessage;
    bool success = false;
    
    TKSmartCardSlotManager *manager = [TKSmartCardSlotManager defaultManager];
    
    if(manager) {
        NSArray<NSString *> *slotNames = [manager slotNames];
        for (NSString *slotName in slotNames) {
            PA_ObjectRef o = PA_CreateObject();
            ob_set_s(o, L"slotName", (const char *)[slotName UTF8String]);
            PA_Variable v = PA_CreateVariable(eVK_Object);
            PA_SetObjectVariable(&v, o);
            PA_SetCollectionElement(readers, PA_GetCollectionLength(readers), v);
        }
        success = true;
    }else{
        errorMessage = "TKSmartCardSlotManager::defaultManager() failed";
    }

    ob_set_b(status, L"success", success);
    ob_set_c(status, L"readers", readers);
    PA_ReturnObject(params, status);
}
#endif

#pragma mark -

#if VERSIONMAC
typedef void (*get_service_t)(TKSmartCard *smartCard,
                              dispatch_semaphore_t sem,
                              Json::Value *serviceCode,
                              Json::Value *returnValue);
#endif

#if VERSIONMAC
static void get_ServiceData(TKSmartCard *smartCard,
                            dispatch_semaphore_t sem,
                            const char *serviceName,
                            uint8_t lo, uint8_t hi,
                            unsigned int blockSize,
                            Json::Value *servicesToRead,
                            Json::Value *returnValue) {
    
    __block Json::Value service(Json::objectValue);
    
    __block Json::Value data(Json::arrayValue);
    
    service["code"] = serviceName;
    
    smartCard.cla = 0xFF;
    
    uint8_t bytes[7] = {
        0xFF,   /* CLA::generic */
        0xA4,   /* INS::selectFile */
        0x00,   /* P1 */
        0x01,   /* P2 */
        0x02,
        hi,   /* service Hi */
        lo    /* service Lo */
    };
    
    __block int cnt = 0;
    
    for (int i = 0; i < blockSize; ++i) {

        [smartCard
         transmitRequest:[NSData dataWithBytes:bytes
                                        length:sizeof(bytes)]
         reply:^(NSData *response, NSError *error) {

            NSLog(@"selectFile:%d\r", i);
            
            if (error == nil)
            {
                uint8_t pbSendBuffer_ReadBinary[5] = {
                    0xFF,   /* CLA::generic */
                    0xB0,   /* INS::readBinary */
                    0x00,
                    0x00,   /* block */
                    0x00
                };
                
                pbSendBuffer_ReadBinary[3] = i;
                                
                NSLog(@"readBinary:%d\r", i);
                
                [smartCard
                 transmitRequest:[NSData dataWithBytes:pbSendBuffer_ReadBinary
                                                length:sizeof(pbSendBuffer_ReadBinary)]
                 reply:^(NSData *response, NSError *error) {
                    
                    if (error == nil)
                    {
                        if([response length] == 18) {
                            std::string hex;
                            print_hex((const uint8_t *)[response bytes], 16, hex);
                            data.append(hex);
                        }
                        
                        cnt++;
                        
                        if(cnt == blockSize){
                            
                            service["data"] = data;
                            returnValue->operator[]("services").append(service);
                            
                            if(servicesToRead->size() == 0) {
                                NSLog(@"readBinary:END\r");
                                [smartCard endSession];
                                dispatch_semaphore_signal(sem);
                            }else{

                                Json::Value serviceCode;
                                if(servicesToRead->removeIndex(0, &serviceCode)) {
                                    std::string name = serviceCode["name"].asString();
                                    unsigned int code = serviceCode["code"].asInt();
                                    uint8_t lo = code >> 8;
                                    uint8_t hi = code & 0x00FF;
                                    unsigned int size = serviceCode["size"].asInt();
                                    get_ServiceData(smartCard, sem, name.c_str(), lo, hi, size, servicesToRead, returnValue);
                                }else{
                                    [smartCard endSession];
                                    dispatch_semaphore_signal(sem);
                                }
                                
                            }
   
                        }
                        
                    }else{
                        NSLog(@"readBinary:ERROR\r");
//                      hard fail! do not call [smartCard endSession];
                        dispatch_semaphore_signal(sem);
                    }
                    

                }];
                usleep(TK_USLEEP_DURATION_FOR_POLLING);
            }else{
                NSLog(@"selectFile:ERROR\r");
//              hard fail! do not call [smartCard endSession];
                dispatch_semaphore_signal(sem);
            }
            
        }];
        usleep(TK_USLEEP_DURATION);
    }
    
}
#endif

#if VERSIONMAC
static void get_m(TKSmartCard *smartCard,
                  dispatch_semaphore_t sem,
                  const char *mName,
                  const uint8_t *bytes,
                  unsigned int len,
                  Json::Value *servicesToRead,
                  Json::Value *returnValue,
                  get_service_t next_call) {
                
    [smartCard
     transmitRequest:[NSData dataWithBytes:bytes
                                    length:len]
                                     reply:^(NSData *response, NSError *error) {
       
        if (error == nil)
        {
            uint8_t SW1 = 0;
            uint8_t SW2 = 0;
            
            [response getBytes:&SW1 range:NSMakeRange([response length] - 2, 1)];
            [response getBytes:&SW2 range:NSMakeRange([response length] - 1, 1)];
            
            if ( SW1 != 0x90 || SW2 != 0x00 )
            {
                if ( SW1 == 0x63 && SW2 == 0x00 )
                {
                    /* data is not available */
                }
                NSLog(@"getData:ERROR\r");
                [smartCard endSession];
                dispatch_semaphore_signal(sem);
            }
            else
            {
                std::string m;
                print_hex((const uint8_t *)[response bytes], 8, m);
                
                returnValue->operator[](mName) = m;
            
                if(next_call == NULL) {
                    NSLog(@"getData:END\r");
                    
                    if(servicesToRead->size() == 0) {
                        [smartCard endSession];
                        dispatch_semaphore_signal(sem);
                    }else{
                       
                        Json::Value serviceCode;
                        if(servicesToRead->removeIndex(0, &serviceCode)) {
                            std::string name = serviceCode["name"].asString();
                            unsigned int code = serviceCode["code"].asInt();
                            uint8_t lo = code >> 8;
                            uint8_t hi = code & 0x00FF;
                            unsigned int size = serviceCode["size"].asInt();
                            get_ServiceData(smartCard, sem, name.c_str(), lo, hi, size, servicesToRead, returnValue);
                        }else{
                            [smartCard endSession];
                            dispatch_semaphore_signal(sem);
                        }

                    }
                    
                }else{
                    
                    (*next_call)(smartCard, sem, servicesToRead, returnValue);
                }
                                
            }
        }else{
//                      hard fail! do not call [smartCard endSession];
            dispatch_semaphore_signal(sem);
        }
    }];
}

#endif

#if VERSIONMAC
static void get_PMm(TKSmartCard *smartCard,
                    dispatch_semaphore_t sem,
                    Json::Value *servicesToRead,
                    Json::Value *returnValue) {
    
    uint8_t bytes[5] = {
        0xFF,   /* CLA::generic */
        0xCA,   /* INS::getData */
        0x01,   /* P1::getPMm */
        0x00,   /* P2::none */
        0x00    /* LE::maxLength */
    };
    
    get_m(smartCard, sem, "PMm", bytes, 5, servicesToRead, returnValue, NULL);
    
}

static void get_IDm(TKSmartCard *smartCard,
                    dispatch_semaphore_t sem,
                    Json::Value *servicesToRead,
                    Json::Value *returnValue) {
    
    uint8_t bytes[5] = {
        0xFF,   /* CLA::generic */
        0xCA,   /* INS::getData */
        0x00,   /* P1::getIDm */
        0x00,   /* P2::none */
        0x00    /* LE::maxLength */
    };
    
    get_m(smartCard, sem, "IDm", bytes, 5, servicesToRead, returnValue, get_PMm);
    
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

#if VERSIONMAC
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
        
        __block Json::Value returnValue(Json::objectValue);
        __block Json::Value servicesToRead(Json::arrayValue);
        
        servicesToRead.copy(serviceCodes);
        
        TKSmartCardSlotManager *manager = [TKSmartCardSlotManager defaultManager];
        if(manager) {
            NSString *name = [[NSString alloc]initWithUTF8String:slotName.c_str()];
            TKSmartCardSlot *slot = [manager slotNamed:name];
            [name release];
            if(slot) {
                TKSmartCard *smartCard = [slot makeSmartCard];
                if(smartCard) {
                    
                    dispatch_semaphore_t sem = dispatch_semaphore_create(0);

                    returnValue["success"] = false;
                    
                    [smartCard beginSessionWithReply:^(BOOL _success, NSError *error) {
                        
                        if (_success) {
     
                            Json::Value services(Json::arrayValue);
                            
                            returnValue["services"] = services;
                            
                            get_IDm(smartCard, sem, &servicesToRead, &returnValue);
                    
                        }else{
                           //beginSessionWithReply() failed
                            dispatch_semaphore_signal(sem);
                        }
                        
                    }];
                    // wait for the asynchronous blocks to finish
                    dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
                    
                }else{
                    errorMessage = "TKSmartCard::makeSmartCard() failed";
                }
            }else{
                errorMessage = "TKSmartCardSlotManager::slotNamed() failed";
            }
        }else{
            errorMessage = "TKSmartCardSlotManager::defaultManager() failed";
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
