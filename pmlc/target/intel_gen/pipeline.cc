// Copyright 2019, Intel Corporation

#include "mlir/Conversion/GPUToSPIRV/ConvertGPUToSPIRVPass.h"
#include "mlir/Conversion/LoopToStandard/ConvertLoopToStandard.h"
#include "mlir/Conversion/LoopsToGPU/LoopsToGPUPass.h"
#include "mlir/Conversion/StandardToLLVM/ConvertStandardToLLVMPass.h"
#include "mlir/Dialect/GPU/Passes.h"
#include "mlir/Dialect/SPIRV/SPIRVOps.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Transforms/Passes.h"

#include "pmlc/compiler/registry.h"
#include "pmlc/conversion/pxa_to_affine/pxa_to_affine.h"
#include "pmlc/conversion/tile_to_pxa/tile_to_pxa.h"
#include "pmlc/dialect/tile/transforms/passes.h"

using namespace mlir; // NOLINT[build/namespaces]
using pmlc::conversion::pxa_to_affine::createLowerPXAToAffinePass;

namespace pmlc::target::intel_gen {

namespace {

void addToPipeline(OpPassManager &pm) {
  pm.addPass(pmlc::dialect::tile::createComputeBoundsPass());
  pm.addNestedPass<FuncOp>(createCanonicalizerPass());
  pm.addNestedPass<FuncOp>(createCSEPass());

  pm.addPass(pmlc::conversion::tile_to_pxa::createLowerTileToPXAPass());
  pm.addNestedPass<FuncOp>(createCanonicalizerPass());
  pm.addNestedPass<FuncOp>(createCSEPass());

  // TODO: do optimizations here

  pm.addPass(createLowerPXAToAffinePass());
  pm.addNestedPass<FuncOp>(createCanonicalizerPass());
  pm.addNestedPass<FuncOp>(createCSEPass());

  pm.addPass(createLowerAffinePass());
  pm.addNestedPass<FuncOp>(createCanonicalizerPass());
  pm.addNestedPass<FuncOp>(createCSEPass());

  pm.addPass(createSimpleLoopsToGPUPass(1, 1));
  pm.addNestedPass<FuncOp>(createCanonicalizerPass());
  pm.addNestedPass<FuncOp>(createCSEPass());

  pm.addPass(createGpuKernelOutliningPass());
  // NOTE: canonicalizer/cse at this stage causes later passes to fail

  pm.addNestedPass<ModuleOp>(createConvertGPUToSPIRVPass());
  pm.addPass(createCanonicalizerPass());
  pm.addPass(createCSEPass());

  // pm.addPass(createLowerToLLVMPass(true));
}

static PassPipelineRegistration<>
    passPipelineReg("target-intel_gen", "Target pipeline for Intel GEN iGPUs",
                    addToPipeline);
static compiler::TargetRegistration targetReg("intel_gen", addToPipeline);

} // namespace

} // namespace pmlc::target::intel_gen
