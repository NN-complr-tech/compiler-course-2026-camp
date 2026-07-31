#include "X86.h"
#include "X86InstrInfo.h"
#include "X86Subtarget.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"

using namespace llvm;

namespace {

class ImmediateFoldingPass : public MachineFunctionPass {
public:
  static char ID;
  ImmediateFoldingPass() : MachineFunctionPass(ID) {}

  // the most important function! LLVM calls it for every func
  bool runOnMachineFunction(MachineFunction &MF) override;
  StringRef getPassName() const override {
    return "X86 Immediate Folding";
  }

private:
// help-functions
  bool tryFoldArithmetic(MachineInstr &ArithMI, MachineRegisterInfo &MRI,
                         const TargetInstrInfo *TII);
  unsigned getImmediateOpcode(unsigned Opcode) const;
  bool isSafeToFold(const MachineInstr &ArithMI, const MachineInstr &MovMI,
                    const MachineRegisterInfo &MRI) const;
};

char ImmediateFoldingPass::ID = 0;

} // namespace

static RegisterPass<ImmediateFoldingPass>
    X("immediate-folding", "Fold immediate operands into arithmetic operations",
      false, false);

// first help-func - very easy. simply a table of correspondences
unsigned ImmediateFoldingPass::getImmediateOpcode(unsigned Opcode) const {
  switch (Opcode) {
  case X86::ADD32rr: return X86::ADD32ri;
  case X86::SUB32rr: return X86::SUB32ri;
  case X86::AND32rr: return X86::AND32ri;
  case X86::OR32rr:  return X86::OR32ri;
  case X86::XOR32rr: return X86::XOR32ri;
  default: llvm_unreachable("Unsupported opcode");
  }
}
// there the logics starts - can we even fold?
bool ImmediateFoldingPass::isSafeToFold(const MachineInstr &ArithMI,
                                       const MachineInstr &MovMI,
                                       const MachineRegisterInfo &MRI) const {
  // first of all - is there MOV32?
  if (MovMI.getOpcode() != X86::MOV32ri) return false;

  Register ConstReg = MovMI.getOperand(0).getReg(); // if its MOV32 - taking register from it
  // getOperand(0) - 0 because MOV has 2 regs for example
  // %2 = MOV32ri 67  0 - %2 1 - 67 so we are taking %2


  //checking the virtual register - LLVM distinguishes between physical registers of type eax, ebx and virtual ones %2, %15 
  // pass only works with virtual ones
  if (!ConstReg.isVirtual()) return false;

  // check the number of uses
  //For example, %1 = MOV32ri 10      ADD %0, %1        SUB %2, %1
  //There are two uses here. if we remove MOV, both calculations will fail
  if (!MRI.hasOneUse(ConstReg)) return false;

  // The only user must be ArithMI
  MachineInstr *User = MRI.use_nodbg_begin(ConstReg)->getParent(); // every register knows its user
  if (User != &ArithMI) return false; // make sure that the current instruction uses this register.

  // now making sure that the second operand is really a register and that register is from mov
  const MachineOperand &Src2 = ArithMI.getOperand(2);
  if (!Src2.isReg() || Src2.getReg() != ConstReg) return false;

  return true;
}


// there we create a new instruction
bool ImmediateFoldingPass::tryFoldArithmetic(MachineInstr &ArithMI,
                                            MachineRegisterInfo &MRI,
                                            const TargetInstrInfo *TII) {
  unsigned RR_Opcode = ArithMI.getOpcode();
  // check if this is a supported register-register arithmetic instruction
  switch (RR_Opcode) {
  case X86::ADD32rr:
  case X86::SUB32rr:
  case X86::AND32rr:
  case X86::OR32rr:
  case X86::XOR32rr:
    break;
  default:
    return false;
  }

  // get the second source operand (should be a register)
  const MachineOperand &SrcOp2 = ArithMI.getOperand(2);
  if (!SrcOp2.isReg())
    return false;

  Register SrcReg2 = SrcOp2.getReg();
  if (!SrcReg2.isVirtual())
    return false;

  // findig the definition of that register!
  MachineInstr *DefMI = MRI.getVRegDef(SrcReg2);
  if (!DefMI)
    return false;

  // Verify safety of folding
  if (!isSafeToFold(ArithMI, *DefMI, MRI))
    return false;

  // All checks passed! fold the immediate into a new instruction
  int64_t ImmValue = DefMI->getOperand(1).getImm();
  MachineBasicBlock &MBB = *ArithMI.getParent();
  MachineFunction &MF = *MBB.getParent();

  Register DestReg = ArithMI.getOperand(0).getReg();
  Register SrcReg1 = ArithMI.getOperand(1).getReg();

  unsigned RI_Opcode = getImmediateOpcode(RR_Opcode);
  //finally building new inst
  MachineInstrBuilder MIB = BuildMI(MBB, ArithMI, ArithMI.getDebugLoc(),
                                    TII->get(RI_Opcode));
  MIB.addReg(DestReg, RegState::Define);
  MIB.addReg(SrcReg1);
  MIB.addImm(ImmValue);

  MachineInstr *NewMI = MIB.getInstr();
  //If there are hidden operands they cannot be lost!
//  Therefore, copyImplicitOps(...) moves them to the new instruction.
  NewMI->copyImplicitOps(MF, ArithMI);
  NewMI->setFlags(ArithMI.getFlags());

  // Remove old instructions (they are no longer valid after this)
  ArithMI.eraseFromParent();
  DefMI->eraseFromParent();

  return true;
}

bool ImmediateFoldingPass::runOnMachineFunction(MachineFunction &MF) {
  bool Changed = false;
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const TargetInstrInfo *TII = MF.getSubtarget().getInstrInfo();

  for (MachineBasicBlock &MBB : MF) {
    // We iterate over the instructions. The iterator is incremented before processing
    //because the current instruction may be deleted. If the increment occurred after the deletion, 
    // the iterator would become invalid.
    for (auto I = MBB.begin(), E = MBB.end(); I != E;) {
      MachineInstr &MI = *I++;
      if (tryFoldArithmetic(MI, MRI, TII)) {
        Changed = true;
      }
    }
  }

  return Changed;
}