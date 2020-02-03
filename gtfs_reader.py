#!/usr/local/bin/python3.6
""" Reads gtfs files and builds a network with them """

import argparse
import csv
import datetime
import logging
import os.path

############### Utility Functions #############

def filter_stops(stop_info):
    """ Filter for only type 0 stops, ie load/unload"""

    new_stop_info = {}
    for stop in stop_info:
        if stop_info[stop]["location_type"] == "0":
            new_stop_info[stop] = stop_info[stop]

    return new_stop_info

def map_to_parent(stop_id, stop_info):
    """ Given a stop ID and the stop Info dict, map to the parent station,
    unless there isnt one."""

    parent = stop_info[stop_id]["parent_station"]
    # Some stations (and bus stops) don't have a parent, stay the same
    if parent == "":
        parent = stop_id

    return parent

def map_to_date(s_time, date):
    """Spit out a formated date-time using the set two"""
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
    elif int(time_split[0]) == 28:
        date = (old_date + datetime.timedelta(days=1)).strftime("%Y%m%d")
        s_time = "04:" + time_split[1] + ":" + time_split[2]

    return date + " " + s_time


def compute_time(format_time):
    """ Given a formated time, compute epoch"""
    return int(datetime.datetime.strptime(format_time, "%Y%m%d %H:%M:%S").timestamp())

def hash_route(route):
    """ Given a route as a list of stops and times, compute a silly hash for it"""
    # Empirically, sometimes you can get the same route multiple times
    # with slightly different times, for the hash, just check the start time
    # along with the stops for the remainder
    #
    # Original full path, timing hashes
    #str_list = [x[0]+x[1] for x in route]
    # Less picky hash
    str_list = [route[0][0] + route[0][1]]
    str_list.extend([x[0] for x in route[1:]])

    return "".join(str_list)


################### Parsers ####################

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
    ]

    gtfs_optional_files = [
        "transfers",
    ]

    # Build a dict of all the required file names
    for gtfs_file in gtfs_req_files:
        # build the actual file name
        file_name = gtfs_file + ".txt"
        file_name = os.path.join(data_dir, file_name)
        gtfs_info[gtfs_file] = file_name

        if not os.path.isfile(file_name):
            logging.error("Missing file %s", file_name)
            raise RuntimeError("Missing file %s"%(file_name))

    # Now do the optional
    for gtfs_file in gtfs_optional_files:
        # build the actual file name
        file_name = gtfs_file + ".txt"
        file_name = os.path.join(data_dir, file_name)

        # Here, not existing isnt an error and it doesnt add the path
        # (Since it doesnt exist)
        if not os.path.isfile(file_name):
            logging.info("Optional file %s not found, skipping", file_name)
        else:
            gtfs_info[gtfs_file] = file_name

    return gtfs_info

def parse_gtfs_file(raw_file_name, key_column, filter_set=None, filter_col=None):
    """Parse a GTFS file, with key column being the unique key

       If a filter set is given, only take rows where the filter col has a
       value that lives in the filter set.
    """

    # If there is no unique key, return a list
    if key_column is None:
        parse_info = []
    else:
        parse_info = {}

    with open(raw_file_name) as raw_file:
        reader = csv.DictReader(raw_file)
        for row in reader:
            # If we need to filter
            if filter_set is not None:
                if row[filter_col] not in filter_set:
                    continue

            # Either take the whole row or grab the key
            if key_column is None:
                parse_info.append(row)
            else:
                # Get the key value
                key_val = row[key_column]
                parse_info[key_val] = {}

                # Loop through the rest
                for row_key in row:
                    parse_info[key_val][row_key] = row[row_key]

    return parse_info


def parse_stop_times_file(stop_times_file, filter_set=None):
    """Load the stop times into a big dict"""
    # NOTE: This is maybe possible with the above parsing function, but
    # would be a little complicated since there is no key in the same way, and
    # I just want the sequences to go in a list
    stop_times = {}
    with open(stop_times_file) as stop_times_raw:
        # Spin through the lines
        for index, line in enumerate(stop_times_raw):
            # If its the first one, learn the column positions
            if index == 0:
                columns = line.split(",")
                key_index = columns.index("trip_id")
                continue
            # Get the trip_id
            data_row = line.split(",")
            trip_id = data_row[key_index]

            # Check the filter set. Unilike the other files, this filter set
            # can only work on the trip_ids
            if filter_set is not None and trip_id not in filter_set:
                continue

            # did we have this?
            if trip_id not in stop_times:
                stop_times[trip_id] = []

            # Make a dict for this one
            trip_stop = {columns[x]: data for x, data in enumerate(data_row)}

            # Type cleaning
            trip_stop["stop_sequence"] = int(trip_stop["stop_sequence"])

            # Add it to the list
            stop_times[trip_id].append(trip_stop)

    # Sort them all by sequence number. Probably we could do that at insert
    for trip in stop_times:
        stop_times[trip].sort(key=lambda x: x["stop_sequence"])

    return stop_times

