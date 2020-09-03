#!/usr/bin/bash
SRC_DIR=../
SYSTEM=stampede2

module load gcc
module load cmake

for cfg in dense_nmf dense_ntf dense_distnmf dense_distntf;
do    
    mkdir ../build_$SYSTEM\_$cfg
done
#dense builds
for cfg in distntf; # nmf ntf distnmf distntf;
do
    echo $cfg
    echo build_$SYSTEM\_dense_$cfg
    pushd ../build_$SYSTEM\_dense_$cfg
    CC=gcc CXX=g++ cmake $SRC_DIR/$cfg/ -DCMAKE_BUILD_CUDA=0
    make
    popd
done

#copy all the executable
# cp ../build_$SYSTEM\_dense_nmf/dense_nmf .
cp ../build_$SYSTEM\_dense_ntf/dense_ntf .
# cp ../build_$SYSTEM\_dense_distnmf/dense_distnmf .
# cp ../build_$SYSTEM\_dense_distntf/dense_distntf .
