//
//  ParseController.cpp
//  OmsiDisplay
//
//  Created by Leif Shackelford on 1/16/15.
//  Copyright (c) 2015 Tim Knapen. All rights reserved.
//

#include "ParseController.h"

#include <curl/curl.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <list>

#include <Poco/Environment.h>

#include "ViewModel.h"

// KEYS

const string ParseController::appId {"h3LKEM9csMjpD1fNCMvn8OGGbJ0BB6ssNG3el2Ju"};
const string ParseController::clientId {"w42LMhUbfVEPE2dIou9Q7LzR7l1rJsmzRQv5ZjXU"};
const string ParseController::restKey {"RF0IgEBwHJ3X5LrsuhRc38xEH0TakJFoXGHXXWQS"};

// PREFIX

//curl -X POST \
//-H "X-Parse-Application-Id: h3LKEM9csMjpD1fNCMvn8OGGbJ0BB6ssNG3el2Ju" \
//-H "X-Parse-REST-API-Key: RF0IgEBwHJ3X5LrsuhRc38xEH0TakJFoXGHXXWQS" \
//-H "Content-Type: application/json" \
//-d '{"deviceId":"1337","zone":"theory"}' \
//https://api.parse.com/1/classes/Display


//myAppID:javascript-key=myJavaScriptKey

const string ParseController::parseDomain {"https://api.parse.com"};
const string ParseController::objectPrefix {"/1/classes/"};
const string ParseController::filePrefix {"/1/files/"};
const string ParseController::functionPrefix {"/1/functions/"};

// CURL HELPERS
string contentTypeString(int type){
    switch (type) {
        case 0:
            return "Content-Type: application/json";

        case 1:
            return "Content-Type: image/jpeg";
            
        default:
            break;
    }
}

curl_slist* commonHeaders(int type = 0){
    
    vector<string> header;
    
    header.push_back("X-Parse-Application-Id: h3LKEM9csMjpD1fNCMvn8OGGbJ0BB6ssNG3el2Ju");
    header.push_back("X-Parse-REST-API-Key: RF0IgEBwHJ3X5LrsuhRc38xEH0TakJFoXGHXXWQS");
    header.push_back(contentTypeString(type));
    
    curl_slist *headers=NULL; // init to NULL is important
    for (auto& str : header) {
        //printf("HEADER += %s \n", str.c_str());
        headers = curl_slist_append(headers, str.c_str());
    }
    
    return headers;
}

struct CurlRepsonseString {
    char *ptr;
    size_t len;
};

void init_curl_response_string(struct CurlRepsonseString *s) {
    s->len = 0;
    s->ptr = (char*)malloc(s->len+1);
    if (s->ptr == NULL) {
        fprintf(stderr, "malloc() failed\n");
        exit(EXIT_FAILURE);
    }
    s->ptr[0] = '\0';
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, struct CurlRepsonseString *s)
{
    size_t new_len = s->len + size*nmemb;
    s->ptr = (char*)realloc(s->ptr, new_len+1);
    if (s->ptr == NULL) {
        fprintf(stderr, "realloc() failed\n");
        exit(EXIT_FAILURE);
    }
    memcpy(s->ptr+s->len, ptr, size*nmemb);
    s->ptr[new_len] = '\0';
    s->len = new_len;
    
    return size*nmemb;
}

// PARSE CONTROLLER

