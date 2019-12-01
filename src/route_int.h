//route_int.h -- internal globals for the route

#ifndef _route_int_h
#define _route_int_h

#define MIN_ROUTE_GAP 7200

route_t* route_list;
route_set_t* route_set_list_l;
route_set_t* route_set_list;

int g_total_routes;
int g_total_transit_units;
int route_offset;
int g_start_time;

#endif
