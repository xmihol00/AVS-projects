
make clean
make -j

g=64
echo "" > loop_perf_$g.txt

for j in {1..10}; do
    ./PMC ../data/bun_zipper_res1.pts loop.obj -b loop -g $g >> loop_perf_$g.txt
    ./PMC ../data/bun_zipper_res2.pts loop.obj -b loop -g $g >> loop_perf_$g.txt
    ./PMC ../data/bun_zipper_res3.pts loop.obj -b loop -g $g >> loop_perf_$g.txt
    ./PMC ../data/bun_zipper_res4.pts loop.obj -b loop -g $g >> loop_perf_$g.txt
    ./PMC ../data/dragon_vrip_res1.pts loop.obj -b loop -g $g >> loop_perf_$g.txt
    ./PMC ../data/dragon_vrip_res2.pts loop.obj -b loop -g $g >> loop_perf_$g.txt
    ./PMC ../data/dragon_vrip_res3.pts loop.obj -b loop -g $g >> loop_perf_$g.txt
    ./PMC ../data/dragon_vrip_res4.pts loop.obj -b loop -g $g >> loop_perf_$g.txt
done

