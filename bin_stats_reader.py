""" Read binary stats files, mostly adapted from
    https://github.com/caitlinross/ross-binary-reader"""

import argparse
import csv
import os
import os.path
import struct

################ Format specifications ################
# Here the meta data given in the ROSS documentation:
#    https://ross-org.github.io/instrumentation/data-format.html:
#
#   int sample_type //0 = PE, 1 = KP, 2 = LP, 3 = Model
#   int sample_size
#   double virtual_time
#   double real_time
#
# Therefore the total size is 2 *(4) + 2*(8) = 24
META_SIZE = 24
META_DATA_KEYS = ["sample_type",
                  "sample_size",
                  "virtual_time",
                  "real_time",]

# PE Fields
#
#   unsigned int pe_id
#   unsigned int events_processed
#   unsigned int events_aborted
#   unsigned int events_rolled_back
#   unsigned int total_rollbacks
#   unsigned int secondary_rollbacks
#   unsigned int fossil_collect_attempts
#   unsigned int priority_queue_size
#   unsigned int network_sends
#   unsigned int network_receives
#   unsigned int num_gvts
#   unsigned int pe_event_ties
#   unsigned int all_reduce_count
#   float efficiency
#   float network_read_time
#   float network_other_time
#   float gvt_time
#   float fossil_collect_time
#   float events_aborted_time
#   float events_processed_time
#   float priority_queue_time
#   float rollback_time
#   float cancel_q_time
#   float avl_tree_time
#   float buddy_time
#   float lz4_time
PE_KEYS = ["pe_id",
           "events_processed",
           "events_aborted",
           "events_rolled_back",
           "total_rollbacks",
           "secondary_rollbacks",
           "fossil_collect_attempts",
           "priority_queue_size",
           "network_sends",
           "network_receives",
           "num_GVTs",
           "pe_event_ties",
           "all_reduce_count",
           "efficiency",
           "network_read_time",
           "network_other_time",
           "GVT_time",
           "fossil_collect_time",
           "events_aborted_time",
           "events_processed_time",
           "priority_queue_time",
           "rollback_time",
           "cancel_q_time",
           "avl_tree_time",
           "buddy_time",
           "lz4_time",]

# KP Fields
#
# unsigned int pe_id
# unsigned int kp_id
# unsigned int events_processed
# unsigned int events_aborted
# unsigned int events_rolled_back
# unsigned int total_rollbacks
# unsigned int secondary_rollbacks
# unsigned int network_sends
# unsigned int network_receives
# float time_ahead_gvt
# float efficiency
KP_KEYS = ["pe_id",
           "kp_id",
           "events_processed",
           "events_aborted",
           "events_rolled_back",
           "total_rollbacks",
           "secondary_rollbacks",
           "network_sends",
           "network_receives",
           "time_ahead_gvt",
           "efficiency",]

# LP Fields
#
# unsigned int pe_id
# unsigned int kp_id
# unsigned int lp_id
# unsigned int events_processed
# unsigned int events_aborted
# unsigned int events_rolled_back
# unsigned int network_sends
# unsigned int network_receives
# float efficiency

LP_KEYS = ["pe_id",
           "kp_id",
           "lp_id",
           "events_processed",
           "events_aborted",
           "events_rolled_back",
           "network_sends",
           "network_receives",
           "efficiency",]

FORMAT_TYPE = {"meta_data": "@iidd",
               "pe": "@13I13f",
               "kp": "@9I2f",
               "lp": "@8If",
               "model": "",}

KEY_TYPE = {"meta_data": META_DATA_KEYS,
            "pe": PE_KEYS,
            "kp": KP_KEYS,
            "lp": LP_KEYS,
            "model": [],}

#######################################################

def read_data(stats_file, read_type, meta_data=None):
    """ Generic function for reading data"""
    if meta_data:
        data = struct.unpack(FORMAT_TYPE[read_type], stats_file.read(meta_data["sample_size"]))
    else:
        # If we didn't get a meta_data, its a meta data read
        if read_type != "meta_data":
            raise RuntimeError("Missing meta data for read!")
        data = struct.unpack(FORMAT_TYPE[read_type], stats_file.read(META_SIZE))

    key_dict = dict(zip(KEY_TYPE[read_type], data))

    return key_dict


