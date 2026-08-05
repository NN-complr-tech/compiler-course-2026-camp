#include "AArch64.h"
#include "AArch64InstrInfo.h"
#include "AArch64Subtarget.h"
#include "MCTargetDesc/AArch64AddressingModes.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include <optional>

using namespace llvm;

namespace {

struct FoldingTarget {
  unsigned NewOpc;
  bool IsLogical;
  unsigned RegSize;
};

class ImmediateFoldingPass : public MachineFunctionPass {
public:
  static char ID;
  ImmediateFoldingPass() : MachineFunctionPass(ID) {}

  bool runOnMachineFunction(MachineFunction &MF) override;

private:
  std::optional<uint64_t> getConstantValue(const MachineInstr *MI);
  std::optional<FoldingTarget> getFoldingTarget(unsigned Opcode);
  bool isCommutative(unsigned Opcode);
};

char ImmediateFoldingPass::ID = 0;

std::optional<uint64_t>
ImmediateFoldingPass::getConstantValue(const MachineInstr *MI) {
  if (!MI)
    return std::nullopt;

  unsigned Opc = MI->getOpcode();
  if (Opc == AArch64::MOVZWi || Opc == AArch64::MOVZXi) {
    if (MI->getOperand(1).isImm() && MI->getOperand(2).isImm()) {
      uint64_t Val = MI->getOperand(1).getImm();
      uint64_t Shift = MI->getOperand(2).getImm();
      return Val << Shift;
    }
  } else if (Opc == AArch64::MOVi32imm || Opc == AArch64::MOVi64imm) {
    if (MI->getOperand(1).isImm()) {
      return MI->getOperand(1).getImm();
    }
  }
  return std::nullopt;
}

std::optional<FoldingTarget>
ImmediateFoldingPass::getFoldingTarget(unsigned Opcode) {
  switch (Opcode) {
  case AArch64::ADDWrr:
    return FoldingTarget{AArch64::ADDWri, false, 32};
  case AArch64::SUBWrr:
    return FoldingTarget{AArch64::SUBWri, false, 32};
  case AArch64::ANDWrr:
    return FoldingTarget{AArch64::ANDWri, true, 32};
  case AArch64::ORRWrr:
    return FoldingTarget{AArch64::ORRWri, true, 32};
  case AArch64::EORWrr:
    return FoldingTarget{AArch64::EORWri, true, 32};
  case AArch64::ADDXrr:
    return FoldingTarget{AArch64::ADDXri, false, 64};
  case AArch64::SUBXrr:
    return FoldingTarget{AArch64::SUBXri, false, 64};
  case AArch64::ANDXrr:
    return FoldingTarget{AArch64::ANDXri, true, 64};
  case AArch64::ORRXrr:
    return FoldingTarget{AArch64::ORRXri, true, 64};
  case AArch64::EORXrr:
    return FoldingTarget{AArch64::EORXri, true, 64};
  default:
    return std::nullopt;
  }
}

bool ImmediateFoldingPass::isCommutative(unsigned Opcode) {
  switch (Opcode) {
  case AArch64::ADDWrr:
  case AArch64::ADDXrr:
  case AArch64::ANDWrr:
  case AArch64::ANDXrr:
  case AArch64::ORRWrr:
  case AArch64::ORRXrr:
  case AArch64::EORWrr:
  case AArch64::EORXrr:
    return true;
  default:
    return false;
  }
}

bool ImmediateFoldingPass::runOnMachineFunction(MachineFunction &MF) {
  const TargetInstrInfo *TII = MF.getSubtarget().getInstrInfo();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  bool Changed = false;

  for (MachineBasicBlock &MBB : MF) {
    for (auto MII = MBB.begin(), MIE = MBB.end(); MII != MIE;) {
      MachineInstr &MI = *MII++;

      auto Target = getFoldingTarget(MI.getOpcode());
      if (!Target)
        continue;

      if (MI.getNumOperands() < 3 || !MI.getOperand(1).isReg() ||
          !MI.getOperand(2).isReg())
        continue;

      Register Reg1 = MI.getOperand(1).getReg();
      Register Reg2 = MI.getOperand(2).getReg();

      if (!Reg1.isVirtual() || !Reg2.isVirtual())
        continue;

      MachineInstr *Def1 = MRI.getUniqueVRegDef(Reg1);
      MachineInstr *Def2 = MRI.getUniqueVRegDef(Reg2);

      auto Val1 = getConstantValue(Def1);
      auto Val2 = getConstantValue(Def2);

      Register RegToKeep;
      uint64_t ImmVal = 0;
      bool CanFold = false;

      if (Val2) {
        RegToKeep = Reg1;
        ImmVal = *Val2;
        CanFold = true;
      } else if (Val1 && isCommutative(MI.getOpcode())) {
        RegToKeep = Reg2;
        ImmVal = *Val1;
        CanFold = true;
      }

      if (!CanFold)
        continue;

      uint64_t EncodedImm = ImmVal;

      if (Target->IsLogical) {
        if (!AArch64_AM::isLogicalImmediate(ImmVal, Target->RegSize))
          continue;
        EncodedImm =
            AArch64_AM::encodeLogicalImmediate(ImmVal, Target->RegSize);
      } else {
        if (ImmVal >= 4096)
          continue;
      }

      auto MIB = BuildMI(MBB, MI, MI.getDebugLoc(), TII->get(Target->NewOpc))
                     .add(MI.getOperand(0))
                     .addReg(RegToKeep);

      if (Target->IsLogical) {
        MIB.addImm(EncodedImm);
      } else {
        MIB.addImm(EncodedImm);
        MIB.addImm(0);
      }

      MI.eraseFromParent();
      Changed = true;
    }
  }

  return Changed;
}

} // namespace

static RegisterPass<ImmediateFoldingPass>
    X("aarch64-imm-folding", "AArch64 Immediate Operand Folding Pass", false,
      false);