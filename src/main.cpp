#define WIN32_LEAN_AND_MEAN
#include "mapped-hooks.hpp"
#include "Windows.h"
#include <gd.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdio>
#include <ctime>
#include "utils.hpp"

using namespace cocos2d;
using namespace cocos2d::extension;
//using namespace gd;

class BIUpdateManager : public CCNode {
    const char* BIurlRoot = "https://geometrydash.eu/mods/betterinfo/";

    //std::vector<std::string> resources;
    std::ofstream logStream;
    std::string version;
    std::string channel;
    bool isLoaded = false;
public:
    static BIUpdateManager* create() {
        auto ret = new BIUpdateManager();
        if (ret && ret->init()) {
            ret->autorelease();
        } else {
            delete ret;
            ret = nullptr;
        }
        return ret;
    }

    bool init() {
        bool init = CCNode::init();
        if(!init) return false;

        logStream.open(BIpath("log.txt"), std::ios_base::app);
        log("--------------------------");
        log("Loading BetterInfo Wrapper");

        return true;
    }

    void showFileWriteError(const std::string& file) {
        log("Failed to write: " + file);
        std::stringstream errorText;
        errorText << "Unable to write the following file: " << file << "\n\nMake sure you have enough disk space available and that Geometry Dash has permissions to write in the directory.\n\nIf the problem persists, you might want to look at the instructions for manual installation.";
        MessageBox(nullptr, errorText.str().c_str(), "BetterInfo", MB_ICONERROR | MB_OK);
    }

    void log(std::string status) {
        auto t = std::time(nullptr);
        struct tm timeinfo;
        localtime_s(&timeinfo, &t);
        logStream << "[" << std::put_time(&timeinfo, "%d-%m-%Y %H-%M-%S") << "] " << status << std::endl;
    }

    void updateAndLoad() {
        if(updateChannel() == "disabled") return;
        doVersionHttpRequest();
    }

    void trimString(std::string& string) {
        string.erase(0, string.find_first_not_of('\n'));
        string.erase(string.find_last_not_of('\n') + 1);
        string.erase(0, string.find_first_not_of('\r'));
        string.erase(string.find_last_not_of('\r') + 1);
        string.erase(0, string.find_first_not_of(' '));
        string.erase(string.find_last_not_of(' ') + 1);
    }

    std::string BIpath(const std::string& file) {
        std::stringstream pathStream;
        pathStream << CCFileUtils::sharedFileUtils()->getWritablePath2() << "betterinfo";
        std::filesystem::create_directory(pathStream.str());
        pathStream << "/" << file;
        return pathStream.str();
    }

    std::string resourcesPath(const std::string& file) {
        std::stringstream pathStream;
        pathStream << CCFileUtils::sharedFileUtils()->getWritablePath2() << "Resources/" << file;
        return pathStream.str();
    }

    std::string updateChannel() {
        std::ifstream channelStream(BIpath("channel.txt"));
        channelStream >> channel;
        channelStream.close();

        if(channel.empty()) {
            log("Resetting update channel");

            channel = "stable";

            dumpToFile(BIpath("channel.txt"), channel);
        }


        log("Current update channel is: " + channel);

        return channel;
    } 

    std::string channelUrl(const std::string& file) {
        std::stringstream urlStream;
        urlStream << BIurlRoot << channel << "/" << file;
        return urlStream.str();
    }

    std::string versionUrl(const std::string& file) {
        std::stringstream urlStream;
        urlStream << BIurlRoot << version << "/" << file;
        return urlStream.str();
    }

    std::string versionResourcesUrl(const std::string& file) {
        std::stringstream urlStream;
        urlStream << "resources/" << file;
        return versionUrl(urlStream.str());
    }

    void dumpToFile(const std::string& path, std::vector<char>* data) {
        std::ofstream fout(path, std::ios::out | std::ios::binary);
        fout.write((char*)&((*data)[0]), data->size());
        fout.close();
        if(!fout) showFileWriteError(path);
    }

    void dumpToFile(const std::string& path, std::string data) {
        std::ofstream fout(path, std::ios::out | std::ios::binary);
        fout.write(data.c_str(), data.size());
        fout.close();
        if(!fout) showFileWriteError(path);
    }

    void doHttpRequest(const std::string& url, SEL_HttpResponse pSelector) {
        CCHttpRequest* request = new CCHttpRequest;
        request->setUrl(url.c_str());
        request->setRequestType(CCHttpRequest::HttpRequestType::kHttpPost);
        request->setResponseCallback(this, pSelector);
        CCHttpClient::getInstance()->send(request);
        request->release();
    }

    void doHttpRequest(const std::string& url, SEL_HttpResponse pSelector, const std::string& userData) {
        CCHttpRequest* request = new CCHttpRequest;
        request->setUrl(url.c_str());
        request->setRequestType(CCHttpRequest::HttpRequestType::kHttpPost);
        request->setResponseCallback(this, pSelector);
        std::string* userDataReal = new std::string(userData);
        request->setUserData(userDataReal);
        CCHttpClient::getInstance()->send(request);
        request->release();
    }

    void doVersionHttpRequest() {
        doHttpRequest(channelUrl("version.txt"), httpresponse_selector(BIUpdateManager::onVersionHttpResponse));
    }

