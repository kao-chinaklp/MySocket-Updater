#include <curl/curl.h>

#include <cstdio>
#include <string>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <fstream>
#include <iostream>

#include "json.hpp"

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
        printf("]%d%", static_cast<int>(progress*100.0));
        std::this_thread::sleep_for(milliseconds(100));
    }
    return 0;
}

string GetCurrentVersion(){
    ifstream file;
    string line, key, value;
    string Version;
    file.open("config.ini", std::ios::in);
    if(file.fail())return "";
    while(getline(file, line)){
        int Idx=line.find('=');
        if(Idx==-1)continue;
        int EndIdx=line.find('\n', Idx);
        key=line.substr(0, Idx);
        value=line.substr(Idx+1, EndIdx-Idx-1);
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

inline void quit(){
    #ifdef _WIN32
    system("pause");
    #endif
}

CURLcode FileDownload(string url, string FileName){
    FILE* File=nullptr;
    CURLcode res;
    File=fopen(FileName.c_str(), "wb");
    if(File==nullptr){
        printf("无法写入文件！\n");
        quit();
        exit(0);
    }
    if(!init()){
        quit();
        exit(0);
    }
    for(int i=1;i<=3;i++){
        printf("正在尝试连接服务器(%d/3)...\n", i);
        curl_easy_setopt(curl, CURLOPT_URL, DownloadUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE, ftell(File));
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, ProgressCallback);// 进度条动画
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, DownloadCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, File);
        res=curl_easy_perform(curl);
        if(res==CURLE_OK)break;
    }
    fclose(File);
    curl_easy_cleanup(curl);
    printf("\n");
    return res;
}

int main(){
    if(!init()){
        quit();
        return 0;
    }
    CURLcode res;
    ifstream ifile;
    string CurrentVersion=GetCurrentVersion();
    // 这里的token有时限，请自行更新
    string LatestVersion=GetLatestVersion("kao-chinaklp", "MySocket", "<your token>");
    if(LatestVersion.empty()){
        quit();
        return 0;
    }
    if(!CheckVersion(LatestVersion, CurrentVersion))goto _end;
    if((res=FileDownload(DownloadUrl, "MySocket.exe"))!=CURLE_OK){
        printf("更新失败！%s\n", curl_easy_strerror(res));
        quit();
        return 0;
    }
    printf("更新成功！\n");
    _end:
    quit();
    return 0;
}
