#!/usr/local/bin/python3.6
""" Reads gtfs files and builds a network with them """

import csv
import datetime
import logging
import os.path
import sys

def min_date(data_dir):
    """ Peak into calendar.txt and pick the minumum date """
    calendar = os.path.join(data_dir, "calendar.txt")
    date = []
    with open(calendar, 'r') as cal_file:
        reader = csv.reader(cal_file)
        for line in reader:
            # TODO: this is bad it should use the columns and find start date etc etc etc
            start = line[8]
            if start == "start_date":
                continue
            date.append(int(start))

    return str(min(date))

def read_gtfs_files(data_dir):
    """ Read all the files and load them into python dict raw"""
    gtfs_info = {}

    # GTFS sfiles standards
    gtfs_req_files = [
        "agency",
        "stops",
        "routes",
        "trips",
        "stop_times",
        "calendar",
        "calendar_dates",
        # There are others but not required. Someday.
    ]

    for gtfs_file in gtfs_req_files:
        # build the actual file name
        file_name = gtfs_file + ".txt"
        file_name = os.path.join(data_dir, file_name)
        with open(file_name) as gtfs_file_obj:
            # Just dump the whole thing into mem.
            # these might be kind of big in some cases...
            gtfs_info[gtfs_file] = gtfs_file_obj.read().strip()

    return gtfs_info

def parse_gtfs_file(raw_file, key_column):
    """Parse a GTFS file, with key column being the unique key"""
    parse_info = {}

    reader = csv.DictReader(raw_file.splitlines())
    for row in reader:
        # Get the key value
        key_val = row[key_column]
        parse_info[key_val] = {}

        # Loop through the rest
        for row_key in row:
            parse_info[key_val][row_key] = row[row_key]
    return parse_info

def filter_stops(stop_info):
    """ Filter for only type 0 stops, ie load/unload"""

    new_stop_info = {}
    for stop in stop_info:
        if stop_info[stop]["location_type"] == "0":
            new_stop_info[stop] = stop_info[stop]

    return new_stop_info

def load_stop_times(stop_times_raw):
    """Load the stop times into a big dict"""
    # NOTE: This is maybe possible with the above parsing function, but
    # would be a little complicated since there is no key in the same way, and
    # I just want the sequences to go in a list
    stop_times = {}
    # Spin through the lines
    for index, line in enumerate(stop_times_raw.split("\n")):
        # If its the first one, learn the column positions
        if index == 0:
            columns = line.split(",")
            key_index = columns.index("trip_id")
            continue
        # Get the trip_id
        data_row = line.split(",")
        trip_id = data_row[key_index]

        # did we have this?
        if trip_id not in stop_times:
            stop_times[trip_id] = []

        # Make a dict for this one
        trip_stop = {columns[x]: data for x, data in enumerate(data_row)}

        # Type cleaning
        trip_stop["stop_sequence"] = int(trip_stop["stop_sequence"])

        # Add it to the list
        stop_times[trip_id].append(trip_stop)

    # Sort them all by sequence number. Probably we could do that at insert,
    # but fuck it
    for trip in stop_times:
        stop_times[trip].sort(key=lambda x: x["stop_sequence"])

    return stop_times

def build_stop_adj_matrix(stop_times):
    """Given a set of stop times, build an adjacencey matrix """

    stop_adj = {}

    for trip in stop_times:
        curr_trip = stop_times[trip]
        # Loop through the seq of stops
        for index, stop in enumerate(curr_trip):
            # If its the first one, just move on
            if index == 0:
                continue

            # Where did I come from?
            prev = curr_trip[index - 1]
            prev_id = prev["stop_id"]

            # Put it in adj matrix if needed
            if prev_id not in stop_adj:
                stop_adj[prev_id] = {}

            # What's the time between the two?
            # TODO Getting hacky with time, assuming no DST silly
            depart_components = prev["departure_time"].split(":")
            depart_components[0] = str(int(depart_components[0]) % 24)
            depart_time = datetime.datetime.strptime(":".join(depart_components),
                                                     "%H:%M:%S")
            arrive_components = stop["arrival_time"].split(":")
            arrive_components[0] = str(int(arrive_components[0]) % 24)
            arrive_string = ":".join(arrive_components)
            arrive_time = datetime.datetime.strptime(arrive_string,
                                                     "%H:%M:%S")

            # TODO: That mod to deal with day wrap around is real sketchy
            weight = (arrive_time - depart_time).total_seconds() % 86400

            # Save the minimum wieght in the matrix, ie the min time between stations
            if stop["stop_id"] in stop_adj[prev_id]:
                curr = stop_adj[prev_id][stop["stop_id"]]
                weight = min(curr, weight)

            # Stick it in the matrix
            stop_adj[prev_id][stop["stop_id"]] = weight

    return stop_adj

