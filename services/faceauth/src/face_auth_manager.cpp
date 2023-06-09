/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "face_auth_manager.h"
#include <openssl/bn.h>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <iservice_registry.h>
#include "securec.h"
#include "face_auth_log_wrapper.h"
#include "face_auth_event_handler.h"
#include "auth_executor_registry.h"
#include "useridm_client.h"
#include "useridm_info.h"
#include "face_auth_get_info_callback.h"
#include "auth_message.h"
#include "co_auth_info_define.h"
#include "face_auth_thread_pool.h"
#include "face_auth_req.h"
#include "face_auth_camera.h"
#include "face_auth_defines.h"
#include "return_callback.h"

namespace OHOS {
namespace UserIAM {
namespace FaceAuth {
const int RAND_NUM_BITS = 32;
const int TOP = -1;
const int BOTTOM = 0;
const int INVALID_EVENT_ID = -1;
static const std::string FACE_LOCAL_INIT_ALGO_NAME = "face_auth_local_init";
std::shared_ptr<FaceAuthManager> FaceAuthManager::manager_ = nullptr;
std::mutex FaceAuthManager::mutex_;
sptr<AuthResPool::IExecutorMessenger> FaceAuthManager::executorMessenger_;
std::shared_ptr<FaceAuthEventHandler> FaceAuthManager::handler_ = nullptr;
std::shared_ptr<AppExecFwk::EventRunner> FaceAuthManager::runner_ = nullptr;
std::shared_ptr<AuthResPool::QueryCallback> FaceAuthManager::queryCallback_ = nullptr;
std::shared_ptr<FaceAuthExecutorCallback> FaceAuthManager::executorCallback_ = nullptr;
static void CheckSystemAbility();

std::shared_ptr<FaceAuthManager> FaceAuthManager::GetInstance()
{
    if (manager_ == nullptr) {
        std::lock_guard<std::mutex> lock_l(mutex_);
        if (manager_ == nullptr) {
            manager_ = std::make_shared<FaceAuthManager>();
        }
    }
    return manager_;
}

int32_t FaceAuthManager::Init()
{
    FACEAUTH_HILOGI(MODULE_SERVICE, "start");
    std::string threadName("FaceAuthEventRunner");
    runner_ = AppExecFwk::EventRunner::Create(threadName);
    if (runner_ == nullptr) {
        FACEAUTH_HILOGE(MODULE_SERVICE, "failed to create runner");
        return FA_RET_ERROR;
    }
    runner_->Run();
    handler_ = std::make_shared<FaceAuthEventHandler>(runner_);
    std::shared_ptr<FaceAuthCA> faceAuthCA = FaceAuthCA::GetInstance();
    if (faceAuthCA == nullptr) {
        FACEAUTH_HILOGE(MODULE_SERVICE, "create FaceAuthCA instance failed");
        return FA_RET_ERROR;
    }
    if (FA_RET_OK != faceAuthCA->Init()) {
        FACEAUTH_HILOGE(MODULE_SERVICE, "Init CA failed");
        return FA_RET_ERROR;
    }
    FACEAUTH_HILOGI(MODULE_SERVICE, "init Success");
    std::thread checkThread(OHOS::UserIAM::FaceAuth::CheckSystemAbility);
    checkThread.join();
    QueryRegStatus();
    return FA_RET_OK;
}

void CheckSystemAbility()
{
    const int checkTimes = 3;
    const int sleepTime = 1;
    sptr<ISystemAbilityManager> sam = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (sam == nullptr) {
        FACEAUTH_HILOGE(MODULE_SERVICE, "Failed to get system ability manager");
        return;
    }
    for (int i = 0; i < checkTimes; i++) {
        bool isExist = false;
        sam->CheckSystemAbility(SUBSYS_USERIAM_SYS_ABILITY_AUTHEXECUTORMGR, isExist);
        if (!isExist) {
            FACEAUTH_HILOGI(MODULE_SERVICE, "AUTHEXECUTORMGR is not exist, start ability failed, to do next");
        } else {
            FACEAUTH_HILOGI(MODULE_SERVICE, "AUTHEXECUTORMGR is exist, start AUTHEXECUTORMGR ability success");
            return;
        }
        if (i < checkTimes - 1) {
            FACEAUTH_HILOGI(MODULE_SERVICE, "begin sleep");
            sleep(sleepTime);
            FACEAUTH_HILOGI(MODULE_SERVICE, "end sleep");
        }
    }
    FACEAUTH_HILOGE(MODULE_SERVICE, "start AUTHEXECUTORMGR ability all failed");
}

void FaceAuthManager::QueryRegStatus()
{
    FACEAUTH_HILOGI(MODULE_SERVICE, "start");
    std::shared_ptr<AuthResPool::AuthExecutor> executorInfo = std::make_shared<AuthResPool::AuthExecutor>();
    std::vector<uint8_t> pubKey;
    uint32_t esl = 0;
    uint64_t authAbility = 0;
    std::shared_ptr<FaceAuthCA> faceAuthCA = FaceAuthCA::GetInstance();
    if (faceAuthCA == nullptr) {
        FACEAUTH_HILOGE(MODULE_SERVICE, "create FaceAuthCA instance failed");
        return;
    }
    int32_t ret = faceAuthCA->GetExecutorInfo(pubKey, esl, authAbility);
    if (FA_RET_OK != ret) {
        FACEAUTH_HILOGE(MODULE_SERVICE, "GetExecutorInfo failed");
        return;
    }
    executorInfo->SetAuthType(FACE);
    executorInfo->SetExecutorType(TYPE_ALL_IN_ONE);

    if (queryCallback_== nullptr) {
        queryCallback_ = std::make_shared<FaceAuthQueryCallback>();
    }
    AuthResPool::AuthExecutorRegistry::GetInstance().QueryStatus(*executorInfo, queryCallback_);
}

void FaceAuthManager::RegisterExecutor()
{
    FACEAUTH_HILOGI(MODULE_SERVICE, "start");
    std::shared_ptr<FaceAuthCA> faceAuthCA = FaceAuthCA::GetInstance();
    if (faceAuthCA == nullptr) {
        FACEAUTH_HILOGE(MODULE_SERVICE, "create FaceAuthCA instance failed");
        return;
    }
    std::vector<uint8_t> pubKey;
    uint32_t esl = 0;
    uint64_t authAbility = 0;
    int32_t ret = faceAuthCA->GetExecutorInfo(pubKey, esl, authAbility);
    if (FA_RET_OK != ret) {
        FACEAUTH_HILOGE(MODULE_SERVICE, "GetExecutorInfo failed");
        return;
    }
    std::shared_ptr<AuthResPool::AuthExecutor> executorInfo = std::make_shared<AuthResPool::AuthExecutor>();
    executorInfo->SetPublicKey(pubKey);
    executorInfo->SetExecutorSecLevel(static_cast<ExecutorSecureLevel>(esl));
    executorInfo->SetAuthAbility(authAbility);
    executorInfo->SetAuthType(FACE);
    executorInfo->SetExecutorType(TYPE_ALL_IN_ONE);
    executorCallback_ = std::make_shared<FaceAuthExecutorCallback>();
    uint64_t regRet = AuthResPool::AuthExecutorRegistry::GetInstance().Register(executorInfo, executorCallback_);
    if (regRet != 0) {
        FACEAUTH_HILOGI(MODULE_SERVICE, "FaceAuthInitSeq::RegisterExecutor successful.executor id = %{public}s",
            getMaskedString(regRet).c_str());
    } else {
        FACEAUTH_HILOGE(MODULE_SERVICE, "FaceAuthInitSeq::RegisterExecutor failed");
    }
    return;
}

void FaceAuthManager::VerifyAuthInfo()
{
    FACEAUTH_HILOGI(MODULE_SERVICE, "start");
    const int32_t ALL_INFO_GET_USER_ID = -1;

    std::shared_ptr<FaceAuthGetInfoCallback> getInfoCallback = std::make_shared<FaceAuthGetInfoCallback>();
    int32_t ret = UserIDM::UserIDMClient::GetInstance().GetAuthInfo(ALL_INFO_GET_USER_ID, UserIDM::AuthType::FACE,
        getInfoCallback);
    if (ret == 0) {
        FACEAUTH_HILOGI(MODULE_SERVICE, "get auth info success");
    } else {
        FACEAUTH_HILOGE(MODULE_SERVICE, "get auth info failed");
    }
    return;
}

int32_t FaceAuthManager::Release()
{
    FACEAUTH_HILOGI(MODULE_SERVICE, "start");
    std::shared_ptr<FaceAuthCA> faceAuthCA = FaceAuthCA::GetInstance();
    if (faceAuthCA != nullptr) {
        faceAuthCA->Close();
    }
    if (runner_ != nullptr) {
        runner_.reset();
    }
    return FA_RET_OK;
}

ResultCodeForCoAuth FaceAuthManager::AddAuthenticationRequest(const AuthParam &param)
{
    FACEAUTH_HILOGI(MODULE_SERVICE, "start");
    FACEAUTH_HILOGI(MODULE_SERVICE, "[DoAuth]scheduleID = %{public}s", getMaskedString(param.scheduleID).c_str());
    FACEAUTH_HILOGI(MODULE_SERVICE, "[DoAuth]templateID = %{public}s", getMaskedString(param.templateID).c_str());
    FaceReqType reqType = {};
    reqType.reqId = param.scheduleID;
    reqType.operateType = FACE_OPERATE_TYPE_LOCAL_AUTH;
    if (FaceAuthReq::GetInstance()->IsReqNumReachedMax(FACE_OPERATE_TYPE_LOCAL_AUTH)) {
        FACEAUTH_HILOGE(MODULE_SERVICE, "Auth request num exceeds limit");
        return ResultCodeForCoAuth::BUSY;
    }
    FaceInfo faceInfo = {};
    faceInfo.eventId = GenerateEventId();
    if (faceInfo.eventId == INVALID_EVENT_ID) {
        FACEAUTH_HILOGE(MODULE_SERVICE, "faceInfo.eventId is invalid");
        return ResultCodeForCoAuth::GENERAL_ERROR;
    }
    faceInfo.uId = param.callerUID;
    FaceAuthReq::GetInstance()->AddReqInfo(reqType, faceInfo);
    FaceAuthEventHandler::Priority priority = FaceAuthEventHandler::Priority::LOW;
    auto authInfo = std::make_unique<AuthParam>(param);
    handler_->SendEvent(faceInfo.eventId, std::move(authInfo), priority);
    return ResultCodeForCoAuth::SUCCESS;
}

void FaceAuthManager::DoAuthenticate(const AuthParam &param)
{
    ReturnCallback removeRequireInfoWhenReturn([param]() {
        FaceReqType reqType = {};
        reqType.reqId = param.scheduleID;
        reqType.operateType = FACE_OPERATE_TYPE_LOCAL_AUTH;
        FaceAuthReq::GetInstance()->RemoveRequireInfo(reqType);
    });
    this->InitAlgorithm(FACE_LOCAL_INIT_ALGO_NAME);
    ReturnCallback releaseAlgorithmWhenReturn([this]() {
        this->ReleaseAlgorithm(FACE_LOCAL_INIT_ALGO_NAME);
    });
    if (OpenCamera(nullptr) != FA_RET_OK) {
        // RK3568 no support camera, temporary ignore error
        FACEAUTH_HILOGE(MODULE_SERVICE, "Ignore open camera fail");
    }
    ReturnCallback closeCameraWhenReturn([]() {
        std::shared_ptr<FaceAuthCamera> faceAuthCamera = FaceAuthCamera::GetInstance();
        if (faceAuthCamera == nullptr) {
            FACEAUTH_HILOGE(MODULE_SERVICE, "face auth camera is null");
            return;
        }
        faceAuthCamera->CloseCamera();
    });
    std::shared_ptr<FaceAuthCA> faceAuthCA = FaceAuthCA::GetInstance();
    if (faceAuthCA == nullptr) {
        FACEAUTH_HILOGE(MODULE_SERVICE, "faceAuthCA instance is null");
        return;
    }
    int32_t ret = faceAuthCA->StartAlgorithmOperation(Auth, AlgorithmParam {
        .templateId = param.templateID,
        .scheduleId = param.scheduleID
    });
    if (ret != FA_RET_OK) {
        FACEAUTH_HILOGE(MODULE_SERVICE, "StartAlgorithmOperation failed");
        return;
    }
    ret = DoAlgorithmOperation(param.scheduleID);
    if (ret != FA_RET_OK) {
        FACEAUTH_HILOGE(MODULE_SERVICE, "DoAlgorithmOperation failed");
    }
    HandleAlgorithmResult(param.scheduleID, FACE_OPERATE_TYPE_LOCAL_AUTH);
}

ResultCodeForCoAuth FaceAuthManager::AddEnrollmentRequest(const EnrollParam &param)
{
    FACEAUTH_HILOGI(MODULE_SERVICE, "start");
    FACEAUTH_HILOGI(MODULE_SERVICE, "[DoEnroll]scheduleID = %{public}s", getMaskedString(param.scheduleID).c_str());
    FACEAUTH_HILOGI(MODULE_SERVICE, "[DoEnroll]templateID = %{public}s", getMaskedString(param.templateID).c_str());
    FaceReqType reqType = {};
    reqType.reqId = param.scheduleID;
    reqType.operateType = FACE_OPERATE_TYPE_ENROLL;
    if (FaceAuthReq::GetInstance()->IsReqNumReachedMax(FACE_OPERATE_TYPE_ENROLL)) {
        FACEAUTH_HILOGE(MODULE_SERVICE, "Enroll request num exceeds limit");
        return ResultCodeForCoAuth::BUSY;
    }
    FaceInfo faceInfo = {};
    faceInfo.eventId = GenerateEventId();
    if (faceInfo.eventId == INVALID_EVENT_ID) {
        FACEAUTH_HILOGE(MODULE_SERVICE, "faceInfo.eventId is invalid");
        return ResultCodeForCoAuth::GENERAL_ERROR;
    }
    faceInfo.uId = param.callerUID;
    FaceAuthReq::GetInstance()->AddReqInfo(reqType, faceInfo);
    FaceAuthEventHandler::Priority priority = FaceAuthEventHandler::Priority::HIGH;
    auto authInfo = std::make_unique<EnrollParam>(param);
    handler_->SendEvent(faceInfo.eventId, std::move(authInfo), priority);
    return ResultCodeForCoAuth::SUCCESS;
}

void FaceAuthManager::DoEnroll(const EnrollParam &param)
{
    ReturnCallback removeRequireInfoWhenReturn([param]() {
        FaceReqType reqType = {};
        reqType.reqId = param.scheduleID;
        reqType.operateType = FACE_OPERATE_TYPE_ENROLL;
        FaceAuthReq::GetInstance()->RemoveRequireInfo(reqType);
    });
    this->InitAlgorithm(FACE_LOCAL_INIT_ALGO_NAME);
    ReturnCallback releaseAlgorithmWhenReturn([this]() {
        this->ReleaseAlgorithm(FACE_LOCAL_INIT_ALGO_NAME);
    });
    if (OpenCamera(param.producer) != FA_RET_OK) {
        // RK3568 no support camera, temporary ignore error
        FACEAUTH_HILOGI(MODULE_SERVICE, "Ignore open camera fail");
    }
    ReturnCallback closeCameraWhenReturn([]() {
        std::shared_ptr<FaceAuthCamera> faceAuthCamera = FaceAuthCamera::GetInstance();
        if (faceAuthCamera == nullptr) {
            FACEAUTH_HILOGE(MODULE_SERVICE, "face auth camera is null");
            return;
        }
        faceAuthCamera->CloseCamera();
    });
    std::shared_ptr<FaceAuthCA> faceAuthCA = FaceAuthCA::GetInstance();
    if (faceAuthCA == nullptr) {
        FACEAUTH_HILOGE(MODULE_SERVICE, "faceAuthCA instance is null");
        return;
    }
    int32_t ret = faceAuthCA->StartAlgorithmOperation(Enroll, AlgorithmParam {
        .templateId = param.templateID,
        .scheduleId = param.scheduleID
    });
    if (ret != FA_RET_OK) {
        FACEAUTH_HILOGE(MODULE_SERVICE, "StartAlgorithmOperation failed");
        return;
    }
    ret = DoAlgorithmOperation(param.scheduleID);
    if (ret != FA_RET_OK) {
        FACEAUTH_HILOGE(MODULE_SERVICE, "DoAlgorithmOperation failed");
    }
    HandleAlgorithmResult(param.scheduleID, FACE_OPERATE_TYPE_ENROLL);
}

int32_t FaceAuthManager::AddRemoveRequest(const RemoveParam &param)
{
    FACEAUTH_HILOGI(MODULE_SERVICE, "start");
    FACEAUTH_HILOGI(MODULE_SERVICE, "[DoRemove]scheduleID = %{public}s", getMaskedString(param.scheduleID).c_str());
    FACEAUTH_HILOGI(MODULE_SERVICE, "[DoRemove]templateID = %{public}s", getMaskedString(param.templateID).c_str());
    FaceReqType reqType = {};
    reqType.reqId = param.scheduleID;
    reqType.operateType = FACE_OPERATE_TYPE_DEL;
    if (FaceAuthReq::GetInstance()->IsReqNumReachedMax(FACE_OPERATE_TYPE_DEL)) {
        FACEAUTH_HILOGE(MODULE_SERVICE, "Remove request num exceeds limit");
        return FA_RET_ERROR;
    }
    FaceInfo faceInfo = {};
    faceInfo.eventId = GenerateEventId();
    if (faceInfo.eventId == INVALID_EVENT_ID) {
        FACEAUTH_HILOGE(MODULE_SERVICE, "faceInfo.eventId is invalid");
        return FA_RET_ERROR;
    }
    faceInfo.uId = param.callerUID;
    FaceAuthReq::GetInstance()->AddReqInfo(reqType, faceInfo);
    FaceAuthEventHandler::Priority priority = FaceAuthEventHandler::Priority::IMMEDIATE;
    auto authInfo = std::make_unique<RemoveParam>(param);
    handler_->SendEvent(faceInfo.eventId, std::move(authInfo), priority);
    return FA_RET_OK;
}

void FaceAuthManager::DoRemove(const RemoveParam &param)
{
    ReturnCallback removeRequireInfoWhenReturn([param]() {
        FaceReqType reqType = {};
        reqType.reqId = param.scheduleID;
        reqType.operateType = FACE_OPERATE_TYPE_DEL;
        FaceAuthReq::GetInstance()->RemoveRequireInfo(reqType);
    });
    std::shared_ptr<FaceAuthCA> faceAuthCA = FaceAuthCA::GetInstance();
    if (faceAuthCA == nullptr) {
        FACEAUTH_HILOGE(MODULE_SERVICE, "faceAuthCA instance is null");
        return;
    }
    int32_t ret = faceAuthCA->DeleteTemplate(param.templateID);
    if (FA_RET_OK == ret) {
        FACEAUTH_HILOGI(MODULE_SERVICE, "Remove success");
    } else {
        FACEAUTH_HILOGE(MODULE_SERVICE, "Remove failed");
    }
}

void FaceAuthManager::HandleAlgorithmResult(const uint64_t &scheduleID, const FaceOperateType &type)
{
    std::shared_ptr<FaceAuthCA> faceAuthCA = FaceAuthCA::GetInstance();
    if (faceAuthCA == nullptr) {
        FACEAUTH_HILOGI(MODULE_SERVICE, "faceAuthCA is null");
        return;
    }
    AlgorithmResult retResult;
    faceAuthCA->FinishAlgorithmOperation(retResult);
    FACEAUTH_HILOGI(MODULE_SERVICE, "Face auth result = %{public}d", retResult.result);
    pAuthAttributes authAttributes = std::make_shared<AuthResPool::AuthAttributes>();
    authAttributes->SetUint32Value(AUTH_RESULT_CODE, 0);
    authAttributes->SetUint8ArrayValue(AUTH_RESULT, retResult.coauthMsg);
    FaceReqType reqType = {};
    reqType.reqId = scheduleID;
    reqType.operateType = type;
    FaceAuthReq::GetInstance()->RemoveRequireInfo(reqType);
    Finish(scheduleID, TYPE_ALL_IN_ONE, retResult.result, authAttributes);
}

int32_t FaceAuthManager::CancelAuth(const AuthParam &param)
{
    FaceReqType reqType = {};
    reqType.reqId = param.scheduleID;
    reqType.operateType = FACE_OPERATE_TYPE_LOCAL_AUTH;
    int32_t uId = param.callerUID;
    bool isSuccess = FaceAuthReq::GetInstance()->SetCancelFlagSuccess(reqType, uId);
    if (!isSuccess) {
        FACEAUTH_HILOGE(MODULE_SERVICE, "CancelAuth failed, reqId: %{public}s,",
            getMaskedString(reqType.reqId).c_str());
        return FA_RET_ERROR;
    }
    std::shared_ptr<FaceAuthCA> faceAuthCA = FaceAuthCA::GetInstance();
    if (faceAuthCA == nullptr) {
        FACEAUTH_HILOGE(MODULE_SERVICE, "FaceAuthCA instance is null");
        return FA_RET_ERROR;
    }
    int32_t result = faceAuthCA->CancelAlgorithmOperation();
    if (result == FA_RET_OK) {
        FACEAUTH_HILOGI(MODULE_SERVICE, "CancelAlgorithmOperation success");
    } else {
        FACEAUTH_HILOGE(MODULE_SERVICE, "CancelAlgorithmOperation failed");
    }
    return result;
}

int32_t FaceAuthManager::CancelEnrollment(const EnrollParam &param)
{
    FaceReqType reqType = {};
    reqType.reqId = param.scheduleID;
    reqType.operateType = FACE_OPERATE_TYPE_ENROLL;
    int32_t uId = param.callerUID;
    bool isSuccess = FaceAuthReq::GetInstance()->SetCancelFlagSuccess(reqType, uId);
    if (!isSuccess) {
        FACEAUTH_HILOGE(MODULE_SERVICE, "CancelEnrollment failed, reqId: %{public}s",
            getMaskedString(reqType.reqId).c_str());
        return FA_RET_ERROR;
    }
    std::shared_ptr<FaceAuthCA> faceAuthCA = FaceAuthCA::GetInstance();
    if (faceAuthCA == nullptr) {
        FACEAUTH_HILOGE(MODULE_SERVICE, "FaceAuthCA instance is null");
        return FA_RET_ERROR;
    }
    int32_t result = faceAuthCA->CancelAlgorithmOperation();
    if (result == FA_RET_OK) {
        FACEAUTH_HILOGI(MODULE_SERVICE, "CancelAlgorithmOperation success");
    } else {
        FACEAUTH_HILOGE(MODULE_SERVICE, "CancelAlgorithmOperation failed");
    }
    return result;
}

void FaceAuthManager::SetExecutorMessenger(const sptr<AuthResPool::IExecutorMessenger> &messager)
{
    executorMessenger_ = messager;
}
void FaceAuthManager::SendData(uint64_t scheduleId, uint64_t transNum, int32_t srcType, int32_t dstType,
    pAuthMessage msg)
{
    if (executorMessenger_ != nullptr) {
        executorMessenger_->SendData(scheduleId, transNum, srcType, dstType, msg);
    } else {
        FACEAUTH_HILOGI(MODULE_SERVICE, "executorMessenger_ is null");
    }
}

void FaceAuthManager::Finish(uint64_t scheduleId, int32_t srcType, int32_t resultCode, pAuthAttributes finalResult)
{
    if (executorMessenger_ != nullptr) {
        executorMessenger_->Finish(scheduleId, srcType, resultCode, finalResult);
    } else {
        FACEAUTH_HILOGI(MODULE_SERVICE, "executorMessenger_ is null");
    }
}

FIRetCode FaceAuthManager::InitAlgorithm(std::string bundleName)
{
    FACEAUTH_HILOGI(MODULE_SERVICE, "Init, bundleName:%{public}s", bundleName.c_str());
    AlgoResult result = IsNeedAlgoLoad(bundleName);
    if (result == AR_EMPTY) {
        auto promiseObj = std::make_shared<std::promise<int32_t>>();
        auto futureObj = promiseObj->get_future();
        FaceAuthThreadPool::GetInstance()->AddTask([promiseObj]() {
            promiseObj->set_value(FaceAuthCA::GetInstance()->LoadAlgorithm());
        });
        std::chrono::microseconds span(INIT_DYNAMIC_TIME_OUT);
        if (futureObj.wait_for(span) == std::future_status::timeout) {
            FACEAUTH_HILOGI(MODULE_SERVICE, "LoadAlgorithm TimeOut");
            return FI_RC_ERROR;
        }
        return static_cast<FIRetCode>(futureObj.get());
    }
    FACEAUTH_HILOGE(MODULE_SERVICE, "Init Fail %{public}d", result);
    return FI_RC_ERROR;
}

FIRetCode FaceAuthManager::ReleaseAlgorithm(std::string bundleName)
{
    FACEAUTH_HILOGI(MODULE_SERVICE, "Release, bundleName:%{public}s", bundleName.c_str());
    AlgoResult result = IsNeedAlgoRelease(bundleName);
    if (result == AR_EMPTY) {
        auto promiseObj = std::make_shared<std::promise<int32_t>>();
        auto futureObj = promiseObj->get_future();
        FaceAuthThreadPool::GetInstance()->AddTask([promiseObj]() {
            promiseObj->set_value(FaceAuthCA::GetInstance()->ReleaseAlgorithm());
        });
        std::chrono::microseconds span(RELEASE_DYNAMIC_TIME_OUT);
        if (futureObj.wait_for(span) == std::future_status::timeout) {
            FACEAUTH_HILOGI(MODULE_SERVICE, "ReleaseAlgorithm TimeOut");
            return FI_RC_ERROR;
        }
        return static_cast<FIRetCode>(futureObj.get());
    }
    FACEAUTH_HILOGE(MODULE_SERVICE, "Release Fail %{public}d", result);
    return FI_RC_ERROR;
}

bool FaceAuthManager::IsAlgorithmInited()
{
    if (bundleNameList_.empty()) {
        FACEAUTH_HILOGI(MODULE_SERVICE, "algorithm is initted");
        return true;
    }
    FACEAUTH_HILOGI(MODULE_SERVICE, "algorithm is not initted");
    return false;
}

AlgoResult FaceAuthManager::IsNeedAlgoLoad(std::string bundleName)
{
    FACEAUTH_HILOGI(MODULE_SERVICE, "start");
    AlgoResult result = AR_SUCCESS;

    if (bundleNameList_.empty()) {
        result = AR_EMPTY;
    }

    if (bundleNameList_.find(bundleName) != bundleNameList_.end()) {
        bundleNameList_[bundleName] = bundleNameList_[bundleName] + 1;
        FACEAUTH_HILOGI(MODULE_SERVICE,
            "Add same bundleName:%{public}s, num:%{public}d", bundleName.c_str(), bundleNameList_[bundleName]);
        return AR_ADD_AGAIN;
    }

    bundleNameList_.insert(std::pair<std::string, int32_t>(bundleName, 1));

    FACEAUTH_HILOGI(MODULE_SERVICE, "Insert bundleName:%{public}s", bundleName.c_str());
    FACEAUTH_HILOGI(MODULE_SERVICE, "Result:%{public}d", result);
    return result;
}

AlgoResult FaceAuthManager::IsNeedAlgoRelease(std::string bundleName)
{
    AlgoResult result = AR_SUCCESS;
    if (bundleNameList_.erase(bundleName) != 0) {
        if (bundleNameList_.empty()) {
            result = AR_EMPTY;
        }
        FACEAUTH_HILOGI(MODULE_SERVICE, "Remove Success bundleName:%{public}s", bundleName.c_str());
    } else {
        result = AR_NOT_FOUND;
        FACEAUTH_HILOGE(MODULE_SERVICE, "Remove Fail bundleName:%{public}s", bundleName.c_str());
    }
    return result;
}

int32_t FaceAuthManager::GenerateEventId()
{
    int32_t randomNum = INVALID_EVENT_ID;
    if (!GetRandomNum(&randomNum)) {
        FACEAUTH_HILOGE(MODULE_SERVICE, "GetRandomNum error");
        return INVALID_EVENT_ID;
    }
    int32_t eventId = randomNum;
    FACEAUTH_HILOGI(MODULE_SERVICE, "GenerateEventId generate eventId %{public}u", eventId);
    return eventId;
}
int32_t FaceAuthManager::OpenCamera(sptr<IBufferProducer> producer)
{
    auto promiseObj = std::make_shared<std::promise<int32_t>>();
    auto futureObj = promiseObj->get_future();
    FaceAuthThreadPool::GetInstance()->AddTask([promiseObj, producer]() {
        promiseObj->set_value(FaceAuthCamera::GetInstance()->OpenCamera(producer));
    });
    std::chrono::microseconds span(OPEN_CAMERA_TIME_OUT);
    if (futureObj.wait_for(span) == std::future_status::timeout) {
        FACEAUTH_HILOGI(MODULE_SERVICE, "Open Camera TimeOut");
        return FA_RET_ERROR;
    }
    if (futureObj.get() == FA_RET_ERROR) {
        FACEAUTH_HILOGE(MODULE_SERVICE, "Open Camera Fail");
        return FA_RET_ERROR;
    }
    return FA_RET_OK;
}

std::pair<int32_t, std::vector<uint8_t>> FaceAuthManager::GetAlgorithmState(const int &requestId)
{
    int retCode = FACE_ALGORITHM_OPERATION_BREAK;
    std::vector<uint8_t> retMsg;
    std::shared_ptr<FaceAuthCA> faceAuthCA = FaceAuthCA::GetInstance();
    if (faceAuthCA == nullptr) {
        FACEAUTH_HILOGE(MODULE_SERVICE, "faceAuthCA is null");
        return std::make_pair(FACE_ALGORITHM_OPERATION_BREAK, retMsg);
    }
    FACEAUTH_HILOGI(MODULE_SERVICE, "requestId %{public}d: start wait for algorithm state", requestId);
    faceAuthCA->GetAlgorithmState(retCode, retMsg);
    FACEAUTH_HILOGI(MODULE_SERVICE, "requestId %{public}d: algorithm state obtained code %{public}d "
        "msg length %{public}zu",
        requestId, retCode, retMsg.size());
    return std::make_pair(retCode, retMsg);
}

FIRetCode FaceAuthManager::DoAlgorithmOperation(const uint64_t &scheduleID)
{
    const std::chrono::seconds GET_RESULT_TIME_LIMIT(GET_RESULT_TIME_LIMIT_IN_SEC);
    using RetPair = std::pair<int32_t, std::vector<uint8_t>>;
    int requestId = 0;
    std::chrono::steady_clock::time_point begin_time = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - begin_time)
        < GET_RESULT_TIME_LIMIT) {
        auto promiseObj = std::make_shared<std::promise<RetPair>>();
        auto futureObj = promiseObj->get_future();
        FaceAuthThreadPool::GetInstance()->AddTask([promiseObj, requestId]() {
            promiseObj->set_value(FaceAuthManager::GetInstance()->GetAlgorithmState(requestId));
        });
        if (futureObj.wait_for(GET_RESULT_TIME_LIMIT) == std::future_status::timeout) {
            FACEAUTH_HILOGE(MODULE_SERVICE, "GetAlgorithmState timeOut");
            return FI_RC_ERROR;
        }
        auto ret = futureObj.get();
        int32_t retCode = ret.first;
        if (retCode == FACE_ALGORITHM_OPERATION_BREAK) {
            FACEAUTH_HILOGI(MODULE_SERVICE, "GetAlgorithmState break");
            return FI_RC_OK;
        }

        if (ret.second.size() > 0) {
            std::shared_ptr<AuthResPool::AuthMessage> msg = std::make_shared<AuthResPool::AuthMessage>(
                ret.second);
            SendData(scheduleID, 0, TYPE_ALL_IN_ONE, TYPE_CO_AUTH, msg);
        }
        requestId++;
    }
    FACEAUTH_HILOGE(MODULE_SERVICE, "timeout");
    return FI_RC_ERROR;
}

bool FaceAuthManager::GetRandomNum(int32_t *randomNum)
{
    if (randomNum == nullptr) {
        FACEAUTH_HILOGE(MODULE_SERVICE, "randomNum is nullptr");
        return false;
    }
    BIGNUM *bn = BN_new();
    if (bn == nullptr) {
        FACEAUTH_HILOGE(MODULE_SERVICE, "BN_new fail");
        return false;
    }
    if (BN_rand(bn, RAND_NUM_BITS, TOP, BOTTOM) == 0) {
        FACEAUTH_HILOGE(MODULE_SERVICE, "BN_rand fail");
        BN_free(bn);
        return false;
    }
    char *decVal = BN_bn2dec(bn);
    if (decVal == nullptr) {
        FACEAUTH_HILOGE(MODULE_SERVICE, "BN_bn2dec is nullptr");
        BN_free(bn);
        return false;
    }
    *randomNum = atoi(decVal);
    BN_free(bn);
    return true;
}

FIRetCode FaceAuthManager::DoWaitInitAlgorithm(std::future<int32_t> futureObj)
{
    std::chrono::microseconds span(INIT_DYNAMIC_TIME_OUT);
    if (futureObj.wait_for(span) == std::future_status::timeout) {
        FACEAUTH_HILOGI(MODULE_SERVICE, "LoadAlgorithm TimeOut");
        return FI_RC_ERROR;
    }
    return static_cast<FIRetCode>(futureObj.get());
}

void FaceAuthManager::UnfreezeTemplates(std::vector<uint64_t> templateIdList)
{
    std::shared_ptr<FaceAuthCA> faceAuthCA = FaceAuthCA::GetInstance();
    if (faceAuthCA == nullptr) {
        FACEAUTH_HILOGE(MODULE_SERVICE, "get FaceAuthCA instance failed");
        return;
    }

    for (auto templateId : templateIdList) {
        if (faceAuthCA->ResetRemainTimes(templateId) != FA_RET_OK) {
            FACEAUTH_HILOGE(MODULE_SERVICE, "resetRemainTimes failed");
        }
    }
}

void FaceAuthManager::FreezeTemplates(std::vector<uint64_t> templateIdList)
{
    std::shared_ptr<FaceAuthCA> faceAuthCA = FaceAuthCA::GetInstance();
    if (faceAuthCA == nullptr) {
        FACEAUTH_HILOGE(MODULE_SERVICE, "get FaceAuthCA instance failed");
        return;
    }

    for (auto templateId : templateIdList) {
        if (faceAuthCA->FreezeTemplate(templateId) != FA_RET_OK) {
            FACEAUTH_HILOGE(MODULE_SERVICE, "FreezeTemplate failed");
        }
    }
}

void FaceAuthManager::SetBufferProducer(sptr<IBufferProducer> &producer)
{
    this->producer_ = producer;
    FACEAUTH_HILOGI(MODULE_SERVICE, "set buffer producer %{public}s", getPointerNullString(this->producer_).c_str());
}

sptr<IBufferProducer> FaceAuthManager::GetBufferProducer()
{
    FACEAUTH_HILOGI(MODULE_SERVICE, "get buffer producer %{public}s", getPointerNullString(this->producer_).c_str());
    return this->producer_;
}
} // namespace FaceAuth
} // namespace UserIAM
} // namespace OHOS
