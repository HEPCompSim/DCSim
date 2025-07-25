import argparse
import ast
import os
import re
from typing import Callable

def generate_dcsim_args(
        args: argparse.Namespace,
        line: tuple[dict[str, float], float],
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

    return [
        "--platform", platform_generator(args.platform, calibration),
        "--output-file", f"{args.platform}_{args.shell}_{line}.csv",
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


def generate_platform(platform_file: str|os.PathLike, calibration_params: dict) -> str|os.PathLike:
    """
    Fills in the missing values in the templated platform file based on the provided calibration parameters.
    Args:
        platform_file (str|os.PathLike): Path to the platform file.
        calibration_params (dict): Dictionary containing calibration parameters.
    Returns:
        str|os.PathLike: The path to the modified platform file.
    """
    with open(platform_file, 'r') as file:
        platform = file.read()
        # Replace the placeholders in the XML file with the specified values
        platform = re.sub(r'{cpu-speed}', str(calibration_params["cpuSpeed"]), platform)
        platform = re.sub(r'{read-speed}', str(calibration_params["cacheSpeed"]), platform)
        platform = re.sub(r'{link-speed}', str(calibration_params["internalNetworkSpeed"]), platform)
        platform = re.sub(r'{net-speed}', str(calibration_params["externalNetworkSpeed"]), platform)
    
    with open(platform_file, 'w') as file:
        file.write(platform)
        
    return platform_file


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

        # Process the specified slice of the list
        for i, item in enumerate(data[from_line:to_line]):
            line = from_line + i
            # Run the simulations for each hitrate value
            if isinstance(item, tuple):
                # Assuming item is a tuple with calibration parameters and loss value
                for hitrate in args.hitrates:
                    dcsim_args = dcsim_args_generator(line=item, hitrate=hitrate)
                    print(f"Processing line {line} with hitrate {hitrate} and calibration {item}: {' '.join(dcsim_args)}")
                    raise NotImplementedError("Simulation execution logic is not implemented yet.")
            else:
                raise ValueError(f"Expected item to be a tuple, but got {type(item)} instead. Item content: {item}")

    except FileNotFoundError as e:
        raise FileNotFoundError(f"Error: File not found at {file_path}") from e
    except (ValueError, SyntaxError) as e:
        raise Exception(f"Error parsing file {file_path}") from e


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Run simulations with specified parameters.")
    parser.add_argument("--platform", type=str, required=True, help="Platform name")
    parser.add_argument("--workload", type=str, required=True, help="Workload configuration")
    parser.add_argument("--dataset", type=str, required=True, help="Dataset configuration")
    parser.add_argument("--hitrates", type=str, required=True, help="Hitrate values")
    parser.add_argument("--xrd-blocksize", type=int, default=10000000000, help="XRootD block size")
    parser.add_argument("--storage-buffer-size", type=int, default=0, help="Storage buffer size")
    parser.add_argument("--duplications", type=int, default=48, help="Number of duplications")
    parser.add_argument("--seed", type=int, default=42, help="Random seed")
    parser.add_argument("--shell", type=str, required=True, help="Path to the file containing the calibration shell")
    parser.add_argument("--from-line", type=int, required=True, help="Starting line in the shell file list")
    parser.add_argument("--to-line", type=int, required=True, help="Ending line in the shell file list")

    args = parser.parse_args()

    platform_generator = lambda platform_file, calibration: generate_platform(platform_file, calibration)
    dcsim_args_generator = lambda line, hitrate: generate_dcsim_args(args, line, hitrate, platform_generator)

    process_list(args.shell, args.from_line, args.to_line, dcsim_args_generator)
