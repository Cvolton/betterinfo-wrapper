#include <iostream>
#include <filesystem>

const char* BIurlRoot = "https://geometrydash.eu/mods/betterinfo/v1/";

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