shared_ptr<Json::Value> ParseController::apiCall(string requestType, string urlSuffix, shared_ptr<Json::Value> json, int contentType){
    
    CURLcode response;
    
    struct CurlRepsonseString data;
    init_curl_response_string(&data);
    
    CURL *curl = curl_easy_init();
    
    //    URL	HTTP Verb	Functionality
    //    /1/classes/<className>	POST	Creating Objects
    //    /1/classes/<className>/<objectId>	GET	Retrieving Objects
    //    /1/classes/<className>/<objectId>	PUT	Updating Objects
    //    /1/classes/<className>	GET	Queries
    //    /1/classes/<className>/<objectId>	DELETE	Deleting Objects
    
    string url = ParseController::parseDomain + urlSuffix;//ParseController::objectPrefix + className;
    
    //printf("curl request : \n %s \n", url.c_str());
    
    curl_slist *headers = commonHeaders(contentType);
    
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    
    curl_easy_setopt(curl, CURLOPT_URL,url.c_str());
    
    char* raw = nullptr;
    
    if (json) {
        raw = strdup(json->toRawString().c_str());
    }
    
    //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
    
    //    if (requestType == "POST"){
    //        curl_easy_setopt(curl, CURLOPT_POST, 1L);
    //        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, raw);
    //    }
    //    else if (requestType == "PUT"){
    //        //curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    //        curl_easy_setopt(curl, CURLOPT_POST, 1L);
    //        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, raw);
    //    }
    if (requestType.length()) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, requestType.c_str());
    }
    if (raw) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, raw);
    }
    
    
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
    
    
    response = curl_easy_perform(curl);
    
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    
    if(CURLE_OK == response) {
        //printf("response : %s \n", data.ptr);
        shared_ptr<Json::Value> ret = make_shared<ofxJSONElement>(string(data.ptr));
        if (raw) free(raw);
        free(data.ptr);
        return ret;
    }
    
    if (raw) free(raw);
    free(data.ptr);
    return nullptr;
}

shared_ptr<Json::Value> ParseController::apiCall(string requestType, ofFile& file, int contentType){
    
    CURLcode response;
    
    struct CurlRepsonseString data;
    init_curl_response_string(&data);
    
    CURL *curl = curl_easy_init();
    
    //    URL	HTTP Verb	Functionality
    //    /1/classes/<className>	POST	Creating Objects
    //    /1/classes/<className>/<objectId>	GET	Retrieving Objects
    //    /1/classes/<className>/<objectId>	PUT	Updating Objects
    //    /1/classes/<className>	GET	Queries
    //    /1/classes/<className>/<objectId>	DELETE	Deleting Objects
    
    string path = file.getFileName();

    printf("send file %s \n", path.c_str());
    
    string url = ParseController::parseDomain +  ParseController::filePrefix + path ;//ParseController::objectPrefix + className;
    
    //printf("curl request : \n %s \n", url.c_str());
    
    curl_slist *headers = commonHeaders(contentType);
    
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    
    curl_easy_setopt(curl, CURLOPT_URL,url.c_str());
    
    
    // LOOK AT FILE
    
    struct stat file_info;
    double speed_upload, total_time;
    
    FILE *fd;
    
    fd = fopen(file.getAbsolutePath().c_str(), "rb"); /* open file to upload */
    if(!fd) {
        
        return nullptr; /* can't continue */
    }
    
    /* to get the file size */
    if(fstat(fileno(fd), &file_info) != 0) {
        
        return nullptr; /* can't continue */
    }
    
    
    //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
    
    if (requestType.length()) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, requestType.c_str());
    }
    
    /* set where to read from (on Windows you need to use READFUNCTION too) */
    curl_easy_setopt(curl, CURLOPT_READDATA, fd);
    
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    
    /* and give the size of the upload (optional) */
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,
                     (curl_off_t)file_info.st_size);
    
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
    
    response = curl_easy_perform(curl);
    
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    fclose(fd);
    
    if(CURLE_OK == response) {
        //printf("response : %s \n", data.ptr);
        shared_ptr<Json::Value> ret = make_shared<ofxJSONElement>(string(data.ptr));
        free(data.ptr);
        return ret;
    }
    
    free(data.ptr);
    return nullptr;
}

shared_ptr<Json::Value> ParseController::fetchAll(string className){
    
    string url = ParseController::objectPrefix + className;
    
    shared_ptr<Json::Value> result = apiCall("GET", url, nullptr);
    
    if (result) {
        return result;
    }
    else {
        printf("fetch failed \n");
    }
    return nullptr;
}

