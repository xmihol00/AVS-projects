#!/bin/bash
#SBATCH -p qcpu_exp
#SBATCH -A DD-23-135
#SBATCH -n 1 
#SBATCH --comment "use:vtune=2022.2.0"
#SBATCH -t 1:00:00
#SBATCH --mail-type END
#SBATCH -J AVS-vtune

defs=(
    "-DVECTORIZED_SIMD=1"
    "-DVECTORIZED_SIMD=1 -DCAPTURE_AND_ADD=1"
    "-DVECTORIZED_SIMD=1 -DSEPARATE_MEMORY=1"
)
dirs=(
    "critical_section"
    "capture_and_add"
    "separate_memory"
)

cd $SLURM_SUBMIT_DIR

ml intel-compilers/2022.1.0 CMake/3.23.1-GCCcore-11.3.0 VTune/2022.2.0-intel-2021b

for dir in "${dirs[@]}"; do
    mkdir -p ${dir}
done

i=0
for def in "${defs[@]}"; do
    make clean
    make -j DEFS="$def"

    for threads in 18 36; do
        for builder in "ref" "loop" "tree"; do
            rm -rf ${dirs[$i]}/vtune-${builder}-${threads}
            vtune -collect threading -r ${dirs[$i]}/vtune-${builder}-${threads} -app-working-dir . -- ./PMC --builder ${builder} -t ${threads} --grid 128 ../data/bun_zipper_res3.pts
        done
    done

    i=$((i+1))
done
