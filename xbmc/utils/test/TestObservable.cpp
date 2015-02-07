/*
*      Copyright (C) 2015 Team XBMC
*      http://kodi.tv
*
*  This Program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This Program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with XBMC; see the file COPYING.  If not, see
*  <http://www.gnu.org/licenses/>.
*
*/

#include <string>
#include <vector>

#include "utils/Observable.h"

#include "gtest/gtest.h"

std::string observer1("Observer1");
std::string observer2("Observer2");

//Interface declaration specifying the events that can be generated
//and how to handle these events, which parameters are available
class GeneratorEvents
{
public:
  //Empty structs for event names to make use of tag dispatching
  struct TestEvent1 { };
  struct TestEvent2 { };
  struct TestEvent3 { };

  //With perfect forwarding we can have whatever type we want in our
  //events.
  //Avoid making any of the handlers abstract so an observer can
  //ignore everything except the ones it's interested in
  virtual void On(TestEvent1, const int a, const int b, int& c) { }
  virtual void On(TestEvent2, std::vector<std::string>& a) { }

  //Example of an abortable event handler, returning false stops
  //event processing and the remaining observers won't be notified
  virtual bool On(TestEvent3, std::vector<std::string>& a) { return true; }
};

//Example of how an Observable implementation might look like
class Generator : public IObservable<GeneratorEvents>
{
public:
  void Test1(std::vector<std::string>& a)
  {
    NotifyObservers(GeneratorEvents::TestEvent2(), a);
  }

  bool Test2(std::vector<std::string>& a)
  {
    return AskObservers(GeneratorEvents::TestEvent3(), a);
  }
};

class Observer1 : public GeneratorEvents
{
public:
  virtual void On(TestEvent2, std::vector<std::string>& a)
  {
    a.push_back(observer1);
  }

  virtual bool On(TestEvent3, std::vector<std::string>& a)
  {
    return false;
  }
};

class Observer2 : public GeneratorEvents
{
public:
  virtual void On(TestEvent2, std::vector<std::string>& a)
  {
    a.push_back(observer2);
  }

  virtual bool On(TestEvent3, std::vector<std::string>& a)
  {
    a.push_back(observer2);
    return true;
  }
};

TEST(TestObservable, Ordering)
{
  Generator gen;
  Observer1 obs1;
  Observer2 obs2;

  std::vector<std::string> v1;

  gen.AddObserver(&obs1);
  gen.AddObserver(&obs2);
  
  gen.Test1(v1);
  
  ASSERT_STREQ(observer1.c_str(), v1[0].c_str());
  ASSERT_STREQ(observer2.c_str(), v1[1].c_str());

  v1.clear();
  
  Generator gen2;
  gen2.AddObserver(&obs1);
  gen2.AddObserver(&obs2);
  
  ASSERT_FALSE(gen2.Test2(v1));
  ASSERT_TRUE(v1.empty());
}

TEST(TestObservable, AddObserverBefore)
{
  Generator gen;
  Observer1 obs1;
  Observer2 obs2;

  std::vector<std::string> v1;

  gen.AddObserver(&obs1);
  gen.AddObserverBefore(&obs2, &obs1);

  gen.Test1(v1);

  ASSERT_STREQ(observer2.c_str(), v1[0].c_str());
  ASSERT_STREQ(observer1.c_str(), v1[1].c_str());

  v1.clear();

  Generator gen2;
  gen2.AddObserver(&obs1);
  gen2.AddObserverBefore(&obs2, &obs1);

  ASSERT_FALSE(gen2.Test2(v1));
  ASSERT_EQ(1, v1.size());
  ASSERT_STREQ(observer2.c_str(), v1[0].c_str());
}

TEST(TestObservable, AddObserverFirst)
{
  Generator gen;
  Observer1 obs1;
  Observer2 obs2;

  std::vector<std::string> v1;

  gen.AddObserver(&obs1);
  gen.AddObserverFirst(&obs2);

  gen.Test1(v1);

  ASSERT_STREQ(observer2.c_str(), v1[0].c_str());
  ASSERT_STREQ(observer1.c_str(), v1[1].c_str());

  v1.clear();

  Generator gen2;
  gen2.AddObserver(&obs1);
  gen2.AddObserverBefore(&obs2, &obs1);

  ASSERT_FALSE(gen2.Test2(v1));
  ASSERT_EQ(1, v1.size());
  ASSERT_STREQ(observer2.c_str(), v1[0].c_str());
}

TEST(TestObservable, RemoveObserver)
{
  Generator gen;
  Observer1 obs1;
  Observer2 obs2;

  std::vector<std::string> v1;

  gen.AddObserver(&obs1);
  gen.AddObserver(&obs2);

  gen.Test1(v1);

  ASSERT_STREQ(observer1.c_str(), v1[0].c_str());
  ASSERT_STREQ(observer2.c_str(), v1[1].c_str());

  v1.clear();
  gen.RemoveObserver(&obs1);

  gen.Test1(v1);
  
  ASSERT_FALSE(v1.empty());
  ASSERT_EQ(1, v1.size());
  ASSERT_STREQ(observer2.c_str(), v1[0].c_str());
}