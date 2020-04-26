// track.h
//
// Functions to manage tracks and queues at stations

#ifndef _track_h
#define _track_h

struct track_t;

track_t* track_map(int curr_station, int prev_station, station_state* s, message *in_msg);
track_t* track_map_rev(int curr_station, int prev_station, station_state* s, message *in_msg);

// Queue management
int add_train(tw_lpid new_train, track_t* track);
int pop_head(track_t* track);
int add_train_head(tw_lpid new_train, track_t* track);
int pop_tail(track_t* track);

# endif
