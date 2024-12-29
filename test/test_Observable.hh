//
// test_Observable.cc for pekwm
// Copyright (C) 2021-2024 Claes Nästén <pekdon@gmail.com>
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
	TestObserver();
	virtual ~TestObserver();
	virtual void notify(Observable* observable, Observation* observation);

	std::vector<TestObservation*> observations;
	int last_seq;
};

class TestObserverMapping : public TestSuite {
public:
	TestObserverMapping(void);
	~TestObserverMapping(void);

	virtual bool run_test(TestSpec spec, bool status);

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

TestObserver::TestObserver()
	: Observer(),
	  last_seq(0)
{
}

TestObserver::~TestObserver(void)
{
}

void
TestObserver::notify(Observable* observable, Observation* observation)
{
	static int seq = 1;

	TestObservation *test_observation =
		dynamic_cast<TestObservation*>(observation);
	observations.push_back(test_observation);
	last_seq = seq++;
}

TestObserverMapping::TestObserverMapping(void)
	: TestSuite("ObserverMapping")
{
}

TestObserverMapping::~TestObserverMapping(void)
{
}

bool
TestObserverMapping::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "notify", testNotify());
	TEST_FN(spec, "removeObservable", testRemoveObservable());
	TEST_FN(spec, "destructObservable", testDestructObservable());
	return status;
}

void
TestObserverMapping::testNotify(void)
{
	ObserverMapping om;
	TestObservable observable;
	TestObserver observer1;
	TestObserver observer2;
	TestObserver observer3;
	TestObservation observation;

	// notify no observers
	om.notifyObservers(&observable, nullptr);
	ASSERT_EQUAL("no observer", 0, om.size());
	ASSERT_EQUAL("no observer", 0, observer1.observations.size());
	ASSERT_EQUAL("no observer", 0, observer2.observations.size());

	// notify one observer
	om.addObserver(&observable, &observer1, 50);
	ASSERT_EQUAL("one observer", 1, om.size());
	om.notifyObservers(&observable, &observation);
	ASSERT_EQUAL("one observer", 1, observer1.observations.size());
	ASSERT_EQUAL("observer seq", 1, observer1.last_seq);
	ASSERT_EQUAL("one observer", 0, observer2.observations.size());
	ASSERT_EQUAL("observer seq", 0, observer2.last_seq);

	// notify multiple observers
	om.addObserver(&observable, &observer2, 100);
	om.addObserver(&observable, &observer3, 25);
	ASSERT_EQUAL("three observers", 1, om.size());
	om.notifyObservers(&observable, &observation);
	ASSERT_EQUAL("three observers", 2, observer1.observations.size());
	ASSERT_EQUAL("observer seq", 3, observer1.last_seq);
	ASSERT_EQUAL("three observers", 1, observer2.observations.size());
	// notified after observer1 due to higher priority
	ASSERT_EQUAL("observer seq", 4, observer2.last_seq);
	// notified first due to lower priority
	ASSERT_EQUAL("observer seq", 2, observer3.last_seq);

	// remove observers
	om.removeObserver(&observable, &observer1);
	om.removeObserver(&observable, &observer3);
	ASSERT_EQUAL("remove observers", 1, om.size());
	om.notifyObservers(&observable, &observation);
	ASSERT_EQUAL("remove observers", 2, observer1.observations.size());
	ASSERT_EQUAL("remove observers", 2, observer2.observations.size());
	ASSERT_EQUAL("observer seq", 5, observer2.last_seq);

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

	om.addObserver(&observable, &observer1, 100);
	om.addObserver(&observable, &observer2, 100);
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

	ObserverMapping *om = pekwm::observerMapping();
	{
		TestObservable observable;
		om->addObserver(&observable, &observer, 100);
		ASSERT_EQUAL("destruct observable", 1, om->size());
	}

	ASSERT_EQUAL("destruct observable", 0, om->size());
}
