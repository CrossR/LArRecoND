#!/usr/bin/env python3
# HDF5 to ROOT converter for ND LArFlow data

import argparse
from enum import Enum
import os
from collections import defaultdict

import awkward as awk
import h5flow
import h5py as h5
import numpy as np
import uproot as ur


class HitType(Enum):
    FINAL = "final"
    MERGED = "merged"
    PROMPT = "prompt"


class LegacyMode(Enum):
    NONE = 0
    PRE_MINIRUN6 = 1
    POST_MINIRUN6_NO_USEC = 2


# Physics Constants
MEV2GEV = 0.001
TRUE_X_OFFSET = 0  # Offsets if geometry changes
TRUE_Y_OFFSET = 0  # 42+268
TRUE_Z_OFFSET = 0  # -1300
MAX_ARRAY_DEPTH = int(10000)
MAX_ARRAY_DEPTH_DATA = int(100000)
INVALID_TRIGGER_ID = (1 << 31) - 1
INVALID_TIME_VALUE = -5
MIN_HITS_REQUIRED = 2
# TODO: This was just a guesstimated name!
TRIGGER_ID_BEAM = 5

# General settings
PROGRESS_INTERVAL = 10

# Pre-defining the empty MC dictionary structure to avoid recreation in loops
# Make sure this matches the keys used in process_mc!
EMPTY_MC_DATA = {
    "matches": np.array([0], dtype="uint16"),
    "hit_packetFrac": np.array([], dtype="float32"),
    "hit_pdg": np.array([], dtype="int32"),
    "hit_segmentID": np.array([], dtype="int64"),
    "hit_particleID": np.array([], dtype="int64"),
    "hit_particleIDLocal": np.array([], dtype="int64"),
    "hit_vertexID": np.array([], dtype="int64"),
    "mcp_startx": np.array([], dtype="float32"),
    "mcp_starty": np.array([], dtype="float32"),
    "mcp_startz": np.array([], dtype="float32"),
    "mcp_endx": np.array([], dtype="float32"),
    "mcp_endy": np.array([], dtype="float32"),
    "mcp_endz": np.array([], dtype="float32"),
    "mcp_id": np.array([], dtype="int64"),
    "mcp_idLocal": np.array([], dtype="int64"),
    "mcp_pdg": np.array([], dtype="int32"),
    "mcp_energy": np.array([], dtype="float32"),
    "mcp_px": np.array([], dtype="float32"),
    "mcp_py": np.array([], dtype="float32"),
    "mcp_pz": np.array([], dtype="float32"),
    "mcp_vertex_id": np.array([], dtype="int64"),
    "mcp_nuid": np.array([], dtype="int64"),
    "mcp_mother": np.array([], dtype="int64"),
    "vertex_id": np.array([], dtype="int64"),
    "nuID": np.array([], dtype="int64"),
    "nuvtxx": np.array([], dtype="float32"),
    "nuvtxy": np.array([], dtype="float32"),
    "nuvtxz": np.array([], dtype="float32"),
    "nue": np.array([], dtype="float32"),
    "nuPDG": np.array([], dtype="int32"),
    "nupx": np.array([], dtype="float32"),
    "nupy": np.array([], dtype="float32"),
    "nupz": np.array([], dtype="float32"),
    "ccnc": np.array([], dtype="int32"),
    "mode": np.array([], dtype="int32"),
}


def parse_args():
    parser = argparse.ArgumentParser(
        description="Convert ND LArFlow HDF5 files to ROOT format."
    )

    # Required arguments
    parser.add_argument(
        "file_list",
        help="A comma-separated list of HDF5 input files to convert.",
        type=str,
    )

    # Optional arguments
    parser.add_argument(
        "-D",
        "--is_data",
        help="Flag to indicate if the input files are data, not MC (default: False).",
        default=False,
        action="store_true",
    )
    parser.add_argument(
        "-H",
        "--hit_type",
        help="Type of hits to process: 'final', 'merged', or 'prompt' (default: 'prompt').",
        type=str,
        choices=[hit_type.value for hit_type in HitType],
        default=HitType.PROMPT.value,
    )
    parser.add_argument(
        "-L",
        "--legacy_mode",
        help="Pick the legacy mode: 0 for nothing, 1 for samples < MiniRun6, 2 > MiniRun6 without usec (default: 0).",
        type=int,
        choices=[mode.value for mode in LegacyMode],
        default=LegacyMode.NONE.value,
    )
    parser.add_argument(
        "-o",
        "--output_file",
        help="Output ROOT file name (default: input[0]_hits_uproot.root)",
        type=str,
        default="",
    )

    args = parser.parse_args()

    # If no output file is specified, create a default name
    if not args.output_file:
        input_files = args.file_list.split(",")
        base_name = os.path.basename(input_files[0])
        args.output_file = f"{base_name}_hits_uproot.root"

    return args


