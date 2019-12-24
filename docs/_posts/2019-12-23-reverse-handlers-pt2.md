---
layout: post
title: Reverse Handlers Pt. 2
author: Marcel Flores
excerpt_separator: <!--more-->
---

Unsurprisingly, debugging reverse handlers is difficult. After many months of
digging around, I've finally been able to work the reverse handlers, and their
associated components into decent order. Below, I discuss a number of the
unexpected challenges that made the process difficult.

<!--more-->

## The State Machine Knows

The first such issue comes from the complexity of optimistic misses.  In
particular, in the process of playing messages in a causation-inappropriate
order that will later be reversed, the simulator will sometimes play a message
that defies the current expected state of one of the processors. For example,
a train may receive an ACK from a station it doesn't think that it's sent
a TRAIN_ARRIVE to.

Initial attempts to deal with this scenario involved trying to make the
state machine *robust* to such errors, essentially ignoring such messages.
However, this appeared to result in state machines (both trains and stations)
that simply stopped responding, as they reached unrecoverable states.

Ultimately, the solution came directly from ROSS, which is entirely equipped to
handle such scenarios. In particular, ROSS has [a function](https://github.com/ROSS-org/ROSS/blob/e940296d42bcb15f96e84991793ab96db849f553/core/tw-lp.c#L168) `tw_lp_suspend()`,
which indicates that an LP has reached an untenable status and suspends
execution until things are unwound.

## Bad Input

Another significant issue came from problems with the input data: in this case
the list of routes extracted from the GTFS data. In particular, the set of data
I was testing with appeared to contain a set of near duplicate runs. While the
existing parser filters for duplicates, it only removed exact matches: these
duplicates featured identical starting times and stations, but about halfway
through the run one version was a minute slower than the other.[^footnote]

 [^footnote]: In the LA Metro data, this appeared in the 3rd week in December on the Gold line departing Atlantic.]

Ultimately, this created a strange tie for the simulator, and while the
sequential execution sorted this deterministically, the optimistic did not,
resulting in occasional mismatches.

Currently, this issue has been solved by relaxing the match conditions on the
de-duplication in the GTFS parsing. Ideally, it might be best to do this within
Capacity itself. For now, however, I've opted to keep the parsing code as
simple as possible in the main program.

## Too Many LPs

In the process of debugging the above issues, I started to suspect that the 
number of LPs was untenable. In particular, the original implementation
simply assigned each route to its own LP. This appeared to make a mess at the
very start, as every single route (about 19k for the test set) attempted to
arrive at their initial station immediately. However, since these arrivals were
spread out over 2 weeks of simulation time, this resulted in significant
contention and rollback.

To deal with this, I implemented a simple scheduling algorithm that assigned as
many routes as possible to each LP (avoiding overlaps). This reduced the number
of LPs to about 270. Unfortunately I still seem to be creating a mess for ROSS,
as it reports an efficiency of -2800%, which seems bad and appears to result
in optimistic computation taking substantially longer than sequential.

Now that things appear a bit more stable before, it's time to work on massaging
things a bit better into the world of ROSS. In particular, in understanding how
I can adjust the LP assignment, state machine design, and other components to
improve the performance and begin collecting meaningful metrics.

