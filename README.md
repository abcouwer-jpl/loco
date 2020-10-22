# loco

An implementation of the LOCO-I (LOw COmplexity LOssless COmpression for Images) 
algorithm geared towards flight-safety and portability

Developer/maintainer: Neil Abcouwer, neil.abcouwer@jpl.nasa.gov

This software is based on the LOCO-I algorithm which is described in:

  Marcelo J. Weinberger, Gadiel Seroussi and Guillermo Sapiro,
    "LOCO-I: A Low Complexity, Context-Based, Lossless Image
    Compression Algorithm", in Proc. Data Compression Conference
    (DCC '96), pp. 140-149, 1996.

This implementation originates with Matt Klimesh and Aaron Kiely's
implementation for MER, and was modified by Neil Abcouwer. 
Modifications include using fixed-width types, putting code common 
to compression and decompression in a loco_common file, 
removal of global variables in favor of passed struct pointers, 
reduction of function parameters via use of structs, 
and other modifications per MISRA/JPL/P10 guidelines.

This implementation lacks the run length encoding ("embedded
alphabet extension") of LOCO-I.  There are other differences as well.

## Using the code

`include/loco/loco_pub.h` defines public functions for compression and decompression.

`include/loco/loco_types_pub.h` defines macros for buffer sizing 
at compile time and various public types.

loco expects a configuration dependent `include/loco/loco_conf_global_types.h`
and `include/loco/loco_conf_private.h` that one must create for your 
specific configuration. `loco_conf_global_types.h` defines sized types, and 
`loco_conf_global_types.h` defines macros. 
`test/loco_test_global_types.h` and `test/loco_test_private.h` are examples 
that will be copied over to `include/loco` for unit testing.

## building

This code does not by itself compile into an executable or library. 
This code is intended to be compiled along with the code that uses it.

For testing, you'll need the following dependencies:

`build-essential cmake gcc valgrind lcov`

To run unit tests (including pulling google test framework): 

`./build.bash test`

To run unit tests with coverage reports:

`./build.bash coverage`

Then open ./build/coverage/index.html to look at results.

To save unit test output to the test folder (so it can be committed 
for later delta comparison)

`  ./build.bash save`

To run unit tests with valgrind 
(note, this disables death tests, as they don't play well with valgrind):

`./build.bash valgrind`

To clean (remove the build directory):

`./build.bash clean`

To use with your own framework, you will need to define your own versions of
`loco_conf_global_types.h` and `loco_conf_private.h`, 
to do appropriate declarations for your framework, and you may need to make 
build changes so they aren't overwritten.


### configuration, types, and macros

This library was written with the intention of being usable for safety-critical 
applications (like spaceflight) and to be flexible enough to be used in 
multiple frameworks. This leads to conflict. For instance, 
safety critical code neccesitates the use of fixed-width types, 
but flexibility means that such types may vary. If not already available, 
users must write a `loco_conf_global_types.h` to define certain types and a 
`loco_conf_private.h` to define certain macros. 
`test/loco_test_global_types.h` and `test/loco_test_private.h` 
provide more explanation and examples, and are copied when running unit tests.

One example is using NASA Core FSW's `common_types.h` as the basis 
for sized types. Running

`./build.bash cfstest`

copies `configs/cfs/loco_cfs_global_types.h`, which includes 
`common_types.h` (copied to the test folder for convenience) to 
`loco_conf_global_types.h`, then builds and runs the unit tests. 
But it is unclear (to Neil) whether Core FSW has assertions (see below) 
so as of yet there is no `loco_cfs_private.h`.

## assertions

This library was written with the philosophy that inproper inputs to functions, 
such as an improperly sized image, are programming errors, and that assertions 
be used to check programming errors. See http://spinroot.com/p10/rule5.html 
for more on asserts.

Functions use `LOCO_ASSERT` macros. It is the intent that users of the 
loco library will copy an appropriate `loco_conf_private.h` to include/loco, 
such that asserts are defined appropriately for the application.
They might be defined as `ROS_ASSERT`, `BOOST_ASSERT`, 
c `assert`, or send some asycnhronous message to do whatever
the system needs during an anomaly. 

They could also be disabled, but this is is discouraged.

## static analysis

This library has been analyzed using Cobra (http://spinroot.com/cobra/, 
https://github.com/nimble-code/Cobra). 

To use Cobra, follow the Cobra instructions to clone and configure. 
On linux, you may need the "yacc" program from the "bison" package.
You may want to add these lines in your bashrc (or equivalent) 
as discussed in the cobra readme:

`export COBRA=/path/to/your/clone/of/Cobra`

`export PATH=$PATH:$COBRA/bin_linux`

`export C_BASE=$COBRA/rules`

Then run commands of the form:

`cobra -f file -I/path/to/this/repo/include /path/to/this/repo/src/*.c`

Where file can be one of several rules files. This code, compiled with the 
unit test headers, was checked against the basic, misra2012, p10, and jpl rules. 
Running 

`./build.bash cobra`

runs all these checks.

Deviations from the rules:
- The main encoding/decoding loops loop over every pixel of an image or 
the entirety of the compressed bitstream. Thus the overhead of function calls 
becomes significant. Therefore these main loops exceed advised function sizes 
and use macros for the sake of performance.
- P10: Locally-defined macro functions are used to improve performance.
- JPL: Using "ifdef __cplusplus" mangle guard puts code over the preprocessor 
conditional limit of one per header, but that's the price you pay for c++ compatibility.

## contributions

Pull requests or emails about small code fixes, new code features, 
configurations for targeted frameworks, or improvements to the build process 
are welcome. Please ensure any pull requests are fully covered by unit testing, 
follow the established standards, and pass static analysis.

## todo

- Run semmle and codesonar on code
- Add more pictures to test against during unit testing
- Optimize decompression code (which did not run on-board for MER and inheritors)
- Support images stored in 8-bit pixels

## JPL Development Info  

This software was developed at the Jet Propulsion Laboratory, 
California Institute of Technology, under a contract with the 
National Aeronautics and Space Administration (80NM0018D0004).  

This work was funded by funded by the NASA Space Technology 
Mission Directorate (STMD) and the proposal, 
"CubeRover for Affordable, Modular, and Scalable Planetary Exploration" 
was selected as part of the NASA 2019 Tipping Point solicitation Topic 4: 
Other Capabilities Needed for Exploration. The project is managed by the 
STMD Utilizing Public-Private Partnerships to Advance Tipping Point 
Technologies Program.

This software was developed under JPL Task Plan No 15-106860.

This software is reported via JPL NTR 51705.

This software was approved for open source by JPL Software Release Authority 
Brian Morrison on 2020-10-21.
