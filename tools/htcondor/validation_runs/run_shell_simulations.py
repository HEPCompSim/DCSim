import argparse
import ast
import glob
import os
import re
import subprocess
import tarfile
from typing import Callable

def generate_dcsim_args(
        args: argparse.Namespace,
        line: tuple[dict[str, float], float],
        iline: int,
        hitrate: float,
        platform_generator: Callable
) -> list[str]:
    """
    Generates the command line arguments for dcsim based on the provided parameters.
    Args:
        args (argparse.Namespace): Parsed command line arguments.
        line (tuple[dict[str, float], float]): The current line in the calibration shell file \
                                    containing the calibration parameters in a dict \
                                    and the respective achieved loss value.
        hitrate (float): The hitrate value for the simulation.
        platform_generator (function): Function to generate platform configuration.
    Returns:
        list: A list of command line arguments for dcsim.
    """
    if isinstance(line, tuple):
        calibration = line[0]
        # loss = line[1]
    else:
        raise ValueError(
            "Expected line to be a tuple containing calibration parameters and calibration loss.",
            f" But got {type(line)} instead.",
            f" Line content: {line}"
        )

    #TODO: make sure that in right working directory, for now basename is used
    platform = os.path.basename(args.platform)
    shell = os.path.basename(args.shell)

    return [
        "--platform", platform_generator(platform, calibration),
        "--output-file", f"{platform.split('.')[0]}_{shell.split('.')[0]}_CP{iline}_hitrate{hitrate}.csv",
        "--workload-configurations", args.workload,
        "--dataset-configurations", args.dataset,
        "--hitrate", str(hitrate),
        "--xrd-blocksize", str(args.xrd_blocksize),
        "--storage-buffer-size", str(args.storage_buffer_size),
        "--duplications", "48",
        "--cfg=network/loopback-bw:100000000000000",
        "--no-caching",
        "--seed", "0",
        "--xrd-flops-per-time", str(calibration["xrootd_flops"])
    ]


def generate_platform(platform_file: str|os.PathLike, calibration_params: dict, scenario = "fcfn") -> str|os.PathLike:
    """
    Fills in the missing values in the templated platform file based on the provided calibration parameters.
    Args:
        platform_file (str|os.PathLike): Path to the platform file.
        calibration_params (dict): Dictionary containing calibration parameters.
    Returns:
        str|os.PathLike: The path to the modified platform file.
    """
    # adjust the choice of calibration parameter keys depending on the scenario of simulation
    if "sc" in scenario :
        read_key = "disk"
    elif "fc" in scenario:
        read_key = "ramDisk"
    else:
        raise ValueError(f"Unknown cache scenario {scenario}. Expected 'fc' or 'sc'.")
    if "sn" in scenario:
        net_key = "externalSlowNetwork"
    elif "fn" in scenario:
        net_key = "externalFastNetwork"
    else:
        raise ValueError(f"Unknown network scenario {scenario}. Expected 'sn' or 'fn'.")

    if not os.path.exists(platform_file):
        raise FileNotFoundError(f"Platform file {platform_file} does not exist.")
    # Read the platform file and replace the placeholders with actual values
    with open(platform_file, 'r') as file:
        platform = file.read()
        # Replace the placeholders in the XML file with the specified values
        platform = re.sub(r'{cpu-speed}', str(calibration_params["cpuSpeed"]), platform)
        platform = re.sub(r'{read-speed}', str(calibration_params[read_key]), platform)
        platform = re.sub(r'{link-speed}', str(calibration_params["internalNetwork"]), platform)
        platform = re.sub(r'{net-speed}', str(calibration_params[net_key]), platform)

    # Write the modified platform to a temporary file
    platform_file_tmp = f"{os.path.basename(platform_file)}.tmp"
    with open(platform_file_tmp, 'w') as file:
        file.write(platform)

    return platform_file_tmp


