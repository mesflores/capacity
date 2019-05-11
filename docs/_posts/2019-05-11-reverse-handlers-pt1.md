---
layout: post
title: Reverse Handlers Pt. 1
author: Marcel Flores
excerpt_separator: <!--more-->
---

Over the past few weeks (months?), I've been working on the reverse handlers for the
Transit Unit and Station state machines. Unsurprisingly, this turns out to be
quite difficult. While writing a "draft" of the handlers isn't too tricky
(*i.e.* figuring out what I think I want to do). Turning that into correct
implementations has proven a little tricker than I expected.  In particular,
I've faced a couple of challenges that made this trickier than I expected, so
I thought I'd outline those a bit here.

<!--more-->

As a quick reminder the ROSS PDES framework works by passing messages between
logical processor (trains and stations in our case). Each LP is given two
handlers for each type of message. The first, the forward handler, is how the
LP responds to that message, *i.e.* the main work of the system. The second,
the reverse handler is designed to *undo* whatever the corresponding forward
handler did. That way the system can optimistically process messages as
messages arrive by executing the forward handler. If ROSS detects that it made
a causality mistake, it can simple play the messages back in reverse, this time
executing the reverse handlers.

## Optimistic Execution and Reality that Never Was

The first challenge I encountered came from the way I had implemented queues at
stations. Recall from the earlier post on the state machines, that when a train
arrives at a station, it sends a TRAIN_ARRIVE message to the station and waits
for an acknowledgement. If the station is empty, it will send the
acknowledgement right away, and the train will proceed with its arrival
process. If the station is full, it will put the train in a list and wait
until the current train is finished before sending the acknowledgement.

As a first pass at implementing this process, I had implemented this list in
the simplest way possible, which came with the, seemingly reasonable,
constraint that only a single train could be in the list at a time. This
constraint proved entirely manageable in the conservative execution world
I had been working in: both my simple test network, and the larger data loaded
from LA Metro, never resulted in more than a single train waiting at a time.
This makes sense: none of the probabilistic delays have been implemented yet,
and the current schedules don't result in waiting trains be default.

However, it turns out this is *very* not true in the optimistic execution
world: the series of messages that would get processed optimistically would
often result in a large number of trains queuing up at a station. While the
messages that caused this would eventually be rolled back, it created
a scenario where the simulator needed to deal with a situation that never
happened during conservative execution. Of course, given the limitations in my
earlier implementation, driving the queues in this fashion lead to hard
crashes. Since the conservative version never hit those failures, I assumed
this was due to a bug in my reverse handler.

The take away here is no ultimately not surprising: optimistic execution will
happily create scenarios that "can't happen" in the ordinary world. While in
this case, it was easy enough to manage by improving my queuing system to allow
for more than one train, I suspect it might spell some trouble, or at least
further complications, for the way I'm managing the state machines in general.

## Optimistic Execution Seems Non-Deterministic

In it's current state, I've been validating the output of the optimistic
execution by running each version in three modes:
1, A single process (i.e. just running the model directly)
2. A parallel execution running in conservative mode (synch=2)
3. A parallel execution running in optimistic mode (synch=3)

I take the output of each run (basically timestamps and actions generated with
tw_output()), sort by the timestamp, and save it. I then diff the output of
each type of run. Sure enough 1 and 2 always match, with no issue.
What I discovered, however, is that sometimes the third matches, and sometimes it does not. 

This is ultimately, also not very surprising: I have a bug in one of the
reverse handlers, but it only get's triggered when that reverse handler gets
called with certain conditions. But that means that handler is not always
called the same way, meaning the actual execution is non-deterministic. This is also
straightforward to address: I've simply adjusted the verification
script to run *many* times, and raise an alarm if it the output fails to match
for any single run. It does, however, make debugging tricky, as you often need
a failure to see what went wrong. The bug I'm currently working on seems to
have a relatively low failure rate -- on the order of 1 in 25.

## Onward

Again, the challenges outlined here may be obvious to those familiar with
parallel programming and distributed systems, but they certainly caused me
a healthy bit of confusion. As I mentioned I'm also worried that they might
be a bad sign for the state-machine-and-control-message approach that I've
taken, but that remains to be seen.

