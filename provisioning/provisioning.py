import os
import sys
import argparse

IDF_PATH = os.getenv("IDF_PATH")
PART_TOOL_PATH = os.path.join(IDF_PATH, "components", "partition_table")
sys.path.append(PART_TOOL_PATH)

from parttool import *

PARTITION_NAME = "dev_config"

DEV_CONF_BIN_FILE = "device_config.bin"
DEV_CONF_CSV_FILE = "device_config.csv"
DEV_CONF_STORAGE_NAME = "config"
DEV_CONF_SN = 5

def write_config_to_target(dev="COM9", SN=DEV_CONF_SN):
    dev_config_str = f'# AAD csv file\nkey,type,encoding,value\n{DEV_CONF_STORAGE_NAME},namespace,,\nSN,data,u32,{SN}'
    f = open(DEV_CONF_CSV_FILE, "w")
    f.write(dev_config_str)
    f.close()
    result = os.system(
        f"python {IDF_PATH}/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py generate {DEV_CONF_CSV_FILE} {DEV_CONF_BIN_FILE} 0x3000"
    )
    print(result)
    target = ParttoolTarget(dev)
    target.write_partition(PartitionName(PARTITION_NAME), DEV_CONF_BIN_FILE)

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-p", "--port", help="COM port", type=str)
    parser.add_argument("-s", "--serial", help="Device serial number", type=int)
    args = parser.parse_args()
    if args.port is None or args.serial is None:
        print("run: provisioning.py -p (PORT) -s (SERIAL NUMBER)")
        exit(-1)
    write_config_to_target(args.port, args.serial)
