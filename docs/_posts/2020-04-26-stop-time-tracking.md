---
layout: post
title: Stop Time Tracking
author: Marcel Flores
excerpt_separator: <!--more-->
---

Some months back, I started to compare the performance of the simulated
schedule with the scheduled completion time of a run. The idea was that as soon
as I could get that logging in place, I could start adding some random delays
and measure the impact. However, I quickly noticed a deficiency in the existing
model.

<!--more-->

## Too Far Ahead

What I expected to find were completions that were slightly behind time: the
state machine design does consume some small amount of simulation time, so one
might expect by the end of the run, a transit unit would have consumed the time
necessary to move between the stations _and_ the time to negotiate the state
machine changes with each of the stations. All in all, I expected this extra
time to be on the order of seconds.

However, when I started looking at the differences, I noticed that some of the
runs were finishing quite a bit early: the train was arriving at its final
destination well before it should have been, sometimes on the order of tens of
minutes early.

A bit of digging into the cases that were causing this revealed scheduled runs
on the LA Metro Green Line that had significantly more time scheduled between
the stops than the travel time. In this case, these were some of the first runs
of the day, so the spacing makes some sense. [^footnote]

[^footnote]: I had actually looked for a version of this some time back, when I was trying to see if the DTLA stops on the expo line added more time during rush hour. At the time, at least, they did not.

## Delaying at Stations

As a solution, I ended up modifying how we store our routes internally.
Previously, the routes were a start time, end time, and a sequence of stops.
But that meant that the only timing information the transit units knew about in
the middle segment was the minimum time between stations. Therefore all runs
progressed through the stops as fast as possible. 

To account for the "delayed" runs that went slower than the minimum time,
routes now store both the sequence of stops and the expected timing of each
arrival. When a train is ready to depart a station (_i.e._ has received
a P_COMPLETE from a station), it simply checks its schedule, and delays the
appropriate amount of time if needed. When it's ready to leave, it prompts the station
for any new passengers, boards them, and then departs as normal.

For the reverse function, a delayed flag is simply stashed in the incoming
message, preventing having to do anything clever when undoing the delays (for
example, having to add a whole new state to the departure procedure).

This approach has the added bonus that it allows the transit unit to flush any
accumulated state-machine-time by simply waiting only until the schedule is
ready to depart.

## Onward! 

In returning to the original goal, measuring on-time performance, the most
significant pending component is figuring out the best way to get data out of
ROSS. There seem to be a few options, ranging fromn just raw output to more
complex stats reporting. Additionally it's likely time to spend some effort
tuning the optimistic parameters: optimistic execution remains woefully slow
and inefficient, but I've spent very little effort trying to improve its
performance.

