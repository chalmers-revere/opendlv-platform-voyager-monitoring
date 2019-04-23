#!/bin/bash
#
# Author: Kamil Wilczek
# E-mail: kamil.van.wilczek@gmail.com
#
# This script is a collectd plugin that gathers metrics
# from nVidia cards.

HOSTNAME="apollo"                              
INTERVAL="${COLLECTD_INTERVAL:-10}"         
TIMESTAMP=$(date +%s)

declare -A config=(
	["temperature_gpu"]=temperature
	["fan_speed"]=percent
	["memory_used"]=memory
	["memory_free"]=memory
	["utilization_gpu"]=percent
	["utilization_memory"]=percent
)

# This parameter will always be present. It is a way to
# uniquely identify a GPU card (very useful in multiGPU
# machines). nvidia-smi also accepts this ID as a parameter
# in many queries. Alternatively, one can use 'uuid'.
query_string="pci.bus_id,"

# Building the nvidia-smi --query-gpu string.
# The last comma is removed during variable expansion
# in the query.
for parameter in "${!config[@]}"
do
	query_string+="${parameter//_/.},"
done

# Current state of all GPUs.
gpus_state=$(nvidia-smi --query-gpu="${query_string%,}" --format=csv,noheader,nounits)

# Output for collectd.
# Bash variable indirection is used here.
# For example, variable name 'power_draw' is used as a parameter in
# collectd protocol and it also has a value assigned to that name.
while IFS=',' read -r gpu_id "${!config[@]}"
do
	for parameter in "${!config[@]}"
	do
		# GPU power state, 'pstate', is returned as a number with
		# 'P' prefix. The easiest way to remove the 'P' without 
		# additional loops and conditions, is to simply expand the
		# 'parameter' variable with '//P'.
		#echo "PUTVAL ${HOSTNAME}/cuda-0/${config[$parameter]}-${parameter} interval=$INTERVAL N:${!parameter//P}" | tee -a /tmp/state.log
		echo "PUTVAL ${HOSTNAME}/cuda-${gpu_id}/${config[$parameter]}-${parameter} interval=$INTERVAL N:${!parameter//P}" | tee -a /tmp/state.log
	done
done <<< "${gpus_state// }"


