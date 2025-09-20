#!/bin/python

import argparse
import pandas as pd
import requests
import time

def get_true_random(min_val, max_val):
    url = f"https://www.random.org/integers/?num=1&min={min_val}&max={max_val}&col=1&base=10&format=plain&rnd=new"
    try:
        r = requests.get(url, timeout=5)  # Add timeout to avoid hanging
        r.raise_for_status()  # Raises HTTPError for bad responses
        return int(r.text.strip())
    except (requests.RequestException, ValueError) as e:
        print(f"Error fetching random number: {e}")
        return None

def parse_args():
    parser = argparse.ArgumentParser(
        prog="gen_proj_name",
        description="Generate a unique project name based on a Saskatchewan town.",
        usage="gen_proj_name"
    )

    return parser

def gen_name():
    should_save_to_file = False

    df = pd.read_csv("ProjectNames.csv")
    # Filter rows where "Used" is NaN
    df = df[df["Used"].isna()]
    if df.empty:
        raise RuntimeError("No free names!")

    # Get the "Town names" column as a list
    town_list = df["Town name"].tolist()

    for i in range(3):
            random_val = get_true_random(0, len(town_list) - 1)
            if random_val is not None:
                break
            time.sleep(5)

    if random_val is None:
        raise RuntimeError("Could not generate a valid random index.")

    selected_name = town_list[random_val]
    print("Town name :", selected_name)

    # save to file
    yes_no = input("Save to file? (y/n) ").lower().strip()
    if yes_no in ["y", "ye", "yes"]:
        print("saving...")
        df.loc[df["Town name"] == selected_name, "Used"] = True
        should_save_to_file = True
        
    if should_save_to_file:
        df.to_csv("ProjectNames.csv", index=False)
    

if __name__ == "__main__":
    parser = parse_args()
    parser.parse_args()

    gen_name()
