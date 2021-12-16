#include <asm/cpufeature.h>

const char * const x86_cap_flags[NCAPINTS*32] = {
	[X86_FEATURE_FPU]                = "fpu",
	[X86_FEATURE_VME]                = "vme",
	[X86_FEATURE_DE]                 = "de",
	[X86_FEATURE_PSE]                = "pse",
	[X86_FEATURE_TSC]                = "tsc",
	[X86_FEATURE_MSR]                = "msr",
	[X86_FEATURE_PAE]                = "pae",
	[X86_FEATURE_MCE]                = "mce",
	[X86_FEATURE_CX8]                = "cx8",
	[X86_FEATURE_APIC]               = "apic",
	[X86_FEATURE_SEP]                = "sep",
	[X86_FEATURE_MTRR]               = "mtrr",
	[X86_FEATURE_PGE]                = "pge",
	[X86_FEATURE_MCA]                = "mca",
	[X86_FEATURE_CMOV]               = "cmov",
	[X86_FEATURE_PAT]                = "pat",
	[X86_FEATURE_PSE36]              = "pse36",
	[X86_FEATURE_PN]                 = "pn",
	[X86_FEATURE_CLFLSH]             = "clflush",
	[X86_FEATURE_DS]                 = "dts",
	[X86_FEATURE_ACPI]               = "acpi",
	[X86_FEATURE_MMX]                = "mmx",
	[X86_FEATURE_FXSR]               = "fxsr",
	[X86_FEATURE_XMM]                = "sse",
	[X86_FEATURE_XMM2]               = "sse2",
	[X86_FEATURE_SELFSNOOP]          = "ss",
	[X86_FEATURE_HT]                 = "ht",
	[X86_FEATURE_ACC]                = "tm",
	[X86_FEATURE_IA64]               = "ia64",
	[X86_FEATURE_PBE]                = "pbe",
	[X86_FEATURE_SYSCALL]            = "syscall",
	[X86_FEATURE_MP]                 = "mp",
	[X86_FEATURE_NX]                 = "nx",
	[X86_FEATURE_MMXEXT]             = "mmxext",
	[X86_FEATURE_FXSR_OPT]           = "fxsr_opt",
	[X86_FEATURE_GBPAGES]            = "pdpe1gb",
	[X86_FEATURE_RDTSCP]             = "rdtscp",
	[X86_FEATURE_LM]                 = "lm",
	[X86_FEATURE_3DNOWEXT]           = "3dnowext",
	[X86_FEATURE_3DNOW]              = "3dnow",
	[X86_FEATURE_RECOVERY]           = "recovery",
	[X86_FEATURE_LONGRUN]            = "longrun",
	[X86_FEATURE_LRTI]               = "lrti",
	[X86_FEATURE_CXMMX]              = "cxmmx",
	[X86_FEATURE_K6_MTRR]            = "k6_mtrr",
	[X86_FEATURE_CYRIX_ARR]          = "cyrix_arr",
	[X86_FEATURE_CENTAUR_MCR]        = "centaur_mcr",
	[X86_FEATURE_CONSTANT_TSC]       = "constant_tsc",
	[X86_FEATURE_UP]                 = "up",
	[X86_FEATURE_ARCH_PERFMON]       = "arch_perfmon",
	[X86_FEATURE_PEBS]               = "pebs",
	[X86_FEATURE_BTS]                = "bts",
	[X86_FEATURE_REP_GOOD]           = "rep_good",
	[X86_FEATURE_NOPL]               = "nopl",
	[X86_FEATURE_XTOPOLOGY]          = "xtopology",
	[X86_FEATURE_TSC_RELIABLE]       = "tsc_reliable",
	[X86_FEATURE_NONSTOP_TSC]        = "nonstop_tsc",
	[X86_FEATURE_EXTD_APICID]        = "extd_apicid",
	[X86_FEATURE_AMD_DCM]            = "amd_dcm",
	[X86_FEATURE_APERFMPERF]         = "aperfmperf",
	[X86_FEATURE_XMM3]               = "pni",
	[X86_FEATURE_PCLMULQDQ]          = "pclmulqdq",
	[X86_FEATURE_DTES64]             = "dtes64",
	[X86_FEATURE_MWAIT]              = "monitor",
	[X86_FEATURE_DSCPL]              = "ds_cpl",
	[X86_FEATURE_VMX]                = "vmx",
	[X86_FEATURE_SMX]                = "smx",
	[X86_FEATURE_EST]                = "est",
	[X86_FEATURE_TM2]                = "tm2",
	[X86_FEATURE_SSSE3]              = "ssse3",
	[X86_FEATURE_CID]                = "cid",
	[X86_FEATURE_FMA]                = "fma",
	[X86_FEATURE_CX16]               = "cx16",
	[X86_FEATURE_XTPR]               = "xtpr",
	[X86_FEATURE_PDCM]               = "pdcm",
	[X86_FEATURE_DCA]                = "dca",
	[X86_FEATURE_XMM4_1]             = "sse4_1",
	[X86_FEATURE_XMM4_2]             = "sse4_2",
	[X86_FEATURE_X2APIC]             = "x2apic",
	[X86_FEATURE_MOVBE]              = "movbe",
	[X86_FEATURE_POPCNT]             = "popcnt",
	[X86_FEATURE_AES]                = "aes",
	[X86_FEATURE_XSAVE]              = "xsave",
	[X86_FEATURE_AVX]                = "avx",
	[X86_FEATURE_F16C]               = "f16c",
	[X86_FEATURE_RDRND]              = "rdrnd",
	[X86_FEATURE_HYPERVISOR]         = "hypervisor",
	[X86_FEATURE_XSTORE]             = "rng",
	[X86_FEATURE_XSTORE_EN]          = "rng_en",
	[X86_FEATURE_XCRYPT]             = "ace",
	[X86_FEATURE_XCRYPT_EN]          = "ace_en",
	[X86_FEATURE_ACE2]               = "ace2",
	[X86_FEATURE_ACE2_EN]            = "ace2_en",
	[X86_FEATURE_PHE]                = "phe",
	[X86_FEATURE_PHE_EN]             = "phe_en",
	[X86_FEATURE_PMM]                = "pmm",
	[X86_FEATURE_PMM_EN]             = "pmm_en",
	[X86_FEATURE_LAHF_LM]            = "lahf_lm",
	[X86_FEATURE_CMP_LEGACY]         = "cmp_legacy",
	[X86_FEATURE_SVM]                = "svm",
	[X86_FEATURE_EXTAPIC]            = "extapic",
	[X86_FEATURE_CR8_LEGACY]         = "cr8_legacy",
	[X86_FEATURE_ABM]                = "abm",
	[X86_FEATURE_SSE4A]              = "sse4a",
	[X86_FEATURE_MISALIGNSSE]        = "misalignsse",
	[X86_FEATURE_3DNOWPREFETCH]      = "3dnowprefetch",
	[X86_FEATURE_OSVW]               = "osvw",
	[X86_FEATURE_IBS]                = "ibs",
	[X86_FEATURE_XOP]                = "xop",
	[X86_FEATURE_SKINIT]             = "skinit",
	[X86_FEATURE_WDT]                = "wdt",
	[X86_FEATURE_NODEID_MSR]         = "nodeid_msr",
	[X86_FEATURE_IDA]                = "ida",
	[X86_FEATURE_ARAT]               = "arat",
	[X86_FEATURE_CPB]                = "cpb",
	[X86_FEATURE_EPB]                = "epb",
	[X86_FEATURE_XSAVEOPT]           = "xsaveopt",
	[X86_FEATURE_PLN]                = "pln",
	[X86_FEATURE_PTS]                = "pts",
	[X86_FEATURE_DTS]                = "dts",
	[X86_FEATURE_TPR_SHADOW]         = "tpr_shadow",
	[X86_FEATURE_VNMI]               = "vnmi",
	[X86_FEATURE_FLEXPRIORITY]       = "flexpriority",
	[X86_FEATURE_EPT]                = "ept",
	[X86_FEATURE_VPID]               = "vpid",
	[X86_FEATURE_NPT]                = "npt",
	[X86_FEATURE_LBRV]               = "lbrv",
	[X86_FEATURE_SVML]               = "svm_lock",
	[X86_FEATURE_NRIPS]              = "nrip_save",
	[X86_FEATURE_FSGSBASE]           = "fsgsbase",
};
