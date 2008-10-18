//
// WorkspaceIndicator.hh for pekwm
// Copyright © 2008 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _TIMER_HH_
#define _TIMER_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "pekwm.hh"

#include <list>
extern "C" {
#include <sys/time.h>
}

/**
 * Timed event running action when timeout is reached.
 */
template<typename T>
class TimedEvent
{
public:
  /**
   * TimedEvent constructor, set timeval to current time + timeout miliseconds.
   */
  TimedEvent(uint timeout, T set_data)
    : data(set_data)
  {
    if (gettimeofday(&timeval, 0) == -1) {
      // FIXME: throw sane exception
    }

    timeval.tv_sec += timeout / 1000;
    timeval.tv_usec += (timeout % 1000) * 1000;
  }

  /**
   * Compare timeout value
   */
  bool operator<(const TimedEvent &rhs) {
    return timercmp(&timeval, &rhs.timeval, <);
  }
  bool operator<(const struct timeval &rhs) {
    return timercmp(&timeval, &rhs, <);
  }

public:
  T data; //!< Data for timed event
  struct timeval timeval; //!< Time when timer expires
};

/**
 * Handler for timeout events.
 */
template<typename T>
class Timer
{
public:
  typedef TimedEvent<T>* timed_event_list_entry;
  typedef typename std::list<timed_event_list_entry> timed_event_list;
  typedef typename timed_event_list::iterator timed_event_list_it;

  /**
   * Timer destructor, removes all timed events.
   */
  ~Timer(void)
  {
    timed_event_list_it it(_events.begin());

    for (; it != _events.end(); ++it) {
      delete *it;
    }
  }

  /**
   * Add timeout to timer.
   *
   * @param timeout Timeout in miliseconds
   * @param data Data for TimedEvent
   * @return TimedEvent for timeout
   */
  timed_event_list_entry add(uint timeout, T data)
  {
    TimedEvent<T> *event = new TimedEvent<T>(timeout, data);

    // Find place in event list
    timed_event_list_it it(_events.begin());
    for (; it != _events.end() && *event < *(*it); ++it)
      ;

    it = _events.insert(it, event);

    // Update timeout if first in the list
    if (it == _events.begin()) {
      updateTimer();
    }

    return event;
  }

  /**
   * Remove timeout from timer.
   */
  void remove(timed_event_list_entry event) {
    timed_event_list_it it(_events.begin());
    for (; it != _events.end(); ++it) {
      if (*it == event) {
        it = _events.erase(it);
        break;
      }
    }

    if (it == _events.begin()) {
      updateTimer();
    }
  }

  /**
   * Fill events list with TimedEvent that has timed out.
   */
  unsigned int getTimedOut(timed_event_list &events) {
    struct timeval now;
    if (gettimeofday(&now, 0) == -1) {
      return 0; // FIXME: raise exception?
    }    

    timed_event_list_it it(_events.begin());
    for (; it != _events.end(); ++it) {
      if (*(*it) < now) {
        events.push_back(*it);
        it = --_events.erase(it);
      }
    }

    if (events.size() > 0) {
      updateTimer();
    }

    return events.size();
  }

private:
  /**
   * Update timer to run next timer.
   */
  void updateTimer(void) {
    if (! _events.size()) {
      return;
    }

    TimedEvent<T> *event = *_events.begin();
    struct itimerval value;

    timerclear(&value.it_interval);
    if (gettimeofday(&value.it_value, 0) == -1) {
      // FIXME: raise exception?
    }
    timersub(&event->timeval, &value.it_value, &value.it_value);

    setitimer(ITIMER_REAL, &value, 0);
  }

private:
  timed_event_list _events; //!< List of events in queue
};

#endif // _TIMER_HH_
