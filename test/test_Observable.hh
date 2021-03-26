//
// test_Observable.cc for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "test.hh"

#include "Observable.hh"

class TestObservable : public Observable {
public:
    virtual ~TestObservable(void);
};

class TestObservation : public Observation {
public:
    virtual ~TestObservation(void);
};

class TestObserver : public Observer {
public:
    virtual ~TestObserver(void);
    virtual void notify(Observable* observable, Observation* observation);

    std::vector<TestObservation*> observations;
};

class TestObserverMapping : public TestSuite {
public:
    TestObserverMapping(void);
    ~TestObserverMapping(void);

private:
    static void testNotify(void);
    static void testRemoveObservable(void);
    static void testDestructObservable(void);
};

TestObservable::~TestObservable(void)
{
}

TestObservation::~TestObservation(void)
{
}

TestObserver::~TestObserver(void)
{
}

void
TestObserver::notify(Observable* observable, Observation* observation)
{
    auto test_observation = dynamic_cast<TestObservation*>(observation);
    observations.push_back(test_observation);
}

TestObserverMapping::TestObserverMapping(void)
    : TestSuite("ObserverMapping")
{
    register_test("notify", TestObserverMapping::testNotify);
    register_test("removeObservable",
                  TestObserverMapping::testRemoveObservable);
    register_test("destructObservable",
                  TestObserverMapping::testDestructObservable);
}

TestObserverMapping::~TestObserverMapping(void)
{
}

void
TestObserverMapping::testNotify(void)
{
    ObserverMapping om;
    TestObservable observable;
    TestObserver observer1;
    TestObserver observer2;
    TestObservation observation;

    // notify no observers
    om.notifyObservers(&observable, nullptr);
    ASSERT_EQUAL("no observer", 0, om.size());
    ASSERT_EQUAL("no observer", 0, observer1.observations.size());
    ASSERT_EQUAL("no observer", 0, observer2.observations.size());

    // notify one observer
    om.addObserver(&observable, &observer1);
    ASSERT_EQUAL("one observer", 1, om.size());
    om.notifyObservers(&observable, &observation);
    ASSERT_EQUAL("one observer", 1, observer1.observations.size());
    ASSERT_EQUAL("one observer", 0, observer2.observations.size());

    // notify multiple observers
    om.addObserver(&observable, &observer2);
    ASSERT_EQUAL("two observers", 1, om.size());
    om.notifyObservers(&observable, &observation);
    ASSERT_EQUAL("two observers", 2, observer1.observations.size());
    ASSERT_EQUAL("two observers", 1, observer2.observations.size());

    // remove observer
    om.removeObserver(&observable, &observer1);
    ASSERT_EQUAL("remove observers", 1, om.size());
    om.notifyObservers(&observable, &observation);
    ASSERT_EQUAL("remove observers", 2, observer1.observations.size());
    ASSERT_EQUAL("remove observers", 2, observer2.observations.size());

    // no observers
    om.removeObserver(&observable, &observer2);
    ASSERT_EQUAL("no observers", 0, om.size());
    om.notifyObservers(&observable, &observation);
    ASSERT_EQUAL("no observers", 2, observer1.observations.size());
    ASSERT_EQUAL("no observers", 2, observer2.observations.size());
}

void
TestObserverMapping::testRemoveObservable(void)
{
    ObserverMapping om;
    TestObservable observable;
    TestObserver observer1;
    TestObserver observer2;
    TestObservation observation;

    om.addObserver(&observable, &observer1);
    om.addObserver(&observable, &observer2);
    // removing the observable should remove the observers from its
    // list as well.
    om.removeObservable(&observable);
    om.notifyObservers(&observable, &observation);

    ASSERT_EQUAL("removed observable", 0, observer1.observations.size());
    ASSERT_EQUAL("removed observable", 0, observer2.observations.size());
}

void
TestObserverMapping::testDestructObservable(void)
{
    TestObserver observer;

    auto om = pekwm::observerMapping();
    {
        TestObservable observable;
        om->addObserver(&observable, &observer);
        ASSERT_EQUAL("destruct observable", 1, om->size());
    }

    ASSERT_EQUAL("destruct observable", 0, om->size());
}
