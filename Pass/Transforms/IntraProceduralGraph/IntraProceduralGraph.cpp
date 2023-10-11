/***
 *
 * #load is followed by a store and load is the "value operand" of the next store, assignment, e.g., a = b
 * #load is followed by a store and load is the "pointer operand" of the next store, assignment, e.g., *a = b
 * #load is followed by a load and store, dereference, e.g., a = *b
 * #store is not successor of a load (single load),
 *  - first operand: ptr address, second operand: ptr address, e.g., a = &b
 *  - first operand: integer/double/float/char, second operand: ptr address
 * Auhtor: Sakib Fuad
 */

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>
#include <unordered_map>
#include <string>
#include <sstream>

using namespace std;

using namespace llvm;

// enum edgetype {
//     ASSIGN = "a",
//     DEREF = "d",
//     IN_DEREF = "-d",
//     ALLOC = "m"
// };

struct edge
{
    string startV;
    string endV;
    string label;
};

/// @brief counter. Here we assign numerical vertex no to each expression for pointsto analysis
/// e.g., a = *b; Here b is assigned 0, *b is assigned 1, and a is assigned 2.
long long int vCounter = 0;
// malloc counter
long long int mCounter = 0;
/// vertex to (expression,type) map. For example, vertex 0 is mapped to *a.
/// type = ptr, noptr
unordered_map<string, string> verToExp;
/// expression to vertex  map. E.g., *a is mapped to 0.
unordered_map<string, string> expToVer;
/// edges map. direct edge from k to v. E.g., (1, 2) means 1 has an direct edge to 2.
vector<edge> edgeList;

// print the edges
void print(vector<edge> edgeList)
{
    for (int i = 0; i < edgeList.size(); i++)
    {
        errs() << edgeList[i].startV << "\t" << edgeList[i].endV << "\t" << edgeList[i].label << "\n";
    }
}

/// print k-v pairs of a map
void print(unordered_map<string, string> targetMap)
{
    for (const auto &kv : targetMap)
    {
        errs() << kv.first << ":" << kv.second << "\n";
    }
}

// void print(unordered_map <string, pair<string, string>> targetMap) {
//     for (const auto &kv : targetMap) {
//         errs() << kv.first << ": (" << kv.second.first << "," << kv.second.second << ")\n";
//     }
// }

namespace
{

