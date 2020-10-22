#!/bin/bash -eux

source_path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd "$source_path"
mkdir -p build
cd build
unset LOCO_DISABLE_DEATH_TESTS

if [ "$#" -ne 1 ] ; then
  echo "Building loco Test configuration"
  cmake -DCMAKE_BUILD_TYPE=Test ..
  make
else 
  shopt -s nocasematch
  if [[ "$1" = "coverage" ]] ; then
    echo "Building loco Coverage configuration"
    cmake -DCMAKE_BUILD_TYPE=Coverage ..
    make
    make coverage
  elif [[ "$1" = "valgrind" ]] ; then
    echo "Running Valgrind on loco"
    export LOCO_DISABLE_DEATH_TESTS=1
    cmake -DCMAKE_BUILD_TYPE=Test ..
    make
    valgrind --leak-check=full -v ./loco_gtest
  elif [[ "$1" = "cfstest" ]] ; then
    echo "Configuring for cFS and running loco tests"
    cmake -DCMAKE_BUILD_TYPE=CfsTest ..
    make
    make test ARGS="-V"
  elif [[ "$1" = "test" ]] ; then
    echo "Running loco tests"
    cmake -DCMAKE_BUILD_TYPE=Test ..
    make
    make test ARGS="-V"
  elif [[ "$1" = "cobra" ]] ; then
    echo "Running cobra tests (assumes cobra is configured)"
  	cobra -f basic -I$source_path/include $source_path/src/*.c
  	cobra -f misra2012 -I$source_path/include $source_path/src/*.c
  	cobra -f p10 -I$source_path/include $source_path/src/*.c
  	cobra -f jpl -I$source_path/include $source_path/src/*.c
  elif [[ "$1" = "cobra-basic" ]] ; then
    echo "Running basic cobra tests (assumes cobra is configured)"
    cobra -f basic -I$source_path/include -I$source_path/build $source_path/src/*.c
  elif [[ "$1" = "cobra-misra" ]] ; then
    echo "Running misra cobra tests (assumes cobra is configured)"
    cobra -f misra2012 -I$source_path/include -I$source_path/build $source_path/src/*.c
  elif [[ "$1" = "cobra-p10" ]] ; then
    echo "Running p10 cobra tests (assumes cobra is configured)"
    cobra -f p10 -I$source_path/include -I$source_path/build $source_path/src/*.c  
  elif [[ "$1" = "cobra-jpl" ]] ; then
    echo "Running jpl cobra tests (assumes cobra is configured)"
    cobra -f jpl -I$source_path/include -I$source_path/build $source_path/src/*.c
  elif [[ "$1" = "save" ]] ; then
    echo "Saving test output"
    if [[ -e ./Testing/Temporary/LastTest.log ]] ; then
      cp ./Testing/Temporary/LastTest.log $source_path/test/output/Test.log
    else 
  	  echo "No test results found"
    fi
  	if [[ -e ./coverage ]] ; then
  	  cp -r coverage/* $source_path/test/output/coverage/
    else
	  echo "No coverage results found"
    fi
  elif [[ "$1" = "clean" ]] ; then
    echo "Cleaning loco build"
    cd "$source_path"
    rm -rf build
  else
    echo "Unrecognized argument $1"
  fi
fi

cd "$source_path"