shared_ptr<Json::Value> ParseController::callFunction(string functionName, shared_ptr<Json::Value> data){
    
    string url = ParseController::functionPrefix + functionName;
    
    //printf("send updated object %s \n", data->toStyledString().c_str());
    
    shared_ptr<Json::Value> result = apiCall("", url, data);
    
    if (result) {
        return result;
    }
    else {
        printf("update failed \n");
    }
    return nullptr;
}

shared_ptr<Json::Value> ParseController::uploadFile(ofFile& file){
    
    shared_ptr<Json::Value> result = apiCall("POST", file);
    
    if (result) {
        return result;
    }
    else {
        printf("upload failed \n");
    }
    return nullptr;
    
}

shared_ptr<Json::Value> ParseController::updateObject(shared_ptr<ViewModel> object){
    
    string url = ParseController::objectPrefix +  object->className() + "/" +  object->objectId;
    
    auto data = make_shared<Value>(object->updatedKeys());
    
    //printf("send updated object %s \n", data->toStyledString().c_str());
    
    shared_ptr<Json::Value> result = apiCall("PUT", url, data);
    
    if (result) {
        return result;
    }
    else {
        printf("update failed \n");
    }
    return nullptr;
}

shared_ptr<Json::Value> ParseController::createNewDisplay(string deviceId){
    
    curl_global_init(CURL_GLOBAL_ALL);
    
    auto url = ParseController::objectPrefix + "Display";
    
    Json::Value value(Json::objectValue); // DICT
    value["deviceId"] = deviceId;
    value["zone"] = "newdevices";
    
    auto data = make_shared<Json::Value>(value);
    
    //printf("display object: %s\n", data->toStyledString().c_str());
    
    auto result = apiCall("POST", url, data);
    
    if (result) {
        return result;
    }
    else {
        printf("post failed \n");
    }
    
    return nullptr;
    
    //
    //
    //
    //    std::ostringstream buf;
    //
    //    curl_writer writer(buf);
    //
    //    curl_easy request;
    //
    //
    //
    //    curl_slist *headers = commonHeaders();
    //
    //    try {
    //        // Header
    //
    //        request.add(curl_pair<CURLoption,string>(CURLOPT_URL,url));
    //
    //        request.add(curl_pair<CURLoption, curl_slist*>(CURLOPT_HTTPHEADER, headers));
    //
    //        request.add(curl_pair<CURLoption,string>(CURLOPT_POSTFIELDS, newDisplay.getRawString(false)));
    //
    //        //printf("perform fetch ! \n");
    //
    //        request.perform();
    //
    //    } catch (curl_easy_exception error) {
    //        // Print errors, if any
    //        error.print_traceback();
    //    }
    //
    //    curl_slist_free_all(headers);
    //
    //    return ofxJSONElement(buf.str());
}

shared_ptr<Json::Value> ParseController::post(){
    
    string url = ParseController::parseDomain + ParseController::objectPrefix + "Display";
    
    printf("REQUEST URL : %s \n", url.c_str());
    
    vector<string> header;
    
    header.push_back("X-Parse-Application-Id: h3LKEM9csMjpD1fNCMvn8OGGbJ0BB6ssNG3el2Ju");
    header.push_back("X-Parse-REST-API-Key: RF0IgEBwHJ3X5LrsuhRc38xEH0TakJFoXGHXXWQS");
    header.push_back("Content-Type: application/json");
    
    curl_slist *headers=NULL; // init to NULL is important
    for (auto& str : header) {
        //printf("HEADER += %s \n", str.c_str());
        headers = curl_slist_append(headers, str.c_str());
    }
    
    // http://curl.haxx.se/libcurl/c/http-post.html
    
    CURL *curl;
    CURLcode res;
    
    /* In windows, this will init the winsock stuff */
    curl_global_init(CURL_GLOBAL_ALL);
    
    /* get a curl handle */
    curl = curl_easy_init();
    if(curl) {
        /* First set the URL that is about to receive our POST. This URL can
         just as well be a https:// URL if that is what should receive the
         data. */
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.parse.com/1/classes/Display");
        /* Now specify the POST data */
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "{\"deviceId\":\"1337\",\"zone\":\"theory\"}");
        
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        
        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));
        
        /* always cleanup */
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    
    curl_slist_free_all(headers);
    
    return result;
}

