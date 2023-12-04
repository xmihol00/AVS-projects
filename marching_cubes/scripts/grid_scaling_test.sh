
files=(
    2
    4
    8
    16
    32
)

i=0
make clean
#make -j DEFS="-DPRINT_DEFINES -DVECTORIZED_SIMD=1 -DSEPARATE_MEMORY=1 -DSCHEDULE=guided"
make -j DEFS="-DCAPTURE_AND_ADD"
for g in 16 32 64 128 256; do
    echo "" > ${files[$i]}_$g.txt

    for j in {1..5}; do
        ./PMC ../data/bun_zipper_res4.pts loop.obj -b loop -g $g -t ${files[$i]} >> loop_${files[$i]}_$g.txt 
        ./PMC ../data/dragon_vrip_res4.pts loop.obj -b loop -g $g -t ${files[$i]} >> loop_${files[$i]}_$g.txt 
        ./PMC ../data/bun_zipper_res3.pts loop.obj -b loop -g $g -t ${files[$i]} >> loop_${files[$i]}_$g.txt 
        ./PMC ../data/dragon_vrip_res3.pts loop.obj -b loop -g $g -t ${files[$i]} >> loop_${files[$i]}_$g.txt 

        ./PMC ../data/bun_zipper_res4.pts tree.obj -b tree -g $g -t ${files[$i]} >> tree_${files[$i]}_$g.txt 
        ./PMC ../data/dragon_vrip_res4.pts tree.obj -b tree -g $g -t ${files[$i]} >> tree_${files[$i]}_$g.txt 
        ./PMC ../data/bun_zipper_res3.pts tree.obj -b tree -g $g -t ${files[$i]} >> tree_${files[$i]}_$g.txt 
        ./PMC ../data/dragon_vrip_res3.pts tree.obj -b tree -g $g -t ${files[$i]} >> tree_${files[$i]}_$g.txt 
    done

    i=$((i+1))
done