class BinStatsReader:
    """Class for reading Binary output from ROSS"""
    def __init__(self,
                 stats_file_name,
                 out_dir,
                 pe=True,
                 kp=False,
                 lp=False,
                 model=False, model_spec=None,):
        """ The stats file to read, which data to parse"""
        self.stats_file_name = stats_file_name

        self.out_dir = out_dir

        self.parse_pe = pe
        self.parse_kp = kp
        self.parse_lp = lp

        self.parse_model = model
        self.model_spec = model_spec

        if self.parse_model and self.model_spec is None:
            raise RuntimeError("Must provide model spec if parsing model data!")

        self.out_files = {"pe": None,
                          "kp": None,
                          "lp": None,
                          "model": None,}
        self.writers = {"pe": None,
                        "kp": None,
                        "lp": None,
                        "model": None,}

        self.prep_output()

    def prep_output(self):
        """ Prep the output dir and files"""

        # First, check the out_dir
        if not os.path.isdir(self.out_dir):
            os.mkdir(self.out_dir)

        # Prep a writer for each type. We trade around the ordering for each, so have
        # to do them one by one.
        if self.parse_pe:
            self.out_files["pe"] = open(os.path.join(self.out_dir, "pe_stats.csv"), 'w')
            # pe_id, timestamp, the rest of the PE keys
            output_keys = ["pe_id", "real_time"]
            output_keys.extend(PE_KEYS[1:])
            self.writers["pe"] = csv.DictWriter(self.out_files["pe"],
                                                fieldnames=output_keys,
                                                extrasaction="ignore")
            self.writers["pe"].writeheader()
        if self.parse_kp:
            self.out_files["kp"] = open(os.path.join(self.out_dir, "kp_stats.csv"), 'w')
            # pe_id, kp_id, timestamp, the rest of the KP keys
            output_keys = ["pe_id", "kp_id", "real_time"]
            output_keys.extend(KP_KEYS[2:])
            self.writers["kp"] = csv.DictWriter(self.out_files["kp"],
                                                fieldnames=output_keys,
                                                extrasaction="ignore")
            self.writers["kp"].writeheader()
        if self.parse_lp:
            self.out_files["lp"] = open(os.path.join(self.out_dir, "lp_stats.csv"), 'w')
            # pe_id, kp_id, timestamp, the rest of the KP keys
            output_keys = ["pe_id", "kp_id", "lp_id", "real_time"]
            output_keys.extend(KP_KEYS[3:])
            self.writers["lp"] = csv.DictWriter(self.out_files["lp"],
                                                fieldnames=output_keys,
                                                extrasaction="ignore")
            self.writers["lp"].writeheader()
        if self.parse_model:
            self.model_out = open(os.path.join(self.out_dir, "model_stats.csv"), 'w')

    def read(self):
        """ Loop through the file and parse"""
        with open(self.stats_file_name, 'rb') as stats_file:
            # Get the total size of the file
            stats_file.seek(0, 2)
            num_bytes = stats_file.tell()

            # We'll jump back to the beginning
            pos = 0
            while pos < num_bytes:
                stats_file.seek(pos)

                meta_data = read_data(stats_file, "meta_data")

                # Seek ahead by however much we read
                pos += META_SIZE
                stats_file.seek(pos)

                # The first field of the metadata is the type, call the
                # appropriate reader
                if self.parse_pe and meta_data["sample_type"] == 0:
                    # Parse the PE data
                    pe_data = read_data(stats_file, "pe", meta_data)
                    full_data = {}
                    full_data.update(meta_data)
                    full_data.update(pe_data)
                    self.writers["pe"].writerow(full_data)
                elif self.parse_kp and meta_data["sample_type"] == 1:
                    # Parse the KP data
                    kp_data = read_data(stats_file, "kp", meta_data)
                    full_data = {}
                    full_data.update(meta_data)
                    full_data.update(kp_data)
                    self.writers["kp"].writerow(full_data)
                elif self.parse_lp and meta_data["sample_type"] == 2:
                    # Parse the LP data
                    lp_data = read_data(stats_file, "lp", meta_data)
                    full_data = {}
                    full_data.update(meta_data)
                    full_data.update(lp_data)
                    self.writers["lp"].writerow(full_data)
                elif self.parse_model and meta_data["sample_type"] == 3:
                    # Parse the model data
                    print("Model data!")

                # Bump the position by however much the metadata told us to
                pos += meta_data["sample_size"]

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Script to read binary output stats from ROSS")
    parser.add_argument("stats_file", help="Binary stats file to parse")
    parser.add_argument("out_dir", help="Directory to write output", )
    args = parser.parse_args()

    bin_reader = BinStatsReader(args.stats_file, args.out_dir, pe=True, kp=True, lp=True)
    bin_reader.read()
