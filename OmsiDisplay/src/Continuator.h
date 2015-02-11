//
//  Continuator.h
//  OmsiDisplay
//
//  Created by Leif Shackelford on 1/19/15.
//  Copyright (c) 2015 Tim Knapen. All rights reserved.
//

#ifndef __OmsiDisplay__Continuator__
#define __OmsiDisplay__Continuator__

#include <stdio.h>
#include <cstdlib>
#include <iostream>
#include <functional>
#include <future>
#include <string>
#include <thread>

using namespace std;

template<class R, class A>
struct Continuator {
    virtual ~Continuator() {}
    virtual R andThen(function<R(A)> k) {}
};

//void asyncApi(function<void(string)> handler);

void asyncApi(string s, function<void(string)> handler);

struct AsyncApi : Continuator<void, string>
{
    AsyncApi(string s) : _s(s) {}
    
    void andThen(function<void(string)> k)
    {
        asyncApi(_s, k);
    }
    string _s;
};

// Example

//AsyncApi callApi;
//callApi.andThen([](string s)
//{
//    cout << s << endl;
//});

// FOR CONTINUING ON UI THREAD

//struct AsyncApi : Continuator<void, string>
//{
//    void andThen(function<void(string)> k)
//    {
//        TheQueue.registerHandler(k);
//        asyncApi([](string s)
//                 {
//                     TheQueue.post(s);
//                 });
//    }
//};


template<class R, class A>
struct Return : Continuator<R, A> {
    Return(A x) : _x(x) {}
    R andThen(function<R(A)> k) {
        return k(_x);
    }
    A _x;
};

template<class R, class A, class C>
struct Bind : Continuator<R, A>
{
    Bind(C & ktor, function<unique_ptr<Continuator<R, A>>(A)> rest)
    : _ktor(ktor), _rest(rest)
    {}
    void andThen(function<R(A)> k)
    {
        function<unique_ptr<Continuator<R, A>>(A)> rest = _rest;
        function<R(A)> lambda = [k, rest](A a)
        {
            return rest(a)->andThen(k);
        };
        _ktor.andThen(lambda);
    }
    C _ktor;
    function<unique_ptr<Continuator<R, A>>(A)> _rest;
};

struct And : Continuator<void, string>
{
    And(unique_ptr<Continuator<void, string>> & ktor1,
        unique_ptr<Continuator<void, string>> & ktor2)
    : _ktor1(move(ktor1)), _ktor2(move(ktor2))
    {}
    void andThen(function<void(pair<string, string>)> k)
    {
        _ktor1->andThen([this, k](string s1)
                        {
                            lock_guard<mutex> l(_mtx);
                            if (_s2.empty())
                                _s1 = s1;
                            else
                                k(make_pair(s1, _s2));
                        });
        _ktor2->andThen([this, k](string s2)
                        {
                            lock_guard<mutex> l(_mtx);
                            if (_s1.empty())
                                _s2 = s2;
                            else
                                k(make_pair(_s1, s2));
                        });
    }
    mutex _mtx;
    string _s1;
    string _s2;
    unique_ptr<Continuator<void, string>> _ktor1;
    unique_ptr<Continuator<void, string>> _ktor2;
};


//struct Loop : Continuator<void, string>
//{
//    Loop(string s) : _s(s) {}
//    
//    
//    void andThen(function<void(string)> k) {
//        cout << "Loop::andThen: " <<_s << endl;
//        Bind<void, string, AsyncApi>(AsyncApi(), [](string t)
//                                     {
//                                         return unique_ptr<Continuator> (new Loop(t));
//                                     }).andThen(k));
//    }
//    string _s;
//};
//
//struct LoopN : Continuator<void, string>
//{
//    LoopN(string s, int n) : _s(s), _n(n) {}
//    
//    
//    void andThen(function<void(string)> k)
//    {
//        cout << "[LoopN::andThen] " <<_s << " " << _n << endl;
//        int n = _n;
//        Bind<void, string, AsyncApi>(AsyncApi(),
//                                     [n](string s) -> unique_ptr<Continuator>
//                                     {
//                                         if (n > 0)
//                                             return unique_ptr<Continuator>(
//                                                                            new LoopN(s, n - 1));
//                                         else
//                                             return unique_ptr<Continuator> (
//                                                                             new Return<void, string>("Done!"));
//                                     }).andThen(k);
//    }
//    string _s;
//    int    _n;
//};

// TESTING

void testAsyncMain1();
void testAsyncMain2();

#endif /* defined(__OmsiDisplay__Continuator__) */
