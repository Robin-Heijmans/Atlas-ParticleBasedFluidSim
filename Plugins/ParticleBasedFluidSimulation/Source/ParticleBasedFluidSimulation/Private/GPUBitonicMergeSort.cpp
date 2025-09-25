#include "GPUBitonicMergeSort.h"
#include "RenderGraphUtils.h"


// OUOU STINKYYYYYY !!!!!!!!!!!1111!!!!!!!1111111!111!!!1!elf!!!!
/*
namespace Shaders {
    IMPLEMENT_GLOBAL_SHADER(FBitonicSortShader, "/Shaders/Compute/BitonicMergeSort.usf", "Sort", SF_Compute);
}

FGPUBitonicSearchSort::FGPUBitonicSearchSort() {

}

void FGPUBitonicSearchSort::Sort(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, Shaders::FBitonicSortShader::FParameters* PassParameters) {
    // Launch each step of the sorting algorithm (once the previous step is complete)
    // Number of steps = [log2(n) * (log2(n) + 1)] / 2
    // where n = nearest power of 2 that is greater or equal to the number of inputs
    uint32 bufferCount = PassParameters->NumEntries;
    int numStages = static_cast<int>(FMath::Log2(static_cast<float>(FMath::RoundUpToPowerOfTwo(bufferCount))));

    const FIntVector DispatchCount(16,1,1);
    // Sort kernel
    TShaderMapRef<Shaders::FBitonicSortShader> ComputeShader(GlobalShaderMap);
    check(ComputeShader.IsValid());

    for (int stageIndex = 0; stageIndex < numStages; stageIndex++)
    {
        for (int stepIndex = 0; stepIndex < stageIndex + 1; stepIndex++)
        {
            // Calculate some pattern stuff
            int groupWidth = 1 << (stageIndex - stepIndex);
            int groupHeight = 2 * groupWidth - 1;
            PassParameters->GroupWidth = groupWidth;
            PassParameters->GroupHeight = groupHeight;
            PassParameters->StepIndex = stepIndex;
            // Run the sorting step on the GPU
            FComputeShaderUtils::AddPass(
                GraphBuilder,
                RDG_EVENT_NAME("Sort spatial lookup table"), 
                ComputeShader,
                PassParameters,
                DispatchCount);
            //ComputeHelper.Dispatch(sortCompute, FMath::RoundUpToPowerOfTwo(indexBuffer.count) / 2);
        }
    }
}

void FGPUBitonicSearchSort::SortAndCalculateOffsets(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, Shaders::FBitonicSortShader::FParameters* PassParameters) {
    Sort(GraphBuilder, GlobalShaderMap, PassParameters);

    const FIntVector DispatchCount(16,1,1);
    // CalculateOffsets kernel
    TShaderMapRef<Shaders::FBitonicSortShader> ComputeShader(GlobalShaderMap);
    check(ComputeShader.IsValid());

    FComputeShaderUtils::AddPass(
        GraphBuilder,
        RDG_EVENT_NAME("Sort spatial lookup table"), 
        ComputeShader,
        PassParameters,
        DispatchCount);
    
}
        */