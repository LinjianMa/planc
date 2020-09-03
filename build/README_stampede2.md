## Instructions for getting PLANC working on Stampede2

### Modules
```
module load gcc
module load cmake
```

### Requirements

- Install MKL, and source the MKL include folder. For example:
```
source /opt/intel/compilers_and_libraries_2017.4.196/linux/mkl/bin/mklvars.sh intel64
```
- Install ARMADILLO, and add it to the library path. For example:
```
export ARMADILLO_INCLUDE_DIR=/work/03940/lma16/stampede2/armadillo-9.900.2/include/
export INCLUDE=$INCLUDE:$ARMADILLO_INCLUDE_DIR:
```

### Compilation

Run
```
./build-stampede2.sh
```

### A simple example for running distributed CP-ALS

```
ibrun -n 8 ./dense_distntf -a 6 -p "2 2 2" -s 1 -t 10 -k 5 -i rand_uniform -d "10 10 10" -e 1
```