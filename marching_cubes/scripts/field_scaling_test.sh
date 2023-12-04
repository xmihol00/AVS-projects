
files=(
    4
    8
    16
    32
)

i=0
k=1
g=64
make clean
#make -j DEFS="-DPRINT_DEFINES -DVECTORIZED_SIMD=1 -DSEPARATE_MEMORY=1 -DSCHEDULE=guided"
make -j DEFS="-DCAPTURE_AND_ADD"
for thread in 4 8 16 32; do
    echo "" > ${files[$i]}_$g.txt

    for j in {1..5}; do
        ./PMC ../data/bun_zipper_res${k}.pts loop.obj -b loop -g $g -t $thread >> loop_${files[$i]}_$g.txt 
        ./PMC ../data/dragon_vrip_res${k}.pts loop.obj -b loop -g $g -t $thread >> loop_${files[$i]}_$g.txt 

        ./PMC ../data/bun_zipper_res${k}.pts tree.obj -b tree -g $g -t $thread >> tree_${files[$i]}_$g.txt 
        ./PMC ../data/dragon_vrip_res${k}.pts tree.obj -b tree -g $g -t $thread >> tree_${files[$i]}_$g.txt 
    done

    i=$((i+1))
    k=$((k+1))
done
