
defs=(
    "-DPRINT_DEFINES -DSCHEDULE=dynamic" 
    "-DPRINT_DEFINES -DSCHEDULE=dynamic -DCHUNK_SIZE=8"
    "-DPRINT_DEFINES -DSCHEDULE=dynamic -DCHUNK_SIZE=16" 
    "-DPRINT_DEFINES -DSCHEDULE=dynamic -DCHUNK_SIZE=32"
    "-DPRINT_DEFINES -DSCHEDULE=dynamic -DCHUNK_SIZE=64"
)
files=(
    "dynamic_default"
    "dynamic_8"
    "dynamic_16"
    "dynamic_32"
    "dynamic_64"
)

i=0
for def in "${defs[@]}"; do
    make clean
    make -j DEFS="$def"

    g=64
    echo "Schedule: $def" >> ${files[$i]}_$g.txt

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
