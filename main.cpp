#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <curl/curl.h>
#include <thread>
#include <unistd.h>
#include <chrono>

static size_t WriteCallBack(void*, size_t, size_t, void*);
std::vector<std::string> &split(const std::string&, char, std::vector<std::string>&);
std::vector<std::string> split(const std::string&, char);
void ExecuteRequest(CURL*, std::string&,const char*, std::ofstream&);
int SendError(CURLcode,std::ostream&,CURLcode,std::string);
void OnError(CURLcode,std::ostream&);
void ParseAndSetTime(std::ifstream&, std::string&);
void SetTime(std::string&, std::tm);
void CleanUp(CURL*, CURL*, std::ifstream&, std::ofstream&);

int main() {

    curl_global_init(CURL_GLOBAL_ALL);
    CURL* handle = curl_easy_init();
    CURL* curl_handle = curl_easy_init();
    std::string readBuffer,buffer;
    std::ofstream out; std::ifstream in;
    out.open("log.txt"); in.open("Password.txt");
    if(!out.is_open() || !in.is_open()){ return 1; }
    curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0L);

    ExecuteRequest(handle, readBuffer, "https://www.google.com", out);
    ParseAndSetTime(in, readBuffer);
    ExecuteRequest(curl_handle,buffer, "https://example.com", out);

    CleanUp(handle, curl_handle, in, out);
    return 0;
}

void ExecuteRequest(CURL* handle, std::string& buffer,const char* URLAddress, std::ofstream& out)
{
    std::thread th;
    CURLcode res;
    curl_easy_setopt(handle, CURLOPT_URL, URLAddress);
    curl_easy_setopt(handle, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteCallBack);
    curl_easy_setopt(handle, CURLOPT_HEADER, 1L);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &buffer);
    res=curl_easy_perform(handle);
    th=std::thread(OnError, res, ref(out));
    if(th.joinable())
    {
        th.join();
    }
}

int SendError(CURLcode errorCode,std::ostream& out,CURLcode value,std::string sendLine)
{
    if (errorCode == value)
    {
        out<<sendLine + ",error code: " + std::to_string(errorCode)<<std::endl;
    }
    exit(1);
}

void OnError(CURLcode errorCode,std::ostream& out)
{
    if(errorCode == CURLcode::CURLE_OK)
    {
        out<<"Successful request"<<std::endl;
    }
    if(errorCode == CURLE_SSL_CONNECT_ERROR) SendError(errorCode, out, CURLE_SSL_CONNECT_ERROR, "Try connecting with SSL");
    if(errorCode == CURLE_COULDNT_RESOLVE_HOST) SendError(errorCode, out, CURLE_COULDNT_RESOLVE_HOST, "Try to  resolve host name");
    if(errorCode == CURLE_PEER_FAILED_VERIFICATION) SendError(errorCode, out, CURLE_PEER_FAILED_VERIFICATION, "Peer verification");
}

void ParseAndSetTime(std::ifstream& in, std::string& readBuffer)
{
    auto bufferLines = split(readBuffer,'\n');
    std::string line;
    std::string passwordLine;
    for (const auto & bufferLine : bufferLines)
    {
        if(bufferLine.find("date: ")!=std::string::npos)
        {
            line=bufferLine;
        }
    }
    std::tm tm={};
    std::string password;
    strptime(line.c_str(), "date: %a, %d %b %Y %H:%M:%S GMT",&tm);
    auto tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    if(in.is_open())
    {
        while(getline(in, passwordLine))
        {
            password = passwordLine;
        }
    }
    SetTime(password, tm);
}

void SetTime(std::string& password,std::tm tm)
{
    std::string time = std::to_string(tm.tm_hour)+":"+std::to_string(tm.tm_min)+":"+std::to_string(tm.tm_sec);
    std::string yearMonthDay = std::to_string(tm.tm_year+1900)+"/"+std::to_string(tm.tm_mon+1)+"/"+std::to_string(tm.tm_mday);
    std::string date_str = "echo "+password+" | sudo -S sudo date +%Y%m%d -s"+yearMonthDay;
    std::string time_str ="echo "+password+" | sudo -S sudo date +%T -s"+time;
    system(date_str.c_str());
    system(time_str.c_str());
}

static size_t WriteCallBack(void* contents, size_t size, size_t memb, void* user)
{
    ((std::string*)user)->append((char*)contents, size* memb);
    return size* memb;
}

std::vector<std::string> &split(const std::string &s, char divider, std::vector<std::string> &elems)
{
    std::stringstream ss(s);
    std::string item;
    while(getline(ss, item, divider))
    {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> split(const std::string &s, char divider)
{
    std::vector<std::string> elems;
    split(s, divider, elems);
    return elems;
}

void CleanUp(CURL* handle, CURL* curl_handle, std::ifstream& in, std::ofstream& out)
{
    curl_easy_cleanup(handle);
    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();
    out.close();in.close();
}