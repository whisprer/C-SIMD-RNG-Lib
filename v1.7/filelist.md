Folder PATH listing for volume X:/

Volume serial number is X0XX-X00X

X:.

|   CMakeLists.cpl							# CMaleLists main config file for compilation of code during benchmarking

|   CMakeLists.old							# CMaleLists main config file for compilation of code during dev

|   CMakeLists.txt								# CMaleLists main config file for those wishing to compile the library themselves

|   filelist.md									# Me.

|   main.cpp									# The main.cpp that runs this entire project code

|   necesary\_deps\_by\_OS.md					# A very useful document identifying which lib files are necessary by OS

|   README.md								# Yu got a Q.? or stuck? or need quick-start>? or \_anything\_ else? this.

|

+---bench

|       bench\_main.cpp          					# Benchmarking main.cpp codebase component

|

+---cmake

|	    ua\_rngConfig.cmake.in					# key config file for final compile of libraries

|

+---docs

|      g-petey\_convo.md						# The entire conversation betwixt myself and G-Petey during the construction of this lib.

|	CHANGELOG								# A Documentation of the change history if the project

|	CONTRIBUTING							# documentation of the guidelines for contributing to the project

|	SECURITY								# the overall security situation of the codebase and issue surrounding

|	LICENCE									# the licencing law and guidelines governing use if this project

|	INSTALLATION							# an installation guide for the libraries

|	**Scrambled-Linear-PRNGs-D.Blackman\_and\_S.Vigna.pdf**

|	for\_further\_dev.md      					# a look toward plans for future development of this code

|	Development\_Roadmap.md				# a drawn out roadmap across time for the further development of the project

|

+---Linux									# Linux directory of the library filles for a Linux OS

|      \\

|       |   LICENCE								# LICENCE copy for Linux users

|       |   README								# README.md for Linux users

|       |

|       +---bin									# bin directory holds the .dlls

|       |       libua\_rng.dll						# a key component of the Linux library

|       |       libua\_rng.dll.a						# a key component of the Linux library

|       |

|       \\----lib									# lib directory holds the .lib files

|       |            ua\_rng-1.7						# a key component of the Linux library

|       |            libua\_rng.a						# a key component of the Linux library

|       |

|       +---cmake								# a directory to hold cmake stuff in building the library

|       |    \\---pkconfig							# a .config file used in the build of the library

|       |

|       |

|       +---include								# the headers directory for the c++ .h files for the Linux version of the library

|           \\---ua

|                  ua\_cpuid.h

|                  ua\_features.h

|                  ua\_normal\_avx2.h

|                  ua\_normal\_polar.h

|                  ua\_normal\_ziggurat.h

|                  ua\_onefile.cpp

|                  ua\_philox4x32\_avx2.h

|                  ua\_platform.h

|                  ua\_rng.h

|                  ua\_xoroshiro128pp.h

|                  ua\_xoroshiro256ss\_avx2.h

|                  ua\_xoroshiro256ss\_scalar.h

|                  ua\_xoshiro256ss\_avx2.h

|                  ua\_xoshiro256ss\_avx512.h

|                  ua\_xoshiro256ss\_scalar.h

|

+---macOS									# the directory to hold the macOS version of the library

|       LICENCE

|       README

|

+---win\_msys2								# the WinMSYS2 version of the library

         LICENCE

|       README

|

+---src										# the directory for the .cpp codebase for the library

|      ua\_cpuid.cpp

|      ua\_rng.cpp

|      xoroshiro256ss\_avx2.cpp

|      xoroshiro256ss\_avx512.cpp

|      xoroshiro256s\_scalar.cpp

|      xoroshiro\_scalar.cpp

|      xoshiro256ss\_avx2.cpp

|      xoshiro256ss\_avx512-m512d.cpp

|      xoshiro256ss\_avx512.cpp

|

`+---tests									# a directory to contain files used in testing during dev

|       msvc\_test.lll

|

+---tools									# scripts used during dev

|       build\_and\_install\_very\_verbose.sh

|       build\_msvc.ps1

|       lib\_build.sh

|       lib\_clean.sh

|       lib\_scipt.sh

|       stage\_dist.sh

|       ua\_purge\_cmake.ps1

|       ua\_purge\_cmake.sh

|

\\---win\_msvc								# directory for WinMSVC version of library

|     LICENCE

|     README

 \\

 +---include									# the directory containing the .h header files for the codebase of the library

  \\---ua

        ua\_cpuid.h

        ua\_features.h

        ua\_normal\_avx2.h

        ua\_normal\_polar.h

        ua\_normal\_ziggurat.h

        ua\_onefile.cpp

        ua\_philox4x32\_avx2.h

        ua\_platform.h

        ua\_rng.h

        ua\_xoroshiro128pp.h

        ua\_xoroshiro256ss\_avx2.h

        ua\_xoroshiro256ss\_scalar.h

        ua\_xoshiro256ss\_avx2.h

        ua\_xoshiro256ss\_avx512.h

        ua\_xoshiro256ss\_scalar.h

