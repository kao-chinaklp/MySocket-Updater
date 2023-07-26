#include "json.hpp"

#include <cstdio>
#include <string>
#include <chrono>
#include <thread>
#include <fstream>
#include <iostream>
#include <curl/curl.h>

#ifdef _WIN32
#include <windows.h>
#endif

using std::string;
using std::ifstream;
using namespace std::chrono;

using json=nlohmann::json;

CURL* curl=nullptr;
string UserAgent;
string DownloadUrl;

bool init(){
    curl=curl_easy_init();
    UserAgent="Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/115.0.0.0 Safari/537.36 Edg/115.0.1901.183";
    curl_easy_setopt(curl, CURLOPT_USERAGENT, UserAgent.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
    if(!curl){
        printf("初始化失败！\n");
        return false;
    }
    return true;
}

size_t DownloadCallback(void* contents, size_t size, size_t nmemb, FILE* file) {
    return fwrite(contents, size, nmemb, file);
}

size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* output){
    size_t totalSize=size*nmemb;
    output->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

// 回调函数用于更新进度条
int ProgressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow){
    // 忽略上传进度
    (void)ulnow;
    (void)ultotal;
    if(dltotal>0){
        // 计算下载进度百分比
        double progress=static_cast<double>(dlnow)/static_cast<double>(dltotal);
        int barWidth=100; // 进度条宽度
        int pos=static_cast<int>(barWidth*progress);
        // 显示进度条
        printf("\r\033[K");
        printf("[");
        for (int i=0;i<barWidth;++i){
            if(i<pos)printf("=");
            else if(i==pos)printf(">");
            else printf(" ");
        }
        printf("]%d", int(progress*100.0));
        printf("%");
        std::this_thread::sleep_for(milliseconds(50));
    }
    return 0;
}

int DebugCallback(CURL *handle, curl_infotype type, char *data, size_t size, void *userptr) {
    string DebugData(data, size);
    printf("%s", DebugData.c_str());
    return size;
}

string GetCurrentVersion(){
    ifstream file;
    string line;
    string Version;
	file.open("config.ini", std::ios::in);
    if(file.fail())return "";
    while(getline(file, line)){
        int Idx=line.find('=');
        if(Idx==-1)continue;
        int EndIdx=line.find('\n', Idx);
        string key=line.substr(0, Idx);
        string value=line.substr(Idx+1, EndIdx-Idx-1);
        if(key=="version")Version=value;
    }
	file.close();
    return Version;
}

string GetLatestVersion(const string Owner, const string repo, const string token=""){
    string response;
    string url="https://api.github.com/repos/"+Owner+"/"+repo+"/releases/latest";
    string AccessToken="Authorization: token "+token;
    struct curl_slist* header=nullptr;
    header=curl_slist_append(header, AccessToken.c_str());
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    CURLcode res=curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if(res!=CURLE_OK){
        printf("无法获取更新！%s\n", curl_easy_strerror(res));
        return "";
    }
    json data=json::parse(response);
    DownloadUrl=string(data["assets"][0]["browser_download_url"]);
    return data["tag_name"];
}

bool CheckVersion(const string Latest, const string Current){
    if(Latest>Current){
        printf("存在更新版本，开始更新\n");
        return true;
    }
    else {
        printf("已经是最新版本了！\n");
        return false;
    }
}

int main(){
    #ifdef _WIN32
    system("chcp 65001");
    #endif
    if(!init())return 0;
    CURLcode res;
    string FileName="MySocket.exe";
    FILE* File=nullptr;
    string CurrentVersion=GetCurrentVersion();
    // 这里的token有时限，请自行更新
    string LatestVersion=GetLatestVersion("kao-chinaklp", "MySocket", "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");
    if(LatestVersion.empty())goto _end;
    if(!CheckVersion(LatestVersion, CurrentVersion))goto _end;
    File=fopen(FileName.c_str(), "w");
    if(File==nullptr){
        printf("无法写入文件！\n");
        goto _end;
    }
    if(!init())goto _end;
    curl_easy_setopt(curl, CURLOPT_URL, DownloadUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, DebugCallback);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);// 开启调试信息输出
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, ProgressCallback);// 进度条动画
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, DownloadCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, File);
    res=curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    printf("\n");
    if(res!=CURLE_OK){
        printf("更新失败！%s\n", curl_easy_strerror(res));
        goto _end;
    }
    printf("更新成功！\n");
    _end:
    fclose(File);
    #ifdef _WIN32
    system("pause");
    #endif
    return 0;
}