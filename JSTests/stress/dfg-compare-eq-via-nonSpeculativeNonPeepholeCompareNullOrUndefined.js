//@ runDefault("--collectContinuously=true", "--collectContinuouslyPeriodMS=0.15", "--useMaximalFlushInsertionPhase=true", "--useLLInt=false", "--useFTLJIT=false", "--jitPolicyScale=0")

// This test exercises DFG::SpeculativeJIT::nonSpeculativeNonPeepholeCompareNullOrUndefined().

for (let i = 0; i < 25; i++)
    'a'.match(/a/);