def process_list(file_path, from_line, to_line, dcsim_args_generator):
    """
    Reads a file containing a list of tuples and processes a slice of it.

    Args:
        file_path (str): The path to the file.
        from_line (int): The starting line number (inclusive).
        to_line (int): The ending line number (exclusive).
        dcsim_args_generator (function): A function that generates dcsim arguments.
    """
    try:
        with open(file_path, 'r') as f:
            content = f.read()
            data = ast.literal_eval(content)
            
        if not isinstance(data, list):
            raise TypeError(f"Error: The file {file_path} does not contain a list.")

    except FileNotFoundError as e:
        raise FileNotFoundError(f"Shell file not found at {file_path}") from e
    except (ValueError, SyntaxError) as e:
        raise Exception(f"Error parsing shell file {file_path}") from e

    # Process the specified slice of the list
    for i, item in enumerate(data[from_line:to_line]):
        line = from_line + i
        # Run the simulations for each hitrate value
        if isinstance(item, tuple):
            # Assuming item is a tuple with calibration parameters and loss value
            for hitrate in args.hitrates:
                dcsim_args = dcsim_args_generator(line=item, iline=line, hitrate=hitrate)
                print(f"Processing line {line} with hitrate {hitrate} and calibration {item}")
                print(f"Running process: dc-sim {' '.join(dcsim_args)}")
                try:
                    completed_process = subprocess.run(['dc-sim'] + dcsim_args, check=True, capture_output=True)
                    print(f"Simulation output:\n{completed_process.stdout.decode()}")
                    print(f"Simulation error:\n{completed_process.stderr.decode()}")
                except subprocess.CalledProcessError as e:
                    raise RuntimeError(
                        f"Error occurred while running simulation for line {line} \
                            with hitrate {hitrate} and calibration {item}"
                    ) from e
        else:
            raise ValueError(f"Expected item to be a tuple, but got {type(item)} instead. Item content: {item}")
    print("All simulations completed successfully.")


if __name__ == "__main__":
    data_path = os.path.join("/", "home", "DCSim", "data")
    parser = argparse.ArgumentParser(description="Run simulations with specified parameters.")
    parser.add_argument("--platform", type=str,
                        default=os.path.join(data_path, "platform-files", "sgbatch_validation_template.xml"),
                        help="Platform name")
    parser.add_argument("--workload", type=str,
                        default=os.path.join(data_path, "workload-configs", "crown_ttbar_slowjob.json"),
                        help="Workload configuration")
    parser.add_argument("--dataset", type=str,
                        default=os.path.join(data_path, "dataset-configs", "crown_ttbar_slowjob.json"),
                        help="Dataset configuration")
    parser.add_argument("--hitrates", type=str, nargs='+',
                        default=["0.0", "0.1", "0.2", "0.3", "0.4", "0.5", "0.6", "0.7", "0.8", "0.9", "1.0"],
                        help="Hitrate values")
    parser.add_argument("--xrd-blocksize", type=int, default=10000000000, help="XRootD block size")
    parser.add_argument("--storage-buffer-size", type=int, default=0, help="Storage buffer size")
    parser.add_argument("--duplications", type=int, default=48, help="Number of duplications")
    parser.add_argument("--seed", type=int, default=42, help="Random seed")
    parser.add_argument("--shell", type=str, required=True,
                        help="Path to the file containing the calibration shell")
    parser.add_argument("--from-line", type=int, required=True,
                        help="Starting line in the shell file list")
    parser.add_argument("--to-line", type=int, required=True,
                        help="Ending line in the shell file list")

    args = parser.parse_args()

    #TODO: make sure that in right working directory, for now basename is used
    platform = os.path.basename(args.platform)
    shell = os.path.basename(args.shell)

    platform_generator = lambda platform_file, calibration: generate_platform(platform_file, calibration)
    dcsim_args_generator = lambda line, iline, hitrate: generate_dcsim_args(args, line, iline, hitrate, platform_generator)

    process_list(shell, args.from_line, args.to_line, dcsim_args_generator)

    outfiles_pattern = f"{platform.split('.')[0]}*_{shell.split('.')[0]}*_CP*_hitrate*.csv"
    output_files = glob.glob(outfiles_pattern)
    print(f"Generated output files: {output_files}")
    # Tar the list of generated output files
    if output_files:
        tar_filename = f"{platform.split('.')[0]}_{shell.split('.')[0]}_{args.from_line}_{args.to_line}.csv.tar.gz"
        with tarfile.open(tar_filename, "w:gz") as tar:
            for file in output_files:
                tar.add(file, arcname=os.path.basename(file))
                os.remove(file)
        print(f"Successfully created tar archive: {tar_filename}")
    else:
        raise FileNotFoundError(f"No output files found matching the pattern {outfiles_pattern}")

