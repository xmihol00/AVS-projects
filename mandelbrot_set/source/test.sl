#!/bin/bash
#SBATCH -p qcpu_exp
#SBATCH -A DD-23-135
#SBATCH -n 1 
#SBATCH -t 1:00:00
#SBATCH --mail-type END
#SBATCH -J AVS-test


cd $SLURM_SUBMIT_DIR
ml intel-compilers/2022.1.0 CMake/3.23.1-GCCcore-11.3.0 Python # pouze na Barbore

[ -d build_test_elipse ] && rm -rf build_test_elipse
[ -d build_test_elipse ] || mkdir build_test_elipse

cd build_test_elipse
rm tmp_*

CC=icc CXX=icpc cmake ..
make > make_log.txt

SHAPES=(1 2 4 8 16 32 64 128 256 432 512 751 1024 1424 1500 1511 2048 2049 3968 4001 4096 5234 6999 8192)
ITERS=(100 500 1000)
CALCULATORS=("ref" "line" "batch")

echo "" > results.txt
echo "" > calculations.txt

for shape in "${SHAPES[@]}"; do
    for iter in "${ITERS[@]}"; do
        for calc in "${CALCULATORS[@]}"; do
            ./mandelbrot -s $shape -i $iter -c $calc -o $calc.npz 2>>calculations.txt >> calculations.txt
	done
        echo "$shape $iter $calc"                        | tee -a results.txt calculations.txt
        echo "Ref vs Line:"                              | tee -a results.txt calculations.txt
        python3 ../scripts/compare.py ref.npz line.npz   | tee -a results.txt calculations.txt
        echo "Ref vs Batch:"                             | tee -a results.txt calculations.txt
        python3 ../scripts/compare.py ref.npz batch.npz  | tee -a results.txt calculations.txt
        echo "Line vs Batch:"                            | tee -a results.txt calculations.txt
        python3 ../scripts/compare.py line.npz batch.npz | tee -a results.txt calculations.txt
        echo ""                                          | tee -a results.txt calculations.txt
    done
done

rm -r *.npz CM* mandelbrot cmake* Makefile ipo_out.optrpt
