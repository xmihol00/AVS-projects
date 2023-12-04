#!/bin/bash

sources=(
    ../data/bun_zipper_res4.pts
    ../data/dragon_vrip_res4.pts
    ../data/bun_zipper_res3.pts
    ../data/dragon_vrip_res3.pts
    ../data/bun_zipper_res2.pts
    ../data/dragon_vrip_res2.pts
    ../data/bun_zipper_res1.pts
    ../data/dragon_vrip_res1.pts
)

grids=(
    8
    16
    32
    64
    128
    256
    512
)

builders=(
    loop 
    tree
)

valid=1
echo "" > test_results.log
echo "" > log_test.log

for grid in "${grids[@]}"; do
    echo "grid $grid:" >> test_results.log
    
    make clean
    make -j

    for source in "${sources[@]}"; do
        ./PMC --builder ref --grid $grid $source ref.obj >>log_test.log
        
        for builder in "${builders[@]}"; do
            ./PMC --builder $builder --grid $grid $source $builder.obj 2>>log_test.log 1>>log_test.log

            echo "$source  $builder  $grid  $def: " >> test_results.log
            echo -en "\e[36m" >> test_results.log
            if python3 ../scripts/check_output.py $builder.obj ref.obj >> test_results.log ; then
                echo -n "" >> test_results.log
            else
                echo -e "\e[31mInvalid output for builder $builder with defs $def and grid $grid" >> test_results.log
                valid=0
            fi
            echo -en "\e[0m" >> test_results.log
        done
    done

    echo -n "Conclusion - grid $grid: " >> test_results.log
    if [ "$valid" -eq 1 ] ; then
        echo -e "\e[32mSuccess $valid \e[0m" >> test_results.log
    else
        echo -e "\e[31mOne or more tests failed\e[0m" >> test_results.log
        valid=1
    fi

    echo "" >> test_results.log
    echo "" >> log_test.log
done