//
// Linux
//
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <unistd.h>

string ParseController::getMacAddress() {
    
    char name[256];
    
    struct ifreq ifr;
    
    int s = socket(PF_INET, SOCK_DGRAM, 0);
    //if (s == -1) throw std::exception("cannot open socket");
    
    strcpy(ifr.ifr_name, "eth0");
    int rc = ioctl(s, SIOCGIFHWADDR, &ifr);
    close(s);
    //if (rc < 0) throw SystemException("cannot get MAC address");
    struct sockaddr* sa = reinterpret_cast<struct sockaddr*>(&ifr.ifr_addr);
    std::memcpy(name, sa->sa_data, sizeof(name));
    
    return string(name);
}

////                Here's an example of a simple HTTP request to get google web page, using the curl_easy interface:
//
//#include "curl_easy.h"
//
//using curl::curl_easy;
//
//int main(int argc, const char **argv) {
//    curl_easy easy;
//    easy.add(curl_pair<CURLoption,string>(CURLOPT_URL,"http://www.google.it") );
//    easy.add(curl_pair<CURLoption,long>(CURLOPT_FOLLOWLOCATION,1L));
//    try {
//        easy.perform();
//    } catch (curl_easy_exception error) {
//        // If you want to get the entire error stack we can do:
//        vector<pair<string,string>> errors = error.what();
//        // Otherwise we could print the stack like this:
//        error.print_traceback();
//        // Note that the printing the stack will erase it
//    }
//    return 0;
//}
//// Here's instead, the creation of an HTTPS POST login form:
//
//#include "curl_easy.h"
//#include "curl_form.h"
//
//using curl::curl_form;
//using curl::curl_easy;
//
//int main(int argc, const char * argv[]) {
//    curl_form form;
//    curl_easy easy;
//
//    // Forms creation
//    curl_pair<CURLformoption,string> name_form(CURLFORM_COPYNAME,"user");
//    curl_pair<CURLformoption,string> name_cont(CURLFORM_COPYCONTENTS,"you username here");
//    curl_pair<CURLformoption,string> pass_form(CURLFORM_COPYNAME,"passw");
//    curl_pair<CURLformoption,string> pass_cont(CURLFORM_COPYCONTENTS,"your password here");
//
//    try {
//        // Form adding
//        form.add(name_form,name_cont);
//        form.add(pass_form,pass_cont);
//
//        // Add some options to our request
//        easy.add(curl_pair<CURLoption,string>(CURLOPT_URL,"your url here"));
//        easy.add(curl_pair<CURLoption,bool>(CURLOPT_SSL_VERIFYPEER,false));
//        easy.add(curl_pair<CURLoption,curl_form>(CURLOPT_HTTPPOST,form));
//        easy.perform();
//    } catch (curl_easy_exception error) {
//        // Print errors, if any
//        error.print_traceback();
//    }
//    return 0;
//}
//// And if we would like to put the returned content in a file? Nothing easier than:
//
//#include <iostream>
//#include "curl_easy.h"
//#include <fstream>
//
//using std::cout;
//using std::endl;
//using std::ofstream;
//using curl::curl_easy;
//
//int main(int argc, const char * argv[]) {
//    // Create a file
//    ofstream myfile;
//    myfile.open ("/Users/Giuseppe/Desktop/test.txt");
//    // Create a writer to handle the stream
//
//    curl_writer writer(myfile);
//    // Pass it to the easy constructor and watch the content returned in that file!
//    curl_easy easy(writer);
//
//    // Add some option to the easy handle
//    easy.add(curl_pair<CURLoption,string>(CURLOPT_URL,"http://www.google.it") );
//    easy.add(curl_pair<CURLoption,long>(CURLOPT_FOLLOWLOCATION,1L));
//    try {
//        easy.perform();
//    } catch (curl_easy_exception error) {
//        // If you want to get the entire error stack we can do:
//        vector<pair<string,string>> errors = error.what();
//        // Otherwise we could print the stack like this:
//        error.print_traceback();
//    }
//    myfile.close();
//    return 0;
//}
//// I have implemented a sender and a receiver to make it easy to use send/receive without handling buffers. For example, a very simple send/receiver would be:
//
//#include "curl_easy.h"
//#include "curl_form.h"
//#include "curl_pair.h"
//#include "curl_receiver.h"
//#include "curl_sender.h"
//
//using curl::curl_form;
//using curl::curl_easy;
//using curl::curl_sender;
//using curl::curl_receiver;
//
//int main(int argc, const char * argv[]) {
//    // Simple request
//    string request = "GET / HTTP/1.0\r\nHost: example.com\r\n\r\n";
//
//    // Creation of easy object.
//    curl_easy easy;
//    try {
//        easy.add(curl_pair<CURLoption,string>(CURLOPT_URL,"http://example.com"));
//        // Just connect
//        easy.add(curl_pair<CURLoption,bool>(CURLOPT_CONNECT_ONLY,true));
//        easy.perform();
//    } catch (curl_easy_exception error) {
//        // If you want to get the entire error stack we can do:
//        vector<pair<string,string>> errors = error.what();
//        // Print errors if any
//        error.print_traceback();
//    }
//
//    // Creation of a sender. You should wait here using select to check if socket is ready to send.
//    curl_sender<string> sender(easy);
//    sender.send(request);
//    // Prints che sent bytes number.
//    cout<<"Sent bytes: "<<sender.get_sent_bytes()<<endl;
//
//    for(;;) {
//        // You should wait here to check if socket is ready to receive
//        try {
//            // Create a receiver
//            curl_receiver<char,1024> receiver;
//            // Receive the content on the easy handler
//            receiver.receive(easy);
//            // Prints the received bytes number.
//            cout<<"Receiver bytes: "<<receiver.get_received_bytes()<<endl;
//        } catch (curl_easy_exception error) {
//            // If any errors occurs, exit from the loop
//            break;
//        }
//    }
//    return 0;
//}


// JSON EXAMPLES

//// creating a JSON Object and filling it with values
//Json::Value Contact::ToJson() const {
//    Json::Value value(Json::objectValue);
//    value["name"] = name_;
//    value["phone_number"] = phone_number_;
//    return value;
//}
//#include "json/json.h"
//
//// saving a vector of objects into a file as JSON
//void AddressBook::JsonSave(const char* filename) {
//    ofstream out(filename, ofstream::out);
//    Json::Value book_json(Json::objectValue), contacts_json(Json::arrayValue);
//    for (vector<Contact>::iterator it = contacts_.begin(); it != contacts_.end(); ++it) {
//        contacts_json.append((*it).ToJson());
//    }
//    book_json["contacts"] = contacts_json;
//    out << book_json;
//    out.close();
//}
//
//// loading that vector of objects from a JSON file
//void AddressBook::JsonLoad(const char* filename) {
//    ifstream in(filename);
//    Json::Value book_json;
//    in >> book_json;
//    for (Json::Value::iterator it = book_json["contacts"].begin(); it != book_json["contacts"].end(); ++it) {
//        AddPerson((*it)["name"].asString(), (*it)["phone_number"].asString());
//    }
//    in.close();
//}


