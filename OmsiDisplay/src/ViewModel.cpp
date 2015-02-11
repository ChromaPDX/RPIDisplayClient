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

//shared_ptr<Playlist> Playlist::fromJson(Value& json) {
//    shared_ptr<Playlist> playlist = make_shared<Playlist>(json);
//    return playlist;
//}