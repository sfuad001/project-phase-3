#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

// This method implements what the pass does
struct ValueNumberingPass : public PassInfoMixin<ValueNumberingPass> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    // Here goes what you want to do with the pass
    std::string func_name = "main";
    errs() << "ValueNumbering: " << F.getName() << "\n";

    // Comment this line
    if (F.getName() != func_name)
      return PreservedAnalyses::all();

    /*
    for (auto &basic_block : F) {
      for (auto &inst : basic_block) {
        errs() << inst << "\n";
        if (inst.getOpcode() == Instruction::Load) {
          errs() << "This is Load" << "\n";
        }
        if (inst.getOpcode() == Instruction::Store) {
          errs() << "This is Store" << "\n";
        }
        if (inst.isBinaryOp()) {
          errs() << "Op Code:" << inst.getOpcodeName() << "\n";
          if (inst.getOpcode() == Instruction::Add) {
            errs() << "This is Addition" << "\n";
          }
          if (inst.getOpcode() == Instruction::Load) {
            errs() << "This is Load" << "\n";
          }
          if (inst.getOpcode() == Instruction::Mul) {
            errs() << "This is Multiplication" << "\n";
          }

          // see other classes, Instruction::Sub, Instruction::UDiv, Instruction::SDiv
          // errs() << "Operand(0)" << (*inst.getOperand(0)) << "\n";
          auto *ptr = dyn_cast<User>(&inst);
          //errs() << "\t" << *ptr << "\n";
          for (auto it = ptr->op_begin(); it != ptr->op_end(); ++it) {
            errs() << "\t" << *(*it) << "\n";
            // if ((*it)->hasName())
            // errs() << (*it)->getName() << "\n";
          }
        } // end if
      } // end for inst
    } // end for block
    */

    return PreservedAnalyses::all();
  }
};

// New PM registration
struct ValueNumberingPassWrapper : public FunctionPass {
  static char ID;
  ValueNumberingPassWrapper() : FunctionPass(ID) {}

  bool runOnFunction(Function &F) override {
    FunctionAnalysisManager FAM;
    ValueNumberingPass VNP;
    VNP.run(F, FAM);
    return false;
  }
};

} // namespace

char ValueNumberingPassWrapper::ID = 0;

// Register the pass
static llvm::RegisterPass<ValueNumberingPassWrapper>
    X("value-numbering", "ValueNumberingPass", false, false);
