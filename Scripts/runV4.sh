#!/bin/bash

# Executes RobotsV4 

if [ "$#" -ne 5 ]; then
    echo "Usage: $0 <grid_width> <grid_height> <num_robots> <num_doors> <num_simulations>"
    exit 1
fi

grid_width=$1
grid_height=$2
num_robots=$3
num_doors=$4
num_simulations=$5

# Check that the number of doors is between 1 and 10
if [ "$num_doors" -lt 1 ] || [ "$num_doors" -gt 10 ]; then
    echo "Error: Number of doors must be between 1 and 10"
    exit 1
fi

# Placeholder for your robot simulation program
robot_simulation_program() {
    simulation_index=$1
    # Replace the following line with the command to run your robot simulation program
    ./RobotsV4/robotsV4 $grid_width $grid_height $num_robots $num_doors $simulation_index
}

# Run the robot simulation program and save the output to numbered files
for i in $(seq 1 $num_simulations); do
    robot_simulation_program "$i"
done

