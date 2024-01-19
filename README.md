# AVS-projects
Vectorization and parallelization projects to Computation Systems Architectures course at FIT BUT using OpenMP and `__m512` instructions.

## Mandelbrot Set
The 1st task was to optimize the computation of a mandelbrot set on a 2:3 plane. The optimization using `__m512` instructions was the fastest solution of the whole course. Overall the solution scored 10/10 points plus a bonus point for an especially thoughtful implementation.

## Marching Cubes
The second task was to optimize and analyze an algorithm for reconstructing a polygonal surface from a scalar field in 3D space (so-called "Marching Cubes"). The performance of this task was not measured. The solution scored 20/20 points plus a bonus point for noticing possible SIMD optimization and for using private memory for each thread.
