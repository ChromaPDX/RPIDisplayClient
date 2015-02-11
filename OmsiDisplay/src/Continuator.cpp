//
//  Continuator.cpp
//  OmsiDisplay
//
//  Created by Leif Shackelford on 1/19/15.
//  Copyright (c) 2015 Tim Knapen. All rights reserved.
//

#include "Continuator.h"

//void testAsyncMain1()
//{
//    Loop("Loop: ").andThen([](string s)
//                           {
//                               cout << "Never happens: " << s << endl;
//                           });
//    // run counter in parallel
//    for(int i = 0; i < 200; ++i)
//    {
//        cout << i << endl;
//        this_thread::sleep_for(chrono::seconds(1));
//    }
//}

void asyncApi(string s, function<void(string)> handler)
{
    thread th([s, handler]()
              {
                  cout << "Started async\n";
                  this_thread::sleep_for(chrono::seconds(3));
                  handler("Done async " + s);
              });
    th.detach();
}


//void testAsyncMain2()
//{
//    AsyncApi callApi;
//    callApi.andThen([](string s)
//                    {
//                        cout << s << std::endl;
//                    });
//    // This code will be executed in parallel to async API
//    for(int i = 0; i < 200; ++i)
//    {
//        cout << i << endl;
//        this_thread::sleep_for(chrono::seconds(1));
//    }
//}