################ GTFS Processors ##############

def build_stop_adj_matrix(stop_times, stop_info):
    """Given a set of stop times, build an adjacencey matrix """

    stop_adj = {}

    for trip in stop_times:
        # Loop through the seq of stops
        for index, stop in enumerate(stop_times[trip]):
            # If its the first one, just move on
            if index == 0:
                continue

            # Get the parent station, unless there isnt one
            stop_parent = map_to_parent(stop["stop_id"], stop_info)

            # Where did I come from?
            prev = stop_times[trip][index - 1]
            prev_parent = map_to_parent(prev["stop_id"], stop_info)

            # Put it in adj matrix if needed
            if prev_parent not in stop_adj:
                stop_adj[prev_parent] = {}

            # What's the time between the two?
            # TODO Getting hacky with time, assuming no DST silly
            depart_components = prev["departure_time"].split(":")
            depart_components[0] = str(int(depart_components[0]) % 24)
            depart_time = datetime.datetime.strptime(":".join(depart_components),
                                                     "%H:%M:%S")
            arrive_components = stop["arrival_time"].split(":")
            arrive_components[0] = str(int(arrive_components[0]) % 24)
            arrive_time = datetime.datetime.strptime(":".join(arrive_components),
                                                     "%H:%M:%S")

            # TODO: That mod to deal with day wrap around is real sketchy
            weight = (arrive_time - depart_time).total_seconds() % 86400

            # Save the minimum weight in the matrix, ie the min time between stations
            if stop_parent in stop_adj[prev_parent]:
                curr = stop_adj[prev_parent][stop_parent]
                weight = min(curr, weight)

            # Stick it in the matrix
            stop_adj[prev_parent][stop_parent] = weight

    return stop_adj

def connect_transfers(transfers, stop_info):
    """ Loop through all the stuff and match all the stations with transfers"""
    matches = {}

    # Ok! If we got a transfers file, go ahead and use that to build match sets
    if transfers is not None:
        for transfer in transfers:
            if transfer["from_stop_id"] in matches:
                matches[transfer["from_stop_id"]].add(transfer["to_stop_id"])
            else:
                matches[transfer["from_stop_id"]] = set([transfer["to_stop_id"]])
    else:
        # So if there is no transfers file, we still might be able to do something.
        # In the LA Metro Data (not seen it elsewhere) there is a field in the stops.txt
        # called "tpis_name". I suspect this is some softward creating columns it shouldn't,
        # but it appears to indicate transfers...
        for stop in stop_info:
            # First, let's check to see if we have it, if not bail
            if "tpis_name" not in stop_info[stop]:
                return {}

            for inner_stop in stop_info:
                if (stop_info[stop]["tpis_name"] == stop_info[inner_stop]["tpis_name"] and
                        stop != inner_stop):

                    # GO ahead and add it to the list
                    if stop in matches:
                        matches[stop].add(inner_stop)
                    else:
                        matches[stop] = set([inner_stop])

    return matches

def extract_dates(cal_row):
    """ Given a row from calendar.txt, figure out the corresponding dates """

    day_seq = ["monday", "tuesday", "wednesday", "thursday", "friday",
               "saturday", "sunday"]

    curr_date = datetime.datetime.strptime(cal_row["start_date"], "%Y%m%d")
    service_end = datetime.datetime.strptime(cal_row["end_date"], "%Y%m%d")

    # Convert the days to just a list
    seq_list = [cal_row[x] for x in day_seq]

    index = curr_date.weekday()
    date_list = []

    # Until we get to the end
    while curr_date <= service_end:
        # Is there service on this day?
        # yes!
        if seq_list[index] == "1":
            date_list.append(curr_date.strftime("%Y%m%d"))

        # Bump it along
        index = (index + 1) % 7
        curr_date = curr_date + datetime.timedelta(days=1)

    return date_list

def build_service_set(gtfs_data):
    """Based on the calendar, figure out which service IDs should run for
    each date"""
    # The master dict of days
    # Keys are the dates, the values are a list of service IDs that run for
    # that day
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
    for exception in gtfs_data["calendar_dates"]:
        service_id = exception["service_id"]
        e_type = exception["exception_type"]
        date = exception["date"]

        if e_type == "1":
            service_days[date].append(service_id)
        elif e_type == "2":
            try:
                service_days[date].remove(service_id)
            except ValueError:
                logging.warning("Service %s wasn't scheduled for %s but was excepted!",
                                service_id, date)
            except KeyError:
                logging.warning("Service %s wasn't scheduled for %s all day but was excepted!",
                                service_id, date)
    return service_days