def connect_transfers(stop_info):
    """ Loop through all the stuff and match all the stations with transfers"""
    matches = {}

    for stop in stop_info:
        for inner_stop in stop_info:
            if (stop_info[stop]["tpis_name"] == stop_info[inner_stop]["tpis_name"] and
                    stop != inner_stop):

                # GO ahead and add it to the list
                if stop in matches:
                    matches[stop].add(inner_stop)
                else:
                    matches[stop] = set([inner_stop])

    return matches

def load_gtfs_data(data_dir):
    """Does all the heavy lifting returns everything in a nice dict"""
    logging.info("Loading GTFS files...")
    raw_data = read_gtfs_files(data_dir)

    logging.info("Parsing Agency...")
    #agency_info = parse_gtfs_file(raw_data["agency"], "agency_id")
    agency_info = parse_gtfs_file(raw_data["agency"], "agency_name")

    # More or less, it will look something like this:
    # Trips-> turn into trains + routes

    logging.info("Parsing Routes...")
    route_info = parse_gtfs_file(raw_data["routes"], "route_id")

    # Load the set of stops
    logging.info("Parsing Stops...")
    stop_info = parse_gtfs_file(raw_data["stops"], "stop_id")
    # Filter for actual load/unload, remove entrances
    stop_info = filter_stops(stop_info)

    logging.info("Parsing Trips...")
    trip_info = parse_gtfs_file(raw_data["trips"], "trip_id")

    logging.info("Parsing Stop Times...")
    stop_times = load_stop_times(raw_data["stop_times"])

    logging.info("Parsing Calendar...")
    calendar = parse_gtfs_file(raw_data["calendar"], "service_id")

    logging.info("Parsing Calendar Dates...")
    calendar_dates = parse_gtfs_file(raw_data["calendar_dates"], "service_id")

    logging.info("Building minimum adjacencey matrix...")
    adj_matrix = build_stop_adj_matrix(stop_times)
    transfers = connect_transfers(stop_info)

    # Stick it all in a dict for now
    gtfs_data = {}

    gtfs_data["agency"] = agency_info
    gtfs_data["routes"] = route_info
    gtfs_data["stops"] = stop_info
    gtfs_data["trip_info"] = trip_info
    gtfs_data["stop_times"] = stop_times
    gtfs_data["calendar"] = calendar
    gtfs_data["calendar_dates"] = calendar_dates
    gtfs_data["adj_matrix"] = adj_matrix
    gtfs_data["transfers"] = transfers

    return gtfs_data

def extract_dates(cal_row):
    """ Given a row from calendar.txt, figure out the corresponding dates """

    day_seq = ["monday", "tuesday", "wednesday", "thursday", "friday",
               "saturday", "sunday"]

    curr_date = datetime.datetime.strptime(cal_row["start_date"], "%Y%m%d")
    service_end = datetime.datetime.strptime(cal_row["end_date"], "%Y%m%d")

    # Convert the days to just a list
    seq_list = [cal_row[x] for x in day_seq]

    # start the index as none so we know if we should be incrementing the date
    index = 0
    start = False

    date_list = []

    # Until we get to the end
    while curr_date <= service_end:
        # Is there service on this day?
        # yes!
        if seq_list[index] == "1":
            date_list.append(curr_date.strftime("%Y%m%d"))
            start = True # We are getting stuff


        # Bumb it along
        index = (index + 1) % 7
        # If we started the counter, go ahead and bump it
        # Otherwise we spin through indices only
        if start:
            curr_date = curr_date + datetime.timedelta(days=1)

    return date_list

