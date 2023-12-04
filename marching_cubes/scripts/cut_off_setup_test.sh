
defs=(
    "-DPRINT_DEFINES -DVECTORIZED_SIMD=1 -DSEPARATE_MEMORY=1 -DCUT_OFF_VALUE=1"
    "-DPRINT_DEFINES -DVECTORIZED_SIMD=1 -DSEPARATE_MEMORY=1 -DCUT_OFF_VALUE=2"
    "-DPRINT_DEFINES -DVECTORIZED_SIMD=1 -DSEPARATE_MEMORY=1 -DCUT_OFF_VALUE=4"
    "-DPRINT_DEFINES -DVECTORIZED_SIMD=1 -DSEPARATE_MEMORY=1 -DCUT_OFF_VALUE=8"
    "-DPRINT_DEFINES -DVECTORIZED_SIMD=1 -DSEPARATE_MEMORY=1 -DCUT_OFF_VALUE=16"
    "-DPRINT_DEFINES -DVECTORIZED_SIMD=1 -DSEPARATE_MEMORY=1 -DCUT_OFF_VALUE=32"
    "-DPRINT_DEFINES -DVECTORIZED_SIMD=1 -DSEPARATE_MEMORY=1 -DCUT_OFF_DYNAMIC"
)
files=(
    "cut_off_1"
    "cut_off_2"
    "cut_off_4"
    "cut_off_8"
    "cut_off_16"
    "cut_off_32"
    "cut_off_dynamic"
)

for g in 32 64; do
    i=0
    for def in "${defs[@]}"; do
        make clean
        make -j DEFS="$def"

        echo "Schedule: $def" > ${files[$i]}_$g.txt

        for j in {1..5}; do
            ./PMC ../data/bun_zipper_res1.pts tree.obj -b tree -g $g >> ${files[$i]}_$g.txt 
            ./PMC ../data/bun_zipper_res2.pts tree.obj -b tree -g $g >> ${files[$i]}_$g.txt 
            ./PMC ../data/bun_zipper_res3.pts tree.obj -b tree -g $g >> ${files[$i]}_$g.txt 
            ./PMC ../data/bun_zipper_res4.pts tree.obj -b tree -g $g >> ${files[$i]}_$g.txt 
            ./PMC ../data/dragon_vrip_res1.pts tree.obj -b tree -g $g >> ${files[$i]}_$g.txt 
            ./PMC ../data/dragon_vrip_res2.pts tree.obj -b tree -g $g >> ${files[$i]}_$g.txt 
            ./PMC ../data/dragon_vrip_res3.pts tree.obj -b tree -g $g >> ${files[$i]}_$g.txt 
            ./PMC ../data/dragon_vrip_res4.pts tree.obj -b tree -g $g >> ${files[$i]}_$g.txt 
        done

        i=$((i+1))
    done
done