def get_var_if_set(
    var: np.ndarray | None,
    dtype: str = "float32",
    default: np.ndarray | None = None,
) -> np.ndarray:
    """
    Return the given variable if set and the event is good; otherwise, return a default array.

    Args:
        var (np.ndarray | None): The variable to return if set.
        dtype (str): Data type for the default array if needed.
        default (np.ndarray | None): Default values to use if the event is bad.
    """
    if var is not None:
        return var
    else:
        default_arg = default if default is not None else []
        return np.array(default_arg).astype(dtype)


def process_mc(
    args: argparse.Namespace,
    f: h5.File,
    flow_out: h5.File,
    hits_ids: np.ndarray,
    spillID: int,
    other_dict: dict,
) -> None:
    """
    Inject MC truth information into the output dictionary.

    Adds three levels of MC truth:
    - Hit-level: backtracking info (PDG, segment IDs, etc.)
    - Particle-level: trajectory information (start/end positions, momentum, etc.)
    - Vertex-level: neutrino interaction information

    Args:
        args: Command-line arguments
        f: HDF5 file handle
        flow_out: H5Flow data manager
        hits_ids: Array of hit IDs for this event
        spillID: Spill/event ID for MC truth lookup
        other_dict: Dictionary to add MC truth arrays to (modified in-place)
    """
    # First, hit-level truth information.
    backtrack_hits = flow_out[
        "charge/calib_" + args.hit_type + "_hits",
        "mc_truth/calib_" + args.hit_type + "_hit_backtrack",
        hits_ids[:],
    ][:, 0]

    backtrack_masked = np.ma.masked_equal(backtrack_hits["fraction"].data, 0.0)
    backtrack_mask_arr = np.ma.getmask(backtrack_masked)
    other_dict["matches"] = backtrack_masked.count(axis=1).astype("uint16")
    # Fractions - note that "packet" is not always right terminology, e.g. with merged hits. Keeping nomenclature.
    other_dict["hit_packetFrac"] = (
        backtrack_hits["fraction"].data[~backtrack_mask_arr].astype("float32")
    )
    # Get the segment IDs then get the segments themselves
    segmentIDs = backtrack_hits["segment_ids"].data[~backtrack_mask_arr]
    all_segments = f["mc_truth/segments/data"]
    all_segments = all_segments[np.where(all_segments["event_id"] == spillID)]
    all_segmentIDs = all_segments["segment_id"]

    if len(all_segmentIDs) > 0 and len(segmentIDs) > 0:
        sorter = np.argsort(all_segmentIDs)
        insert_indices = np.searchsorted(all_segmentIDs, segmentIDs, sorter=sorter)
        insert_indices = np.clip(insert_indices, 0, len(all_segmentIDs) - 1)
        segments_where = sorter[insert_indices]
    else:
        segments_where = np.array([], dtype=int)

    other_dict["hit_pdg"] = all_segments["pdg_id"][segments_where].astype("int32")
    other_dict["hit_segmentID"] = all_segments["segment_id"][segments_where].astype(
        "int64"
    )
    other_dict["hit_particleID"] = all_segments["file_traj_id"][segments_where].astype(
        "int64"
    )
    other_dict["hit_particleIDLocal"] = all_segments["traj_id"][segments_where].astype(
        "int64"
    )
    other_dict["hit_vertexID"] = all_segments["vertex_id"][segments_where].astype(
        "int64"
    )

    # Secondly, event-level truth information.
    traj_indicesArray = np.where(
        flow_out["mc_truth/trajectories/data"]["event_id"] == spillID
    )[0]
    traj = flow_out["mc_truth/trajectories/data"][traj_indicesArray]
    other_dict["mcp_startx"] = (traj["xyz_start"][:, 0]).astype("float32")
    other_dict["mcp_starty"] = (traj["xyz_start"][:, 1]).astype("float32")
    other_dict["mcp_startz"] = (traj["xyz_start"][:, 2]).astype("float32")
    other_dict["mcp_endx"] = (traj["xyz_end"][:, 0]).astype("float32")
    other_dict["mcp_endy"] = (traj["xyz_end"][:, 1]).astype("float32")
    other_dict["mcp_endz"] = (traj["xyz_end"][:, 2]).astype("float32")
    other_dict["mcp_id"] = (traj["file_traj_id"]).astype("int64")
    other_dict["mcp_idLocal"] = (traj["traj_id"]).astype("int64")
    other_dict["mcp_pdg"] = (traj["pdg_id"]).astype("int32")
    other_dict["mcp_energy"] = (traj["E_start"] * MEV2GEV).astype("float32")
    other_dict["mcp_px"] = (traj["pxyz_start"][:, 0] * MEV2GEV).astype("float32")
    other_dict["mcp_py"] = (traj["pxyz_start"][:, 1] * MEV2GEV).astype("float32")
    other_dict["mcp_pz"] = (traj["pxyz_start"][:, 2] * MEV2GEV).astype("float32")
    other_dict["mcp_vertex_id"] = (traj["vertex_id"]).astype("int64")
    other_dict["mcp_nuid"] = other_dict["mcp_vertex_id"]
    other_dict["mcp_mother"] = (traj["parent_id"]).astype("int64")

    # Finally, vertex-level truth information.
    vertex_indicesArray = np.where(
        flow_out["/mc_truth/interactions/data"]["event_id"] == spillID
    )[0]
    vtx = flow_out["/mc_truth/interactions/data"][vertex_indicesArray]
    other_dict["vertex_id"] = (vtx["vertex_id"]).astype("int64")
    other_dict["nuID"] = other_dict["vertex_id"]
    if args.legacy_mode != LegacyMode.PRE_MINIRUN6.value:
        other_dict["nuvtxx"] = (vtx["x_vert"]).astype("float32")
        other_dict["nuvtxy"] = (vtx["y_vert"]).astype("float32")
        other_dict["nuvtxz"] = (vtx["z_vert"]).astype("float32")
    else:
        other_dict["nuvtxx"] = (vtx["vertex"][:, 0]).astype("float32")
        other_dict["nuvtxy"] = (vtx["vertex"][:, 1]).astype("float32")
        other_dict["nuvtxz"] = (vtx["vertex"][:, 2]).astype("float32")
    other_dict["nue"] = (vtx["Enu"] * MEV2GEV).astype("float32")
    other_dict["nuPDG"] = (vtx["nu_pdg"]).astype("int32")
    other_dict["nupx"] = (vtx["nu_4mom"][:, 0] * MEV2GEV).astype("float32")
    other_dict["nupy"] = (vtx["nu_4mom"][:, 1] * MEV2GEV).astype("float32")
    other_dict["nupz"] = (vtx["nu_4mom"][:, 2] * MEV2GEV).astype("float32")

    # Neutrino interaction type coding
    # Little bit of gymnastics here
    ccnc = vtx["isCC"]
    other_dict["ccnc"] = np.invert(ccnc).astype("int32")
    # And more gymnastics here
    codes = 1000 * np.ones(len(other_dict["vertex_id"]), dtype="int32")
    idxQE = np.where(vtx["isQES"])
    idxRES = np.where(vtx["isRES"])
    idxDIS = np.where(vtx["isDIS"])
    idxMEC = np.where(vtx["isMEC"])
    idxCOH = np.where(vtx["isCOH"])
    idxCOHQE = np.where((vtx["isCOH"]) & (vtx["isQES"]))
    codes[idxQE] = 0
    codes[idxRES] = 1
    codes[idxDIS] = 2
    codes[idxCOH] = 3
    codes[idxCOHQE] = 4
    codes[idxMEC] = 10
    other_dict["mode"] = codes


