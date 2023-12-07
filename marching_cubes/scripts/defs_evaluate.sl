#!/bin/bash
#SBATCH -p qcpu_exp
#SBATCH -A DD-23-135
#SBATCH -n 1 
#SBATCH -t 1:00:00
#SBATCH --mail-type END
#SBATCH -J AVS-defs-evaluate

cd $SLURM_SUBMIT_DIR

ml intel-compilers/2022.1.0 CMake/3.23.1-GCCcore-11.3.0 # pouze na Barbore
ml matplotlib/3.5.2-foss-2022a

export OMP_PROC_BIND=close 
export OMP_PLACES=cores

mkdir -p plots

defs=(
    ""
    "-DCAPTURE_AND_ADD=1"
    "-DSEPARATE_MEMORY=1"
    "-DVECTORIZED_SIMD=1"
    "-DVECTORIZED_SIMD=1 -DCAPTURE_AND_ADD=1"
    "-DVECTORIZED_SIMD=1 -DSEPARATE_MEMORY=1"
)

files=(
    "ciritical_section"
    "capture_and_add"
    "separate_memory"
    "ciritical_section_vectorized"
    "capture_and_add_vectorized"
    "separate_memory_vectorized"
)

for def in "${defs[@]}"; do
    make clean
    make -j "DEFS=$def"

    bash ../scripts/generate_data_and_plots.sh

    mv ../*.png ./plots
    mv ./plots/input_scaling_weak.png ${files[$i]}_weak.png
    mv ./plots/input_scaling_strong.png ${files[$i]}_strong.png
    mv ./plots/grid_scaling.png ${files[$i]}_grid.png

    i=$((i+1))
done