def generate_route(route_id, service_days, gtfs_data, min_stamp, max_time):
    """For a given route, spin through stop times and build longest version """
    # Ok so now we know for each day what service IDs are in effect. So now,
    # we'll spin over the days. For each day, we'll figure out which trips
    # correspond to those service IDs, then we'll actually dig out the specific
    # stop times (mapped to the corresponding day).
    route_list = []
    for day in service_days:
        trip_id_set = set()

        # Ok we have the service IDs, let's get the trips they contain
        # Loop through the trips and get all the trip IDs that match the route
        trip_info = gtfs_data["trip_info"]
        for trip in trip_info:
            # Is it our route?
            if trip_info[trip]["route_id"] != route_id:
                continue

            # Is the service ID active for this day?
            if trip_info[trip]["service_id"] not in service_days[day]:
                continue

            # Ok so let's grab that
            trip_id_set.add(trip_info[trip]["trip_id"])

        # Ok, now figure out the routes for each one
        stop_times = gtfs_data["stop_times"]
        stop_info = gtfs_data["stops"]

        for trip_id in stop_times:
            if trip_id not in trip_id_set:
                continue
            # Ok this is a trip we want
            # We need to map each stop ID (which are the
            # platforms) to its parent station
            new_route = [(map_to_parent(stop["stop_id"], stop_info),
                          stop["arrival_time"]) for stop in stop_times[trip_id]]
            # For kind of weird reasons related to midnight overlap you need to
            # tell it the date. Actually this works out well for doing multiple
            # dates at once
            new_route = [(x[0], map_to_date(x[1], day)) for x in new_route]

            # Actually, if this route is over the limit, don't save it
            if max_time and (compute_time(new_route[0][1]) > (max_time + min_stamp)):
                continue

            # Finally, we need to check if the route has a double stop in it
            # Ie if the route contains an out-and-back trip, if so, split it up
            # This returns a list of routes
            new_route_set = split_route(new_route)

            # Add our list (possibly of length 1!) to the big master list
            route_list.extend(new_route_set)

    # At the end of the outer loop, we have a list that has all the routes for
    # all the days that match our particular route_id for a described set of
    # service days
    return route_list

def split_route(route):
    """ Checks to see if a route contains an out and back.
        If so, split it into 2 separate routes
    """
    split_index = None
    # Loop over each step
    for index, hop in enumerate(route):
        if index == 0:
            continue
        curr_stop = hop[0]
        prev_stop = route[index - 1][0]

        if prev_stop == curr_stop:
            split_index = index

    # If we never found a double just return the original route in a list
    if split_index is None:
        return [route,]

    # Split it on the double, return the halves
    return [route[:split_index], route[split_index:]]



################# Primary Flow #################