def insert_empty_mc(other_dict: dict) -> None:
    """
    To keep the structure consistent, insert empty MC truth arrays when processing data.
    Uses the pre-defined EMPTY_MC_DATA to save re-allocation time.

    Args:
        other_dict: Dictionary to add empty MC truth arrays to (modified in-place)
    """
    for key in EMPTY_MC_DATA:
        other_dict[key] = EMPTY_MC_DATA[key]


def process_file(
    args: argparse.Namespace,
    file_index: int,
    input_file: str,
    output_file: ur.WritableDirectory,
) -> None:
    """
    Process a single HDF5 file and write events to ROOT format.

    Args:
        args: Command-line arguments
        file_index: Index of this file in the input list (for first-write detection)
        input_file: Path to input HDF5 file
        output_file: Uproot writable directory for output
    """
    # Open the HDF5 file
    f = h5.File(input_file, "r")
    events = f["charge/events/data"]
    flow_out = h5flow.data.H5FlowDataManager(input_file, "r")

    num_events = len(events)

    # Get the array of the trigger type for every event in the file
    triggerIDs_data = flow_out["charge/events", "charge/ext_trigs", events["id"][:]]
    triggerIDs_all = np.array(np.ma.getdata(triggerIDs_data["iogroup"]), dtype="int32")
    triggerIDs = np.array(np.broadcast_to(INVALID_TRIGGER_ID, shape=num_events))

    # Determine the trigger ID for each event
    mask_nonzero = np.sum(triggerIDs_all, axis=1) != 0
    triggerIDs[mask_nonzero] = triggerIDs_all[mask_nonzero, 0]
    mask_beam = np.any(triggerIDs_all == TRIGGER_ID_BEAM, axis=1)
    triggerIDs[mask_beam] = TRIGGER_ID_BEAM

    # Batch writes, to minimize overhead
    batch_accumulator = defaultdict(list)

    # Process each event
    for i_event in range(num_events):
        bad_event = False

        if i_event % PROGRESS_INTERVAL == 0:
            print(f"Processing event {i_event} of {num_events}")

        event = events[i_event]
        event_calib_prompt_hits = flow_out[
            "charge/events",
            "charge/calib_" + args.hit_type + "_hits",
            events["id"][i_event],
        ]

        # Check we actually have some hits for this event
        if len(event_calib_prompt_hits[0]) == 0:
            print(
                f"This event seems to be empty, setting bad_event to True. Trigger type ({triggerIDs[i_event]})"
            )
            bad_event = True

        # Prepare hit arrays for good events
        hits_x = hits_y = hits_z = hits_Q = hits_E = hits_ts = hits_ids = None

        # Make sure that we have unmasked hits
        if not bad_event and np.ma.count_masked(event_calib_prompt_hits["z"][0]) == len(
            event_calib_prompt_hits[0]
        ):
            print(
                "This event has a hit z array ( len hits =",
                len(event_calib_prompt_hits[0]),
                ") that appears to be only masked values, setting as bad event. Trigger type (",
                triggerIDs[i_event],
                ")",
            )
            bad_event = True

        if not bad_event:
            hits_x = (
                np.ma.getdata(event_calib_prompt_hits["x"][0]) + TRUE_X_OFFSET
            ).astype("float32")
            hits_y = (
                np.ma.getdata(event_calib_prompt_hits["y"][0]) + TRUE_Y_OFFSET
            ).astype("float32")
            hits_z = (
                np.ma.getdata(event_calib_prompt_hits["z"][0]) + TRUE_Z_OFFSET
            ).astype("float32")
            hits_Q = np.ma.getdata(event_calib_prompt_hits["Q"][0]).astype("float32")
            hits_E = np.ma.getdata(event_calib_prompt_hits["E"][0]).astype("float32")
            hits_ts = np.ma.getdata(event_calib_prompt_hits["ts_pps"][0]).astype(
                "float32"
            )
            hits_ids = np.ma.getdata(event_calib_prompt_hits["id"][0])

        # Check we have multiple hit ids
        if not bad_event and len(hits_ids) < MIN_HITS_REQUIRED:
            print(
                f"This event seems to have less than 2 hits, setting bad_event to True. Trigger type ({triggerIDs[i_event]})"
            )
            bad_event = True

        # Get the spill ID, so we can move on to the event-level info
        spillID = 0

        if not bad_event and not args.is_data:
            unmaskedSpillIDs = []

            if args.hit_type == HitType.PROMPT.value:
                allSpillIDs = flow_out[
                    "charge/calib_prompt_hits",
                    "charge/packets",
                    "mc_truth/segments",
                    hits_ids,
                ]["event_id"]
                unmaskedSpillIDs = allSpillIDs.data[~allSpillIDs.mask]
            else:
                event_hits_prompt = flow_out[
                    "charge/events/", "charge/calib_prompt_hits", events["id"][i_event]
                ]
                hits_ids_prompt = np.ma.getdata(event_hits_prompt["id"][0])
                allSpillIDs = flow_out[
                    "charge/calib_prompt_hits",
                    "charge/packets",
                    "mc_truth/segments",
                    hits_ids_prompt,
                ]["event_id"]
                unmaskedSpillIDs = allSpillIDs.data[~allSpillIDs.mask]

            # Check we have some unmasked spill IDs
            if len(unmaskedSpillIDs) > 0:
                spillID = unmaskedSpillIDs[0]
            else:
                print(
                    f"No valid spillID found for this event, setting bad_event to True. Trigger type ({triggerIDs[i_event]})"
                )
                bad_event = True

        # Start on the event info
        runID = np.array([0], dtype="int32")
        subrunID = np.array([0], dtype="int32")
        eventID = np.array([event["id"]], dtype="int32")
        triggerID = np.array([triggerIDs[i_event]], dtype="int32")

        maxTimeFromTrigger = np.max(np.array([event["ts_start"]], dtype="float64"))
        useTimeFromTrigger = True if maxTimeFromTrigger < INVALID_TRIGGER_ID else False

        event_start_t = event_end_t = event_unix_ts = event_unix_ts_usec = None

        if triggerID != INVALID_TRIGGER_ID or useTimeFromTrigger:
            event_start_t = np.array([event["ts_start"]], dtype="int32")
            event_end_t = np.array([event["ts_end"]], dtype="int32")
            event_unix_ts = np.array([event["unix_ts"]], dtype="int32")

            if args.legacy_mode == LegacyMode.NONE.value:
                event_unix_ts_usec = np.array([event["unix_ts_usec"]], dtype="int32")

        # Prepare dictionaries for uproot
        event_dict = {
            "run": runID,
            "subrun": subrunID,
            "event": eventID,
            "triggers": triggerID,
            "unix_ts": get_var_if_set(event_unix_ts, "int32", [INVALID_TIME_VALUE]),
            "event_start_t": get_var_if_set(
                event_start_t, "int32", [INVALID_TIME_VALUE]
            ),
            "event_end_t": get_var_if_set(event_end_t, "int32", [INVALID_TIME_VALUE]),
        }
        other_dict = {
            "x": get_var_if_set(hits_x),
            "y": get_var_if_set(hits_y),
            "z": get_var_if_set(hits_z),
            "ts": get_var_if_set(hits_ts),
            "charge": get_var_if_set(hits_Q),
            "E": get_var_if_set(hits_E),
        }

        # Add unix_ts_usec if not in legacy mode
        if args.legacy_mode == LegacyMode.NONE.value:
            event_dict["unix_ts_usec"] = get_var_if_set(
                event_unix_ts_usec, "int32", [INVALID_TIME_VALUE]
            )

        # Inject MC info for non-data files.
        if not bad_event and not args.is_data:
            process_mc(args, f, flow_out, hits_ids, spillID, other_dict)
        elif bad_event and not args.is_data:
            insert_empty_mc(other_dict)

        # Start the storing process, for later writing
        max_entries = 0
        for key in other_dict:
            max_entries = max(max_entries, len(other_dict[key]))

        max_data_len = MAX_ARRAY_DEPTH_DATA if args.is_data else MAX_ARRAY_DEPTH
        n_sub_events = int(max_entries / max_data_len) + 1

        for i_sub in range(n_sub_events):
            first = max_data_len * i_sub
            last = max_data_len * (i_sub + 1)

            # Copy dict to avoid reference issues
            sub_event_dict = event_dict.copy()
            sub_event_dict["subevent"] = np.array([i_sub], dtype="int32")

            # Populate with data slices
            for key in other_dict.keys():
                # Directly append the numpy array slice.
                # Conversion to Awkward Array happens once at end-of-file.
                sub_event_dict[key] = other_dict[key][first:last]

            # Accumulate in batch
            for key, val in sub_event_dict.items():
                batch_accumulator[key].append(val)

    # Write entire file's worth of events in one go
    if batch_accumulator:
        final_output = {}
        for key, val_list in batch_accumulator.items():
            # Create the Jagged Array structure efficiently from the list of numpy arrays
            final_output[key] = awk.Array(val_list)

        if "subevents" not in output_file:
            output_file.mktree("subevents", final_output)
        else:
            output_file["subevents"].extend(final_output)

    # Close the HDF5 file
    f.close()


def main():
    args = parse_args()

    input_files = args.file_list.split(",")
    output_file = ur.recreate(args.output_file)

    for i, input_file in enumerate(input_files):
        print(f"Processing file {i + 1} of {len(input_files)}")
        process_file(args, i, input_file, output_file)

    # Finally, close the output ROOT file
    output_file.close()
    print(f"Conversion complete. Output written to {args.output_file}")


if __name__ == "__main__":
    main()