    // This method implements what the pass does
    void visitor(Function &F)
    {

        // Here goes what you want to do with a pass

        errs() << "IntraProceduralGraph: " << F.getName() << "\n";
        for (auto &basic_block : F)
        {
            for (auto &inst : basic_block)
            {
                // errs() << inst << "\n";
                // errs() << "opCodeName: " << inst.getOpcodeName() << "\n";
                // errs() << "name:: " << inst.getName() << "\n";
                // errs() << "metadata: " << inst.hasMetadata();

                if (inst.getOpcode() == Instruction::Call)
                {
                    // case: malloc -> a (allocation edge, label: m)
                    // %call = call ptr @malloc(i64 noundef 4) #2
                    // store ptr %call, ptr %a, align 8
                    errs() << "call:: " << inst << "\n";
                    auto *userInst = inst.user_back();
                    if (userInst->getOpcode() == Instruction::Store)
                    {
                        auto *storeInst = dyn_cast<StoreInst>(userInst);
                        string valOpStore = storeInst->getValueOperand()->getName().str();
                        string ptrOpStore = storeInst->getPointerOperand()->getName().str();

                        // create an allocation edge labeled, m
                        struct edge newEdge;
                        newEdge.startV = valOpStore;
                        newEdge.endV = ptrOpStore;
                        newEdge.label = "m";
                        edgeList.push_back(newEdge);
                    }
                }
                else if (inst.getOpcode() == Instruction::Load)
                {
                    errs() << "This is Load: " << inst << "\n";
                    // errs() << "op0: " << inst.getOperand(0)->getName().str() << "\n";
                    // errs() << "op0 type: " << inst.getOperand(0)->getType()->isPointerTy() << "\n";
                    // errs() << "type inst: " << inst.getType()->isPointerTy() << "\n";

                    string startE = inst.getOperand(0)->getName().str();
                    string startV = to_string(vCounter);
                    verToExp.insert(make_pair(startV, startE));
                    expToVer.insert(make_pair(startE, startV));
                    vCounter++;

                    Value *v = &inst;
                    stringstream ss1;
                    ss1 << v;
                    string valueOpLoad = ss1.str();
                    // errs () << "valueOpLoad:: " << valueOpLoad << "\n";
                    errs() << "user back:: " << *(inst.user_back()) << "\n";
                    auto *userInst = inst.user_back();

                    if (userInst->getOpcode() == Instruction::Load)
                    {
                        // case, a = *b
                        // %7 = load ptr, ptr %b, align 8
                        // %8 = load ptr, ptr %7, align 8
                        // store ptr %8, ptr %a, align 8
                        stringstream ss2, ss3;
                        auto *opLoadPtr = userInst->getOperand(0);
                        ss2 << opLoadPtr; // %7
                        string userOpLoad = ss2.str();

                        ss3 << userInst;
                        string userInstAdd = ss3.str(); // %8

                        if (valueOpLoad == userOpLoad && !opLoadPtr->hasName())
                        {
                            auto *userInst2 = userInst->user_back();
                            errs() << "user back 2222: " << *(userInst2) << "\n";
                            if (userInst2->getOpcode() == Instruction::Store)
                            {
                                auto *storeInst = dyn_cast<StoreInst>(userInst2);
                                stringstream ss4;
                                ss4 << storeInst->getValueOperand();
                                string valueOpStore = ss4.str(); // %8

                                if (userInstAdd == valueOpStore && storeInst->getPointerOperand()->hasName())
                                {
                                    string endE = storeInst->getPointerOperand()->getName().str();

                                    // create an assignment edge *b->a
                                    struct edge newEdge;
                                    newEdge.startV = "*" + startE;
                                    newEdge.endV = endE;
                                    newEdge.label = "a";
                                    edgeList.push_back(newEdge);

                                    // create a dereference edge b->*b
                                    newEdge.startV = startE;
                                    newEdge.endV = "*" + startE;
                                    newEdge.label = "d";
                                    edgeList.push_back(newEdge);

                                    // create a dereference edge &b->b
                                    newEdge.startV = "&" + startE;
                                    newEdge.endV = startE;
                                    newEdge.label = "d";
                                    edgeList.push_back(newEdge);

                                    // create an inverse dereference edge *b->b
                                    newEdge.startV = "*" + startE;
                                    newEdge.endV = startE;
                                    newEdge.label = "-d";
                                    edgeList.push_back(newEdge);

                                    // create an inverse dereference edge b->&b
                                    newEdge.startV = startE;
                                    newEdge.endV = "&" + startE;
                                    newEdge.label = "-d";
                                    edgeList.push_back(newEdge);
                                }
                            }
                        }
                    }
                    if (userInst->getOpcode() == Instruction::Store)
                    {
                        auto *storeInst = dyn_cast<StoreInst>(userInst);
                        stringstream ss2;
                        ss2 << storeInst->getValueOperand();
                        string valueOpStore = ss2.str();
                        // errs () << "valueOpStore:: " << valueOpStore << "\n";

                        if (valueOpLoad == valueOpStore && storeInst->getPointerOperand()->hasName())
                        {
                            // case: a =b
                            // % 3 = load i32, ptr % y, align 4
                            // store i32 % 3, ptr % x, align 4
                            string endE = storeInst->getPointerOperand()->getName().str();
                            string endV = to_string(vCounter);
                            vCounter++;

                            struct edge newEdge;
                            newEdge.startV = startE;
                            newEdge.endV = endE;
                            newEdge.label = "a";
                            edgeList.push_back(newEdge);
                        }

                        // Example:
                        // %12 = load i32, ptr %b, align 4
                        // %13 = load ptr, ptr %a, align 8
                        // store i32 %12, ptr %13, align 4
                        // Here, *a = b
                        stringstream ss3;
                        ss3 << storeInst->getPointerOperand();
                        string pointerOpStore = ss3.str();

                        if (valueOpLoad == pointerOpStore)
                        {
                            errs() << "special: " << *(storeInst->getValueOperand()) << "\n";
                            auto *startInst = dyn_cast<User>(storeInst->getValueOperand());
                            errs() << *startInst << "\n";
                            if (startInst->getOperand(0)->hasName())
                            {
                                // *a = b's b
                                string valOpRef = startInst->getOperand(0)->getName().str();

                                // case: *a = b
                                //  b will be "startE" variable's value
                                // In this case there will be three edges created
                                // one: b -> *a (label: a, assignment edge) and another one a -> *a (label: d, dereference edge)
                                // three: &(*a) -> a, inverse dereference edge (label: -d, dereference edge)
                                string endE = storeInst->getPointerOperand()->getName().str();
                                string endV = to_string(vCounter);
                                vCounter++;

                                // create an assignment edge b -> *a
                                struct edge newEdge;
                                newEdge.startV = valOpRef;   // valOpRef = b
                                newEdge.endV = "*" + startE; // startE = a
                                newEdge.label = "a";
                                edgeList.push_back(newEdge);

                                // create two dereference edge
                                // a -> *a
                                // &a -> a
                                newEdge.startV = startE;
                                newEdge.endV = "*" + startE;
                                newEdge.label = "d";
                                edgeList.push_back(newEdge);

                                newEdge.startV = "&" + startE;
                                newEdge.endV = startE;
                                newEdge.label = "d";
                                edgeList.push_back(newEdge);

                                // create two inverse dereference edge
                                // *a -> a
                                // a -> &a
                                newEdge.startV = "*" + startE;
                                newEdge.endV = startE;
                                newEdge.label = "-d";
                                edgeList.push_back(newEdge);

                                newEdge.startV = startE;
                                newEdge.endV = "&" + startE;
                                newEdge.label = "-d";
                                edgeList.push_back(newEdge);
                            }
                        }
                    }
                    // if (inst.getType()->isPointerTy())
                    // {
                    //     errs() << "pointertype"
                    //            << "\n";
                    //     errs() << *(inst.user_back()) << "\n";
                    // }
                }
                else if (inst.getOpcode() == Instruction::Store)
                {
                    // auto *storeInst = dyn_cast<StoreInst>(&inst);
                    // errs() << "This is Store"<< inst << "\n";
                    // errs() << "valueOp: " << storeInst->getValueOperand()->getName().str() << "\n";
                    // errs() << "valueOp type: " << storeInst->getValueOperand()->getType()->isPointerTy() << "\n";
                    // errs() << "pntrOp: " << storeInst->getPointerOperand()->getName().str() << "\n";
                    // errs() << "pntr type: " << storeInst->getPointerOperand()->getType()->isPointerTy() << "\n";
                    // errs() << "type inst: " << storeInst->getType()->isPointerTy() << "\n";
                    // string right1 = inst.getOperand(0)->getName().str();
                    // //errs() << "userback:: " << *(inst.user_back()) << "\n";
                    // if (inst.getType()->isPointerTy()) {
                    //     errs() << "pointertype" << "\n";
                    //     errs() << *(inst.user_back()) << "\n";
                    // }

                    // case, a = &b
                    // store ptr %b, ptr %a, align 8
                    auto *storeInst = dyn_cast<StoreInst>(&inst);
                    auto *valueOp = storeInst->getValueOperand();
                    auto *storeOp = storeInst->getPointerOperand();

                    if (valueOp->hasName() && storeOp->hasName())
                    {
                        errs() << "special store:: " << *storeInst << "\n";
                        string valueOpName = valueOp->getName().str();
                        string storeOpName = storeOp->getName().str();

                        // create an assignment edge &b -> a
                        struct edge newEdge;
                        newEdge.startV = "&" + valueOpName; // valueOpName = b
                        newEdge.endV = storeOpName;         // startE = a
                        newEdge.label = "a";
                        edgeList.push_back(newEdge);

                        // create a dereference edge &b -> b
                        newEdge.startV = "&" + valueOpName;
                        newEdge.endV = valueOpName;
                        newEdge.label = "d";
                        edgeList.push_back(newEdge);
                    }
                }
                /*
                if (inst.isBinaryOp())
                {
                    errs() << "Op Code:" << inst.getOpcodeName()<<"\n";
                    if(inst.getOpcode() == Instruction::Add){
                        errs() << "This is Addition"<<"\n";
                    }
                    if(inst.getOpcode() == Instruction::Load){
                        errs() << "This is Load"<<"\n";
                    }
                    if(inst.getOpcode() == Instruction::Mul){
                        errs() << "This is Multiplication"<<"\n";
                    }

                    // see other classes, Instruction::Sub, Instruction::UDiv, Instruction::SDiv
                    // errs() << "Operand(0)" << (*inst.getOperand(0))<<"\n";
                    auto* ptr = dyn_cast<User>(&inst);
                    //errs() << "\t" << *ptr << "\n";
                    for (auto it = ptr->op_begin(); it != ptr->op_end(); ++it) {
                        errs() << "\t" <<  *(*it) << "\n";
                        // if ((*it)->hasName())
                        // errs() << (*it)->getName() << "\n";
                    }
                } // end if
                */
            } // end for inst
        }     // end for block

        errs() << "Print ver to exp: \n";
        print(verToExp);

        errs() << "Print exp to ver: \n";
        print(expToVer);

        errs() << "Edges \n";
        print(edgeList);
    }

    // New PM implementation
    struct IntraProceduralGraphPass : public PassInfoMixin<IntraProceduralGraphPass>
    {

        // The first argument of the run() function defines on what level
        // of granularity your pass will run (e.g. Module, Function).
        // The second argument is the corresponding AnalysisManager
        // (e.g ModuleAnalysisManager, FunctionAnalysisManager)
        PreservedAnalyses run(Function &F, FunctionAnalysisManager &)
        {
            visitor(F);
            return PreservedAnalyses::all();
        }

        static bool isRequired() { return true; }
    };
}

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK
llvmGetPassPluginInfo()
{
    return {
        LLVM_PLUGIN_API_VERSION, "IntraProceduralGraphPass", LLVM_VERSION_STRING,
        [](PassBuilder &PB)
        {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>)
                {
                    if (Name == "intra-procedural-graph")
                    {
                        FPM.addPass(IntraProceduralGraphPass());
                        return true;
                    }
                    return false;
                });
        }};
}
