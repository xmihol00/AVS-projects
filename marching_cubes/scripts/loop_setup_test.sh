
defs=(
    "-DPRINT_DEFINES"
    "-DPRINT_DEFINES -DCAPTURE_AND_ADD=1"
    "-DPRINT_DEFINES -DSEPARATE_MEMORY=1"
    "-DPRINT_DEFINES -DVECTORIZED_SIMD=1"
    "-DPRINT_DEFINES -DVECTORIZED_SIMD=1 -DCAPTURE_AND_ADD=1"
    "-DPRINT_DEFINES -DVECTORIZED_SIMD=1 -DSEPARATE_MEMORY=1"
)
files=(
    "critical_section"
    "capture_and_add"
    "separate_memory"
    "critical_section_vectorized"
    "capture_and_add_vectorized"
    "separate_memory_vectorized"
)

i=0
for def in "${defs[@]}"; do
    make clean
    make -j DEFS="$def"

    g=64
    echo "Schedule: $def" > ${files[$i]}_$g.txt

    for j in {1..10}; do
        ./PMC ../data/bun_zipper_res1.pts loop.obj -b loop -g $g >> ${files[$i]}_$g.txt 
        ./PMC ../data/bun_zipper_res2.pts loop.obj -b loop -g $g >> ${files[$i]}_$g.txt 
        ./PMC ../data/bun_zipper_res3.pts loop.obj -b loop -g $g >> ${files[$i]}_$g.txt 
        ./PMC ../data/bun_zipper_res4.pts loop.obj -b loop -g $g >> ${files[$i]}_$g.txt 
        ./PMC ../data/dragon_vrip_res1.pts loop.obj -b loop -g $g >> ${files[$i]}_$g.txt 
        ./PMC ../data/dragon_vrip_res2.pts loop.obj -b loop -g $g >> ${files[$i]}_$g.txt 
        ./PMC ../data/dragon_vrip_res3.pts loop.obj -b loop -g $g >> ${files[$i]}_$g.txt 
        ./PMC ../data/dragon_vrip_res4.pts loop.obj -b loop -g $g >> ${files[$i]}_$g.txt 
    done

    i=$((i+1))
done
