#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <curl/curl.h>

using namespace std;

static size_t WriteCallBack(void* contents, size_t size, size_t memb, void* user)
{
    ((string*)user)->append((char*)contents, size* memb);
    return size* memb;
}

vector<string> &split(const string &s, char divider, vector<string> &elems)
{
    stringstream ss(s);
    string item;
    while(getline(ss, item, divider))
    {
        elems.push_back(item);
    }
    return elems;
}

vector<string> split(const string &s, char delim)
{
    vector<string> elems;
    split(s, delim, elems);
    return elems;
}

int main() {

    curl_global_init(CURL_GLOBAL_ALL);
    CURL* handle = curl_easy_init();
    string readBuffer;
    ofstream out;
    out.open("Text.txt");

    curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(handle, CURLOPT_URL, "https://www.google.com");
    curl_easy_setopt(handle, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteCallBack);
    curl_easy_setopt(handle, CURLOPT_HEADER, 1L);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_perform(handle);

    vector<string> bufferLines = split(readBuffer,'\n');
    for (int i = 0; i!=bufferLines.size(); i++)
    {
        if(bufferLines[i].find("date: ")!=string::npos && out.is_open())
        {
            out<<bufferLines[i];
        }
    }

    out.close();
    curl_easy_cleanup(handle);
    curl_global_cleanup();

    return 0;
}