    void onVersionHttpResponse(CCHttpClient* client, CCHttpResponse* response) {
        if(!(response->isSucceed())) {
            log("Getting current version of BetterInfo failed - error code: " + std::to_string(response->getResponseCode()));
            //MessageBox(nullptr, std::to_string(response->getResponseCode()).c_str(), "onVersionHttpResponse", MB_OK);
            if(!isLoaded) MessageBox(nullptr, "Unable to start downloading all required files to load BetterInfo.\n\nPlease make sure that you are connected to the internet and that Geometry Dash is able to access it.\n\nIf the problem persists, you might want to look at the instructions for manual installation.", "BetterInfo", MB_ICONERROR | MB_OK);
            
            return;
        }

        std::vector<char>* responseData = response->getResponseData();
        version = std::string(responseData->begin(), responseData->end());
        trimString(version);

        std::ifstream versionStream(BIpath("version.txt"));
        std::string installedVersion;
        versionStream >> installedVersion;
        //MessageBox(nullptr, installedVersion.c_str(), "installedVersion", MB_OK);
        versionStream.close();

        log("Installed Version: " + installedVersion + ", current version: " + version);

        if(installedVersion.empty() || installedVersion != version || !std::filesystem::exists(BIpath("betterinfo.dll"))) doUpdateHttpRequest();
        doResourcesHttpRequest();
    }

    void doUpdateHttpRequest() {
        doHttpRequest(versionUrl("betterinfo.dll"), httpresponse_selector(BIUpdateManager::onUpdateHttpResponse));
    }

    void onUpdateHttpResponse(CCHttpClient* client, CCHttpResponse* response) {
        if(!(response->isSucceed())) {
            log("Downloading betterinfo.dll failed - error code: " + std::to_string(response->getResponseCode()));
            if(!isLoaded) MessageBox(nullptr, "Unable to download all required files to load BetterInfo.\n\nPlease make sure that you are connected to the internet and that Geometry Dash is able to access it.\n\nIf the problem persists, you might want to look at the instructions for manual installation.", "BetterInfo", MB_ICONERROR | MB_OK);
            //MessageBox(nullptr, std::to_string(response->getResponseCode()).c_str(), "onUpdateHttpResponse", MB_OK);
            return;
        }

        std::vector<char>* responseData = response->getResponseData();

        dumpToFile(BIpath("betterinfo_updated.dll"), responseData);

        if(!isLoaded) loadBI();

        dumpToFile(BIpath("version.txt"), version);

    }

    void doResourcesHttpRequest() {
        doHttpRequest(versionUrl("resources.txt"), httpresponse_selector(BIUpdateManager::onResourcesHttpResponse));
    }

    void onResourcesHttpResponse(CCHttpClient* client, CCHttpResponse* response) {
        if(!(response->isSucceed())) {
            log("Downloading resource list failed - error code: " + std::to_string(response->getResponseCode()));
            //MessageBox(nullptr, std::to_string(response->getResponseCode()).c_str(), "onResourcesHttpResponse", MB_OK);
            return;
        }

        std::vector<char>* responseData = response->getResponseData();
        std::string responseString(responseData->begin(), responseData->end());
        std::stringstream responseStream(responseString);

        for(std::string resource; std::getline(responseStream, resource); ) {

            trimString(resource);
            verifyResource(resource);
        }

    }

    void verifyResource(const std::string& resource) {
        std::string pngPath(
            CCFileUtils::sharedFileUtils()->fullPathForFilename(
                resource.c_str(), false
            )
        );

        if(!(CCFileUtils::sharedFileUtils()->isFileExist(pngPath))) {
            doResourceDownloadHttpRequest(resource);
        }
    }

    void doResourceDownloadHttpRequest(const std::string& resource){
        doHttpRequest(versionResourcesUrl(resource), httpresponse_selector(BIUpdateManager::onResourceDownloadHttpResponse), resource);
    }

    void onResourceDownloadHttpResponse(CCHttpClient* client, CCHttpResponse* response){
        auto userData = reinterpret_cast<std::string*>(response->getHttpRequest()->getUserData());

        if(!(response->isSucceed())) {
            log("Downloading resource " + *userData + " failed - error code: " + std::to_string(response->getResponseCode()));
            delete userData;
            //MessageBox(nullptr, std::to_string(response->getResponseCode()).c_str(), "onResourceDownloadHttpResponse", MB_OK);
            return;
        }

        std::vector<char>* responseData = response->getResponseData();

        dumpToFile(resourcesPath(*userData), responseData);
        delete userData;

        //MessageBox(nullptr, filenameStream.str().c_str(), "onResourceDownloadHttpResponse", MB_OK);

    }

    void loadBI() {
        
        if(std::filesystem::exists(BIpath("betterinfo_updated.dll"))) {
            log("Found downloaded update, renaming dll");
            if(std::filesystem::exists(BIpath("betterinfo.dll"))) std::filesystem::remove(BIpath("betterinfo.dll"));
            std::filesystem::rename(BIpath("betterinfo_updated.dll"), BIpath("betterinfo.dll"));
        }

        isLoaded = (LoadLibrary(BIpath("betterinfo.dll").c_str()) != nullptr);
        log(isLoaded ? "Loaded BetterInfo Mod" : "Failed to load BetterInfo Mod");
    }
};

DWORD WINAPI my_thread(void* hModule) {

    auto manager = BIUpdateManager::create();
    manager->retain();
    manager->loadBI();
    manager->updateAndLoad();

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
        CreateThread(0, 0, my_thread, hModule, 0, 0);
    return TRUE;
}