def generate_route(route_id, gtfs_data):
    """For a given route, spin through stop times and build longest version """

    # The master dict of days
    service_days = {}

    # Loop through each service ID
    for service_id in gtfs_data["calendar"]:
        dates = extract_dates(gtfs_data["calendar"][service_id])
        # add the approproiate service IDs
        for date in dates:
            if date in service_days:
                service_days[date].append(service_id)
            else:
                service_days[date] = [service_id,]

    # Ok, now let's go through the exceptions
    for service_id in gtfs_data["calendar_dates"]:
        e_type = gtfs_data["calendar_dates"][service_id]["exception_type"]
        date = gtfs_data["calendar_dates"][service_id]["date"]

        if e_type == "1":
            service_days[date].append(service_id)
        elif e_type == "2":
            try:
                service_days[date].remove(service_id)
            except ValueError:
                logging.warning("Service %s wasn't scheduled for %s but was excepted!",
                                service_id, date)

    # Ok so now we know for each day what service IDs are in effect. So now,
    # we'll spin over the days. For each day, we'll figure out which trips
    # correspond to those service IDs, then we'll actually dig out the specific
    # stop times (mapped to the corresponding day).
    route_list = []
    for day in service_days:
        trip_id_list = []

        # Ok we have the service IDs, let's get the trips they contain
        # Loop through the trips and get all the trip IDs that match the route
        trip_info = gtfs_data["trip_info"]
        for trip in trip_info:
            # Is it our route
            if trip_info[trip]["route_id"] != route_id:
                continue

            # Is it one of our service IDs?
            if trip_info[trip]["service_id"] not in service_days[day]:
                continue

            # Ok so let's grab that
            trip_id_list.append(trip_info[trip]["trip_id"])

        # Ok, now figure out the routes for each one
        stop_times = gtfs_data["stop_times"]


        for trip_id in stop_times:
            if trip_id not in trip_id_list:
                continue
            # Ok this is a trip we want
            new_route = [(stop["stop_id"], stop["arrival_time"]) for stop in stop_times[trip_id]]
            # For kind of weird reasons related to midnight overlap you need to
            # tell it the date. Actually this works out well for doing multiple
            # dates at once
            new_route = [(x[0], map_to_date(x[1], day)) for x in new_route]

            # Add it to the big master list
            route_list.append(new_route)

    return route_list

def map_to_date(s_time, date):
    """Spit out a formated date-time using theset two"""
    # This time format is bad and I feel bad

    # In general this time hacking is nasty. Basically the issue is that the
    # GTFS runs have a day, but the day might cary over past midnight. Python
    # datetimes dont like that.
    time_split = s_time.split(":")
    # If its more than 24, add one to the date
    old_date = datetime.datetime.strptime(date, "%Y%m%d")
    if int(time_split[0]) == 24:
        date = (old_date + datetime.timedelta(days=1)).strftime("%Y%m%d")
        s_time = "00:" + time_split[1] + ":" + time_split[2]
    elif int(time_split[0]) == 25:
        date = (old_date + datetime.timedelta(days=1)).strftime("%Y%m%d")
        s_time = "01:" + time_split[1] + ":" + time_split[2]
    elif int(time_split[0]) == 26:
        date = (old_date + datetime.timedelta(days=1)).strftime("%Y%m%d")
        s_time = "02:" + time_split[1] + ":" + time_split[2]
    elif int(time_split[0]) == 27:
        date = (old_date + datetime.timedelta(days=1)).strftime("%Y%m%d")
        s_time = "03:" + time_split[1] + ":" + time_split[2]

    return date + " " + s_time

def gen_matrix_out(adj_matrix, outfile):
    """ Generate a clean weighted adj-matrix output for capacity to consume """
    with open(outfile, 'w') as out_f:
        out_f.write("%d\n"%len(adj_matrix))
        for src in adj_matrix:
            for dst in adj_matrix[src]:
                out_f.write("%s %s %d\n"%(src, dst, adj_matrix[src][dst]))

def gen_routes_out():
    """ Generate route info for runs described in GTFS"""
    pass

if __name__ == "__main__":
    # Take the directory with the GTFS files
    data_dir = sys.argv[1]
    outfile = sys.argv[2]
    
    if not os.path.isdir(data_dir):
        raise RuntimeError("Not a directory path!")

    # Load everything
    data = load_gtfs_data(data_dir)

    # Dump the adj matrix
    gen_matrix_out(data["adj_matrix"], outfile)

