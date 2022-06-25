#define WIN32_LEAN_AND_MEAN
#include "Windows.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <winsock2.h>
#include <curl/curl.h>

class Updater { 
public:
    std::ofstream logStream;
    std::string channel;
    std::string version;
    bool shownDownloadError = false;
    bool isLoaded = false;

    struct HttpResponse {
        std::string header;
        std::string content;
        CURLcode curlCode;
        long responseCode;
    };

    const char* BIurlRoot = "https://geometrydash.eu/mods/betterinfo/v1/";

    /**
     * Path/URL helper functions
     */
    std::string BIpath(const std::string& file) {
        std::stringstream pathStream;
        pathStream << "betterinfo";
        std::filesystem::create_directory(pathStream.str());
        pathStream << "/" << file;
        return pathStream.str();
    }

    std::string resourcesPath(const std::string& file) {
        std::stringstream pathStream;
        pathStream << "Resources/" << file;
        return pathStream.str();
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

    /**
     * String helper functions
     */
    void trimString(std::string& string) {
        string.erase(0, string.find_first_not_of('\n'));
        string.erase(string.find_last_not_of('\n') + 1);
        string.erase(0, string.find_first_not_of('\r'));
        string.erase(string.find_last_not_of('\r') + 1);
        string.erase(0, string.find_first_not_of(' '));
        string.erase(string.find_last_not_of(' ') + 1);
    }

    void log(std::string status) {
        auto t = std::time(nullptr);
        struct tm timeinfo;
        localtime_s(&timeinfo, &t);
        logStream << "[" << std::put_time(&timeinfo, "%d-%m-%Y %H-%M-%S") << "] " << status << std::endl;
    }

    /**
     * Error helper functions
     */
    static void showCriticalError(const char* content) {
        MessageBox(nullptr, content, "BetterInfo - Geometry Dash", MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
    }

    void showFileWriteError(const std::string& file) {
        log("Failed to write: " + file);
        std::stringstream errorText;
        errorText << "Unable to write the following file: " << file << "\n\nMake sure you have enough disk space available and that Geometry Dash has permissions to write in the directory.\n\nIf the problem persists, you might want to look at the instructions for manual installation.";
        showCriticalError(errorText.str().c_str());
    }

    void showDownloadError() {
        if(!shownDownloadError) showCriticalError("Unable to download all required files to load BetterInfo.\n\nPlease make sure that you are connected to the internet and that Geometry Dash is able to access it.\n\nIf the problem persists, you might want to look at the instructions for manual installation.");
        shownDownloadError = true;
    }

    /**
     * CURL helper functions
     */
    static size_t writeData(void *ptr, size_t size, size_t nmemb, std::string* data) {
        data->append((char*) ptr, size * nmemb);
        return size * nmemb;
    }

    HttpResponse sendWebRequest(const std::string& url) {
        auto curl = curl_easy_init();
        if(!curl) {
            if(!isLoaded) showCriticalError("Failed to initialize curl, as a result files required to load BetterInfo won't be downloaded.\n\nIf the problem persists, you might want to look at the instructions for manual installation.");
            log("Failed to initialize curl");
            return {"", "", CURLE_FAILED_INIT, 0};
        }

        HttpResponse ret;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeData);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &(ret.content));
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &(ret.header));
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE);
        curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, TRUE);

        ret.curlCode = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &(ret.responseCode));

        if(ret.content.size() == 0) {
            log("Error: Empty file received");
            ret.curlCode = CURLE_HTTP_RETURNED_ERROR;
        }

        if(ret.content.rfind("<html") == 0) {
            log("Error: Invalid content - HTML detected");
            ret.curlCode = CURLE_HTTP_RETURNED_ERROR;
        }

        if(ret.content.rfind("<!DOCT", 0) == 0) {
            log("Error: Invalid content - DOCTYPE detected");
            ret.curlCode = CURLE_HTTP_RETURNED_ERROR;
        }

        curl_easy_cleanup(curl);
        log(url + ": " + std::to_string(ret.responseCode));
        return ret;
    }

    /**
     * Updater logic
     */
    void dumpToFile(const std::string& path, std::string data) {
        std::ofstream fout(path, std::ios::out | std::ios::binary);
        fout.write(data.c_str(), data.size());
        fout.close();
        if(!fout) showFileWriteError(path);
    }

    std::string updateChannel() {
        if(!channel.empty()) return channel;

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

    bool loadBI() {
        
        if(std::filesystem::exists(BIpath("betterinfo_updated.dll"))) {
            log("Found downloaded update, renaming dll");
            if(std::filesystem::exists(BIpath("betterinfo.dll"))) std::filesystem::remove(BIpath("betterinfo.dll"));
            std::filesystem::rename(BIpath("betterinfo_updated.dll"), BIpath("betterinfo.dll"));
        }

        isLoaded = (LoadLibrary(BIpath("betterinfo.dll").c_str()) != nullptr);
        log(isLoaded ? "Loaded BetterInfo Mod" : "Failed to load BetterInfo Mod");
        return isLoaded;
    }

    bool loadMinhook() {
        return LoadLibrary("minhook.x32.dll") != nullptr || std::filesystem::exists("minhook.x32.dll");
    }

    std::string installedVersion() {
        std::ifstream versionStream(BIpath("version.txt"));
        std::string installedVersion;
        versionStream >> installedVersion;
        versionStream.close();
        return installedVersion;
    }

    bool resourceExists(const std::string& resource) {
        return std::filesystem::exists(resourcesPath(resource));
    }

    Updater() {
        logStream.open(BIpath("log.txt"), std::ios_base::app);
        log("--------------------------");
        log("Loading BetterInfo Wrapper");
        isLoaded = loadBI();

        if(updateChannel() == "disabled") return;

        /**
         * Try to load minhook and download if failed
         */
        if(!loadMinhook()) {
            auto response = sendWebRequest(channelUrl("minhook.txt"));
            if(response.curlCode != CURLE_OK) { if(!isLoaded) showDownloadError(); return; }
            response = sendWebRequest(response.content);
            if(response.curlCode != CURLE_OK) { if(!isLoaded) showDownloadError(); return; }
            dumpToFile("minhook.x32.dll", response.content);
            isLoaded = loadBI();
        }

        /**
         * Checking for new version
         */
        auto response = sendWebRequest(channelUrl("version.txt"));
        if(response.curlCode != CURLE_OK) { if(!isLoaded) showDownloadError(); return; }
        version = response.content;
        trimString(version);

        /**
         * Download new version if online version doesn't match offline version
         */
        std::string installedVersion(installedVersion());
        if(installedVersion.empty() || installedVersion != version || !std::filesystem::exists(BIpath("betterinfo.dll"))) {
            response = sendWebRequest(versionUrl("betterinfo.dll"));
            if(response.curlCode != CURLE_OK) { if(!isLoaded) showDownloadError(); return; }

            dumpToFile(BIpath("betterinfo_updated.dll"), response.content);
            if(!isLoaded) loadBI();

            dumpToFile(BIpath("version.txt"), version);
        }

        /**
         * Verify if all resources are present and download ones that aren't
         */
        response = sendWebRequest(versionUrl("resources.txt"));
        if(response.curlCode != CURLE_OK) return;
        std::stringstream responseStream(response.content);

        for(std::string resource; std::getline(responseStream, resource); ) {
            trimString(resource);
            if(resourceExists(resource)) continue;

            response = sendWebRequest(versionResourcesUrl(resource));
            if(response.curlCode != CURLE_OK) continue;

            dumpToFile(resourcesPath(resource), response.content);
        }
    }
};

DWORD WINAPI my_thread(void* hModule) {

    /**
     * Check if BI is already loaded, display error and exit if it is
     */
    char* filenameChar = new char[2048];
    GetModuleFileName((HMODULE) hModule, filenameChar, 2048);
    std::string filename(filenameChar);

    auto pos = filename.find_last_of("\\/");
    if(pos != std::string::npos) {
        filename = filename.substr(pos + 1);
    }

    Sleep(100); //this should be enough time for the dll to start existing
    if(filename != "betterinfo.dll" && GetModuleHandle("betterinfo.dll") != nullptr) {
        Updater::showCriticalError("betterinfo.dll is already loaded.\n\nMake sure you do NOT have your modloader set to load both betterinfo.dll and betterinfo-wrapper.dll\n\nOnly betterinfo-wrapper.dll is supposed to be installed!");
        return 0;
    }

    /**
     * Start the main update check and loading operation
     */
    Updater();

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
        CreateThread(0, 0, my_thread, hModule, 0, 0);
    return TRUE;
}