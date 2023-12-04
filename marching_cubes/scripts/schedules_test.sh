
defs=(
    "-DSCHEDULE=static"  #"-DSCHEDULE=static -DCHUNK_SIZE=1" "-DSCHEDULE=static -DCHUNK_SIZE=16" "-DSCHEDULE=static -DCHUNK_SIZE=64" 
    "-DSCHEDULE=dynamic" #"-DSCHEDULE=dynamic -DCHUNK_SIZE=1" "-DSCHEDULE=dynamic -DCHUNK_SIZE=16" "-DSCHEDULE=dynamic -DCHUNK_SIZE=64" 
    "-DSCHEDULE=guided"  #"-DSCHEDULE=guided -DCHUNK_SIZE=1" "-DSCHEDULE=guided -DCHUNK_SIZE=16" "-DSCHEDULE=guided -DCHUNK_SIZE=64"
)
files=(
    "static"  #"static_1" "static_16" "static_64" 
    "dynamic" #"dynamic_1" "dynamic_16" "dynamic_64" 
    "guided"  #"guided_1" "guided_16" "guided_64"
)

i=0
for def in "${defs[@]}"; do
    make clean
    make -j DEFS="$def"

    #g=128
    #echo "Schedule: $def" > ${files[$i]}_$g.txt
#
    #for j in {1..10}; do
    #    ./PMC ../data/bun_zipper_res1.pts loop.obj -b loop -g $g >> ${files[$i]}_$g.txt
    #    ./PMC ../data/bun_zipper_res2.pts loop.obj -b loop -g $g >> ${files[$i]}_$g.txt
    #    ./PMC ../data/bun_zipper_res3.pts loop.obj -b loop -g $g >> ${files[$i]}_$g.txt
    #    ./PMC ../data/bun_zipper_res4.pts loop.obj -b loop -g $g >> ${files[$i]}_$g.txt
    #    ./PMC ../data/dragon_vrip_res1.pts loop.obj -b loop -g $g >> ${files[$i]}_$g.txt
    #    ./PMC ../data/dragon_vrip_res2.pts loop.obj -b loop -g $g >> ${files[$i]}_$g.txt
    #    ./PMC ../data/dragon_vrip_res3.pts loop.obj -b loop -g $g >> ${files[$i]}_$g.txt
    #    ./PMC ../data/dragon_vrip_res4.pts loop.obj -b loop -g $g >> ${files[$i]}_$g.txt
    #done

    g=64
    echo "Schedule: $def" > ${files[$i]}_$g.txt

    for j in {1..25}; do
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
