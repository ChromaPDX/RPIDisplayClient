//
//  Asset.cpp
//  OmsiDisplay
//
//  Created by Leif Shackelford on 1/27/15.
//  Copyright (c) 2015 Tim Knapen. All rights reserved.
//

#include "ViewModel.h"
#include "ParseController.h"
#include "URLImage.h"

shared_ptr<DisplayClient> DisplayClient::fromJson(const Value& json) {
    auto display = make_shared<DisplayClient>(json);
    return display;
}

shared_ptr<Asset> Asset::fromJson(const Value& json) {
    auto asset = make_shared<Asset>(json);
    return asset;
}

shared_ptr<PlaylistItem> PlaylistItem::fromJson(const Value& json) {
    auto playListItem = make_shared<PlaylistItem>(json);
    return playListItem;
}

shared_ptr<PlaylistItem> PlaylistItem::fromData(AssetType type_, string file_, int duration){
    return make_shared<PlaylistItem>(type_, file_, duration);
}

string Asset::md5name() {
    return md5(url + objectId);
}

string Asset::name() {
    string last = url.substr(url.find_last_of("/") + 1);
    return last.substr(0, last.find_last_of("?"));
}

string Asset::extension() {
    string last = url.substr(url.find_last_of(".") + 1);
    return last.substr(0, last.find_last_of("?"));
}

//shared_ptr<Playlist> Playlist::fromJson(Value& json) {
//    shared_ptr<Playlist> playlist = make_shared<Playlist>(json);
//    return playlist;
//}

/*
 returns the utc timezone offset
 (e.g. -8 hours for PST)
 */
int get_utc_offset() {
    
    time_t zero = 24*60*60L;
    struct tm * timeptr;
    int gmtime_hours;
    
    /* get the local time for Jan 2, 1900 00:00 UTC */
    timeptr = localtime( &zero );
    gmtime_hours = timeptr->tm_hour;
    
    /* if the local time is the "day before" the UTC, subtract 24 hours
     from the hours to get the UTC offset */
    if( timeptr->tm_mday < 2 )
        gmtime_hours -= 24;
    
    return gmtime_hours;
    
}

/*
 the utc analogue of mktime,
 (much like timegm on some systems)
 */
time_t tm_to_time_t_utc( struct tm * timeptr ) {
    
    /* gets the epoch time relative to the local time zone,
     and then adds the appropriate number of seconds to make it UTC */
    return mktime( timeptr ) + get_utc_offset() * 3600;
    
}

std::chrono::system_clock::time_point string_to_time_point(const std::string &str)
{
    using namespace std;
    using namespace std::chrono;
    
    int yyyy, mm, dd, HH, MM, SS, fff;
    
    char scanf_format[] = "%4d-%2d-%2dT%2d:%2d:%2d.%3d";
    
    sscanf(str.c_str(), scanf_format, &yyyy, &mm, &dd, &HH, &MM, &SS, &fff);
    
    tm ttm = tm();
    ttm.tm_year = yyyy - 1900; // Year since 1900
    ttm.tm_mon = mm - 1; // Month since January
    ttm.tm_mday = dd; // Day of the month [1-31]
    ttm.tm_hour = HH; // Hour of the day [00-23]
    ttm.tm_min = MM;
    ttm.tm_sec = SS;
    
    time_t ttime_t = timegm(&ttm);
    // gm instead of mktime
    
    system_clock::time_point time_point_result = std::chrono::system_clock::from_time_t(ttime_t);
    
    time_point_result += std::chrono::milliseconds(fff);
    return time_point_result;
}

std::string time_point_to_string(std::chrono::system_clock::time_point &tp)
{
    using namespace std;
    using namespace std::chrono;
    
    auto ttime_t = system_clock::to_time_t(tp);
    auto tp_sec = system_clock::from_time_t(ttime_t);
    milliseconds ms = duration_cast<milliseconds>(tp - tp_sec);
    
    std::tm * ttm = gmtime(&ttime_t);
    
    char date_time_format[] = "%Y-%m-%dT%H:%M:%S";
    
    char time_str[] = "yyyy.mm.dd.HH-MM.SS.fff";
    
    strftime(time_str, strlen(time_str), date_time_format, ttm);
    
    string result(time_str);
    
    result.append(".");
    int millis = ms.count();
    if (millis < 100) {
        result.append("0");
        if (millis < 10){
            result.append("0");
        }
    }
    result.append(to_string(millis));
    result.append("Z");
    
    return result;
}

std::string current_time_to_string(){
    auto now = std::chrono::high_resolution_clock::now();
    return time_point_to_string(now);
}

//Value time_point_to_json(std::chrono::system_clock::time_point &tp){
//    Value date;
//}

//string currentTimeAsString(){
//    struct tm tm;
//    char tmp[120];
//    time_t t;
//    time(&t);
//    localtime_r(&t,&tm);
//    sprintf(tmp, "%i-%02i-%02iT%02i:%02i:%02i", tm.tm_year + 1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
//    //printf("local time: %s \n", tmp);
//    return stringFromTime(t);
//}
//
//string stringFromTime(time_t t){
//    //strf method
//    char tmp[sizeof "2015-02-05T20:51:18Z"]; // PARSE UTC / ISO 8601
//    
//    strftime(tmp, sizeof tmp, "%Y-%m-%dT%H:%M:%SZ", gmtime(&t));
//    
////    // OR
////    struct tm tm;
////    gmtime(&tm);
////    int milli = curTime.tv_usec / 1000;
////    strftime(tmp, sizeof tmp, "%Y-%m-%dT%H:%M:%S", );
////    
////    char currentTime[84] = "";
////    sprintf(currentTime, "%s:%dZ", buffer, milli);
////    printf("current time: %s \n", currentTime);
//    
//    
//    // TM struct method
//    //    struct tm tm;
//    //    char tmp[120];
//    //    localtime_r(&t,&tm);
//    //    sprintf(tmp, "%i-%02i-%02iT%02i:%02i:%02i", tm.tm_year + 1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
//    return string(tmp);
//}
//
//time_t timeFromString(string& str){
//    //const char *timestr = "2010-11-04T23:23:01Z";
//    
//    struct tm t;
//    strptime(str.c_str(), "%Y-%m-%dT%H:%M:%S.%03dZ", &t);
//    
////    char buf[128];
////    strftime(buf, sizeof(buf), "%d %b %Y %H:%M:%S", &t);
//    
//    time_t in = mktime(&t);
//    
//    printf("time: %s -> %ld -> %s\n", str.c_str(), in, stringFromTime(in).c_str());
//    
//    return in;
//}