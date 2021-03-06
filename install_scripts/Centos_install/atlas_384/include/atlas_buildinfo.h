#ifndef ATL_INSTINFO_H
   #define ATL_INSTINFO_H

#define ATL_ARCH "HAMMER64SSE2"
#define ATL_INSTFLAGS "-1 0 -a 1"
#define ATL_F2CDEFS "-DAdd_ -DF77_INTEGER=int -DStringSunStyle"
#define ATL_ARCHDEFS "-DATL_OS_Linux -DATL_ARCH_PII -DATL_CPUMHZ=2261 -DATL_SSE2 -DATL_SSE1 -DATL_USE64BITS -DATL_GAS_x8664"
#define ATL_DKCFLAGS "-fomit-frame-pointer -mfpmath=387 -O2 -falign-loops=4 -g -Wa,--noexecstack -fPIC -m64"
#define ATL_DKC "gcc"
#define ATL_SKCFLAGS "-fomit-frame-pointer -mfpmath=387 -O2 -falign-loops=4 -g -Wa,--noexecstack -fPIC -m64"
#define ATL_SKC "gcc"
#define ATL_DMCFLAGS "-fomit-frame-pointer -mfpmath=387 -O2 -falign-loops=4 -g -Wa,--noexecstack -fPIC -m64"
#define ATL_DMC "gcc"
#define ATL_SMCFLAGS "-fomit-frame-pointer -mfpmath=387 -O2 -falign-loops=4 -g -Wa,--noexecstack -fPIC -m64"
#define ATL_SMC "gcc"
#define ATL_ICCFLAGS "-DL2SIZE=4194304 -I/builddir/build/BUILD/ATLAS/x86_64_base/include -I/builddir/build/BUILD/ATLAS/x86_64_base/..//include -I/builddir/build/BUILD/ATLAS/x86_64_base/..//include/contrib -DAdd_ -DF77_INTEGER=int -DStringSunStyle -DATL_OS_Linux -DATL_ARCH_PII -DATL_CPUMHZ=2261 -DATL_SSE2 -DATL_SSE1 -DATL_USE64BITS -DATL_GAS_x8664 -DWALL -DATL_NCPU=4 -fomit-frame-pointer -mfpmath=387 -O2 -falign-loops=4 -g -Wa,--noexecstack -fPIC -m64"
#define ATL_ICC "gcc"
#define ATL_F77FLAGS "-fomit-frame-pointer -mfpmath=387 -O2 -falign-loops=4 -g -Wa,--noexecstack -fPIC -m64"
#define ATL_F77 "gfortran"
#define ATL_DKCVERS "gcc (GCC) 4.4.6 20110731 (Red Hat 4.4.6-3)"
#define ATL_SKCVERS "gcc (GCC) 4.4.6 20110731 (Red Hat 4.4.6-3)"
#define ATL_DMCVERS "gcc (GCC) 4.4.6 20110731 (Red Hat 4.4.6-3)"
#define ATL_SMCVERS "gcc (GCC) 4.4.6 20110731 (Red Hat 4.4.6-3)"
#define ATL_ICCVERS "gcc (GCC) 4.4.6 20110731 (Red Hat 4.4.6-3)"
#define ATL_F77VERS "GNU Fortran (GCC) 4.4.6 20110731 (Red Hat 4.4.6-3)"
#define ATL_SYSINFO "Linux c6b6.bsys.dev.centos.org 2.6.32-44.2.el6.x86_64 #1 SMP Wed Jul 21 12:48:32 EDT 2010 x86_64 x86_64 x86_64 GNU/Linux"
#define ATL_DATE    "Wed Mar 21 01:43:44 GMT 2012"
#define ATL_UNAM    "mockbuild"
#define ATL_VERS    "3.8.4"

#endif