class GTFSReader():
    """ Reader to load, validate, and manage GTFS files """
    def __init__(self, data_dir, route_types):
        self.data_dir = data_dir

        # Make sure that data dir is valid
        if not os.path.isdir(data_dir):
            raise RuntimeError("Not a directory path!")

        # Load up the file paths, make sure everything exists
        logging.info("Loading GTFS files...")
        self.raw_files = read_gtfs_files(self.data_dir)

        # Actually load and parse all the data
        self._load_gtfs_data(route_types)

    def _load_gtfs_data(self, route_types):
        """Does all the heavy lifting returns everything in a nice dict"""

        logging.info("Parsing Agency...")
        agency_info = parse_gtfs_file(self.raw_files["agency"], "agency_name")

        # First, we'll have to get the full set of routes
        # Here, we only want route ids that had our matching route type
        logging.info("Parsing Routes...")
        route_info = parse_gtfs_file(self.raw_files["routes"], "route_id",
                                     filter_set=route_types,
                                     filter_col="route_type")

        # Build a filter set for trips
        route_ids = set(route_info.keys())

        logging.info("Parsing Trips...")
        trip_info = parse_gtfs_file(self.raw_files["trips"], "trip_id",
                                    filter_set=route_ids,
                                    filter_col="route_id")

        # Build a filter set for the stop times
        trip_ids = set(trip_info.keys())

        logging.info("Parsing Stop Times...")
        stop_times = parse_stop_times_file(self.raw_files["stop_times"],
                                           filter_set=trip_ids)

        # Load the set of stops
        logging.info("Parsing Stops...")
        stop_info = parse_gtfs_file(self.raw_files["stops"], "stop_id")
        # Filter for actual load/unload, remove entrances
        # NOTE: This may not be correct for all systems!
        stop_info = filter_stops(stop_info)

        logging.info("Parsing Calendar...")
        calendar = parse_gtfs_file(self.raw_files["calendar"], "service_id")

        logging.info("Parsing Calendar Dates...")
        calendar_dates = parse_gtfs_file(self.raw_files["calendar_dates"], None)

        # Load the transfers file, if they gave us one
        if "transfers" in self.raw_files:
            transfers = parse_gtfs_file(self.raw_files["transfers"], None)
        else:
            transfers = None


        # Stick it all in a dict for now
        self.gtfs_data = {}

        self.gtfs_data["agency"] = agency_info
        self.gtfs_data["routes"] = route_info
        self.gtfs_data["stops"] = stop_info
        self.gtfs_data["trip_info"] = trip_info
        self.gtfs_data["stop_times"] = stop_times
        self.gtfs_data["calendar"] = calendar
        self.gtfs_data["calendar_dates"] = calendar_dates
        self.gtfs_data["transfers"] = connect_transfers(transfers, stop_info)

    def gen_matrix_out(self, outfile):
        """ Generate a clean weighted adj-matrix output for capacity to consume """

        logging.info("Building minimum adjacencey matrix...")
        # Generate the matrix
        adj_matrix = build_stop_adj_matrix(self.gtfs_data["stop_times"],
                                           self.gtfs_data["stops"])
        # Dump it to a file
        with open(outfile, 'w') as out_f:
            out_f.write("%d\n"%len(adj_matrix))
            for src in adj_matrix:
                for dst in adj_matrix[src]:
                    out_f.write("%s %s %d\n"%(src, dst, adj_matrix[src][dst]))

    def gen_routes_out(self, outfile, max_time=None):
        """ Generate route info for runs described in GTFS"""
        with open(outfile, 'w') as out_f:
            # First, let's get the start date, write it in the file
            min_date = min([y["start_date"] for (x, y) in self.gtfs_data["calendar"].items()])
            min_stamp = int(datetime.datetime.strptime(min_date, "%Y%m%d").timestamp())
            out_f.write("%d\n"%(min_stamp - 1))

            # Now the specific routes
            full_route_list = [] # Masater list
            unique_set = set() # For keeping track of unique routes

            for route_set in self.gtfs_data["routes"]:
                full_route = generate_route(route_set,
                                            build_service_set(self.gtfs_data),
                                            self.gtfs_data,
                                            min_stamp,
                                            max_time)
                new_routes = []
                # NOTE: Something can result in dupes, de dupe it!
                # Actually, this should really be a hard error I think...
                for route in full_route:
                    # Check the time, don't save it if over the time
                    start_time = compute_time(route[0][1])
                    if (max_time and (start_time - min_stamp > max_time)):
                        continue

                    # Hash it and check for duplicates
                    hash_r = hash_route(route)
                    if hash_r in unique_set:
                        logging.warning("Skipping duplicate route: %s %s", route[0][0], route[0][1])
                        continue
                    unique_set.add(hash_r)
                    new_routes.append(route)

                full_route_list.extend(new_routes)

            out_f.write("%d\n"%(len(full_route_list)))

            for route in sorted(full_route_list, key=lambda x: compute_time(x[0][1])):
                for stop, s_time in route:
                    # Let's make a nice epoch version of the stop time
                    s_epoch = compute_time(s_time)
                    out_f.write("%s,%s "%(stop, s_epoch))
                out_f.write("\n")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Read GTFS files and generate"
                                     "outputs for Capacity simulator",
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument("gtfs_dir", help="GTFS File Directory")
    parser.add_argument("-a", "--adjacency", type=str, help="The output adjacency matrix",
                        default="adj.mat")
    parser.add_argument("-r", "--routes", type=str, help="The output routes file",
                        default="routes.dat")
    parser.add_argument("-m", "--max_time", type=int, help="Only generate"
                        " routes that occur in the first max_time seconds", default=0)
    parser.add_argument("--route_types", nargs="+", help="Route types to load.",
                        default=['0', '1'])
    args = parser.parse_args()

    # Configure the log formatter
    logging.basicConfig(format="%(asctime)s %(levelname)s: %(message)s")

    # Load everything
    print("Loading GTFS data...")
    g_reader = GTFSReader(args.gtfs_dir, args.route_types)

    # Dump the adj matrix
    print("Generating adjacency matrix...")
    g_reader.gen_matrix_out(args.adjacency)

    print("Generating routes file...")
    # Dump the routes themselves
    g_reader.gen_routes_out(args.routes, args.max_time)
