#include "program_util.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stack>

#include "program.hpp"

void ProgramUtil::removeOps(Program &p, Operation::Type type) {
  auto it = p.ops.begin();
  while (it != p.ops.end()) {
    if (it->type == type) {
      it = p.ops.erase(it);
    } else {
      it++;
    }
  }
}

void ProgramUtil::removeComments(Program &p) {
  for (auto &op : p.ops) {
    op.comment.clear();
  }
}

void ProgramUtil::addComment(Program &p, const std::string &comment) {
  Operation nop(Operation::Type::NOP);
  nop.comment = comment;
  p.ops.push_back(nop);
}

bool ProgramUtil::replaceOps(Program &p, Operation::Type oldType,
                             Operation::Type newType) {
  bool result = false;
  for (Operation &op : p.ops) {
    if (op.type == oldType) {
      op.type = newType;
      result = true;
    }
  }
  return result;
}

bool ProgramUtil::isNop(const Operation &op) {
  if (op.type == Operation::Type::NOP || op.type == Operation::Type::DBG) {
    return true;
  } else if (op.source == op.target && (op.type == Operation::Type::MOV ||
                                        op.type == Operation::Type::MIN ||
                                        op.type == Operation::Type::MAX)) {
    return true;
  } else if (op.source.type == Operand::Type::CONSTANT &&
             op.source.value == 0 &&
             (op.type == Operation::Type::ADD ||
              op.type == Operation::Type::SUB)) {
    return true;
  } else if (op.source.type == Operand::Type::CONSTANT &&
             op.source.value == 1 &&
             ((op.type == Operation::Type::MUL ||
               op.type == Operation::Type::DIV ||
               op.type == Operation::Type::DIF ||
               op.type == Operation::Type::POW ||
               op.type == Operation::Type::BIN))) {
    return true;
  }
  return false;
}

size_t ProgramUtil::numOps(const Program &p, bool withNops) {
  if (withNops) {
    return p.ops.size();
  } else {
    size_t num_ops = 0;
    for (auto &op : p.ops) {
      if (op.type != Operation::Type::NOP) {
        num_ops++;
      }
    }
    return num_ops;
  }
}

size_t ProgramUtil::numOps(const Program &p, Operation::Type type) {
  size_t num_ops = 0;
  for (auto &op : p.ops) {
    if (op.type == type) {
      num_ops++;
    }
  }
  return num_ops;
}

size_t ProgramUtil::numOps(const Program &p, Operand::Type type) {
  size_t num_ops = 0;
  for (auto &op : p.ops) {
    auto &m = Operation::Metadata::get(op.type);
    if (m.num_operands == 1 && op.target.type == type) {
      num_ops++;
    } else if (m.num_operands == 2 &&
               (op.source.type == type || op.target.type == type)) {
      num_ops++;
    }
  }
  return num_ops;
}

bool ProgramUtil::isArithmetic(Operation::Type t) {
  return (t != Operation::Type::NOP && t != Operation::Type::DBG &&
          t != Operation::Type::LPB && t != Operation::Type::LPE &&
          t != Operation::Type::CLR && t != Operation::Type::SEQ);
}

bool ProgramUtil::isCommutative(Operation::Type t) {
  return (t == Operation::Type::ADD || t == Operation::Type::MUL ||
          t == Operation::Type::MIN || t == Operation::Type::MAX ||
          t == Operation::Type::GCD || t == Operation::Type::CMP);
}

bool ProgramUtil::hasIndirectOperand(const Operation &op) {
  const auto num_ops = Operation::Metadata::get(op.type).num_operands;
  return (num_ops > 0 && op.target.type == Operand::Type::INDIRECT) ||
         (num_ops > 1 && op.source.type == Operand::Type::INDIRECT);
}

bool isAdditive(Operation::Type t) {
  return (t == Operation::Type::ADD || t == Operation::Type::SUB);
}

bool ProgramUtil::areIndependent(const Operation &op1, const Operation &op2) {
  if (!isArithmetic(op1.type) && op1.type != Operation::Type::SEQ) {
    return false;
  }
  if (!isArithmetic(op2.type) && op2.type != Operation::Type::SEQ) {
    return false;
  }
  if (hasIndirectOperand(op1) || hasIndirectOperand(op2)) {
    return false;
  }
  if (op1.target.value == op2.target.value &&
      !(isAdditive(op1.type) && isAdditive(op2.type) &&
        !(isCommutative(op1.type) && isCommutative(op2.type)))) {
    return false;
  }
  if (op1.source.type == Operand::Type::DIRECT &&
      op2.target.value == op1.source.value) {
    return false;
  }
  if (op2.source.type == Operand::Type::DIRECT &&
      op1.target.value == op2.source.value) {
    return false;
  }
  return true;
}

bool ProgramUtil::getUsedMemoryCells(const Program &p,
                                     std::unordered_set<int64_t> &used_cells,
                                     int64_t &largest_used,
                                     int64_t max_memory) {
  for (auto &op : p.ops) {
    int64_t region_length = 1;
    if (op.source.type == Operand::Type::INDIRECT ||
        op.target.type == Operand::Type::INDIRECT) {
      return false;
    }
    if (op.type == Operation::Type::LPB || op.type == Operation::Type::CLR) {
      if (op.source.type == Operand::Type::CONSTANT) {
        region_length = op.source.value.asInt();
      } else {
        return false;
      }
    }
    if (region_length > max_memory && max_memory >= 0) {
      return false;
    }
    if (op.source.type == Operand::Type::DIRECT) {
      for (int64_t i = 0; i < region_length; i++) {
        used_cells.insert(op.source.value.asInt() + i);
      }
    }
    if (op.target.type == Operand::Type::DIRECT) {
      for (int64_t i = 0; i < region_length; i++) {
        used_cells.insert(op.target.value.asInt() + i);
      }
    }
  }
  largest_used = used_cells.empty() ? 0 : *used_cells.begin();
  for (int64_t used : used_cells) {
    largest_used = std::max(largest_used, used);
  }
  return true;
}

int64_t ProgramUtil::getLargestDirectMemoryCell(const Program &p) {
  int64_t largest = 0;
  for (auto &op : p.ops) {
    if (op.source.type == Operand::Type::DIRECT) {
      largest = std::max<int64_t>(largest, op.source.value.asInt());
    }
    if (op.target.type == Operand::Type::DIRECT) {
      largest = std::max<int64_t>(largest, op.target.value.asInt());
    }
  }
  return largest;
}

Number ProgramUtil::getLargestConstant(const Program &p) {
  Number largest(-1);
  for (auto &op : p.ops) {
    if (op.source.type == Operand::Type::CONSTANT &&
        largest < op.source.value) {
      largest = op.source.value;
    }
  }
  return largest;
}

bool ProgramUtil::hasLoopWithConstantNumIterations(const Program &p) {
  // assumes that the program is optimized already
  std::map<Number, Number> values;
  for (auto &op : p.ops) {
    if (op.target.type != Operand::Type::DIRECT) {
      values.clear();
      continue;
    }
    if (op.type == Operation::Type::MOV) {
      if (op.source.type == Operand::Type::CONSTANT) {
        values[op.target.value] = op.source.value;
      } else {
        values.erase(op.target.value);
      }
    } else if (op.type == Operation::Type::LPB) {
      if (values.find(op.target.value) != values.end()) {
        return true;
      }
      values.clear();
    } else if (op.type == Operation::Type::LPE) {
      values.clear();
    } else if (ProgramUtil::isArithmetic(op.type)) {
      values.erase(op.target.value);
    }
  }
  return false;
}

std::pair<int64_t, int64_t> ProgramUtil::getEnclosingLoop(const Program &p,
                                                          int64_t op_index) {
  int64_t open_loops;
  std::pair<int64_t, int64_t> loop(-1, -1);
  // find start
  if (p.ops.at(op_index).type != Operation::Type::LPB) {
    if (p.ops.at(op_index).type == Operation::Type::LPE) {
      op_index--;  // get inside the loop
    }
    for (open_loops = 1; op_index >= 0 && open_loops; op_index--) {
      auto t = p.ops.at(op_index).type;
      if (t == Operation::Type::LPB) {
        open_loops--;
      } else if (t == Operation::Type::LPE) {
        open_loops++;
      }
    }
    if (open_loops) {
      return loop;
    }
    op_index++;
  }
  loop.first = op_index;
  // find end
  op_index++;
  for (open_loops = 1;
       op_index < static_cast<int64_t>(p.ops.size()) && open_loops;
       op_index++) {
    auto t = p.ops.at(op_index).type;
    if (t == Operation::Type::LPB) {
      open_loops++;
    } else if (t == Operation::Type::LPE) {
      open_loops--;
    }
  }
  op_index--;
  if (open_loops) {
    print(p, std::cout);
    throw std::runtime_error("invalid program");
  }
  loop.second = op_index;
  // final check
  if (p.ops.at(loop.first).type != Operation::Type::LPB ||
      p.ops.at(loop.second).type != Operation::Type::LPE) {
    throw std::runtime_error("internal error");
  }
  return loop;
}

std::string getIndent(int indent) {
  std::string s;
  for (int i = 0; i < indent; i++) {
    s += " ";
  }
  return s;
}

std::string ProgramUtil::operandToString(const Operand &op) {
  switch (op.type) {
    case Operand::Type::CONSTANT:
      return op.value.to_string();
    case Operand::Type::DIRECT:
      return "$" + op.value.to_string();
    case Operand::Type::INDIRECT:
      return "$$" + op.value.to_string();
  }
  return "";
}

std::string ProgramUtil::operationToString(const Operation &op) {
  auto &metadata = Operation::Metadata::get(op.type);
  std::string str;
  if (metadata.num_operands == 0 && op.type != Operation::Type::NOP) {
    str = metadata.name;
  } else if (metadata.num_operands == 1 ||
             (op.type == Operation::Type::LPB &&
              op.source.type == Operand::Type::CONSTANT &&
              op.source.value == 1))  // lpb has an optional second argument
  {
    str = metadata.name + " " + operandToString(op.target);
  } else if (metadata.num_operands == 2) {
    str = metadata.name + " " + operandToString(op.target) + "," +
          operandToString(op.source);
  }
  if (!op.comment.empty()) {
    if (!str.empty()) {
      str = str + " ";
    }
    str = str + "; " + op.comment;
  }
  return str;
}

void ProgramUtil::print(const Operation &op, std::ostream &out, int indent) {
  out << getIndent(indent) << operationToString(op);
}

void ProgramUtil::print(const Program &p, std::ostream &out,
                        std::string newline) {
  int indent = 0;
  for (auto &op : p.ops) {
    if (op.type == Operation::Type::LPE) {
      indent -= 2;
    }
    print(op, out, indent);
    out << newline;
    if (op.type == Operation::Type::LPB) {
      indent += 2;
    }
  }
}

void ProgramUtil::exportToDot(const Program &p, std::ostream &out) {
  out << "digraph G {" << std::endl;

  // merge operations
  std::vector<std::vector<Operation>> merged;
  merged.push_back({});
  for (auto op : p.ops) {
    if (op.type == Operation::Type::NOP) {
      continue;
    }
    if (!merged.back().empty() && !areIndependent(op, merged.back().back())) {
      merged.push_back({});
    }
    op.comment.clear();
    merged.back().push_back(op);
  }

  // insert forks and joins
  for (size_t i = 0; i < merged.size(); i++) {
    if (merged[i].size() > 1) {
      merged.insert(merged.begin() + i,
                    {Operation(Operation::Type::NOP, {}, {}, "triangle")});
      merged.insert(merged.begin() + i + 2,
                    {Operation(Operation::Type::NOP, {}, {}, "invtriangle")});
      i += 2;
    }
  }

  // nodes
  for (size_t i = 0; i < merged.size(); i++) {
    for (size_t j = 0; j < merged[i].size(); j++) {
      std::string shape = "ellipse";
      std::string color = "black";
      std::string fontname = "courier";
      std::string label = operationToString(merged[i][j]);
      if (merged[i][j].type == Operation::Type::NOP) {
        shape = merged[i][j].comment;
        label.clear();
      } else if (merged[i][j].type == Operation::Type::MOV) {
        color = "blue";
      } else if (isArithmetic(merged[i][j].type)) {
        color = "green";
      } else {
        color = "red";
      }
      out << "  o" << i << "_" << j << " [label=\"" << label
          << "\",shape=" + shape + ",color=" + color + ",fontname=\"" +
                 fontname + "\"];"
          << std::endl;
    }
  }

  // edges
  std::stack<std::string> lpbs;
  std::vector<std::string> targets;

  for (size_t i = 0; i < merged.size(); i++) {
    for (size_t j = 0; j < merged[i].size(); j++) {
      // current source node
      std::string src = "o" + std::to_string(i) + "_" + std::to_string(j);
      // target nodes
      targets.clear();
      if (i + 1 < merged.size())  // edge to next node
      {
        for (size_t k = 0; k < merged[i + 1].size(); k++) {
          targets.push_back("o" + std::to_string(i + 1) + "_" +
                            std::to_string(k));
        }
      }
      if (merged[i][j].type == Operation::Type::LPE)  // edge back to loop start
      {
        targets.push_back(lpbs.top());
        lpbs.pop();
      }
      if (!targets.empty()) {
        out << "  " << src << " -> {";
        for (auto t : targets) {
          out << " " << t;
        }
        out << " }" << std::endl;
      }
      if (merged[i][j].type == Operation::Type::LPB) {
        lpbs.push("o" + std::to_string(i) + "_" + std::to_string(j));
      }
    }
  }
  out << "}" << std::endl;
}

size_t ProgramUtil::hash(const Program &p) {
  size_t h = 0;
  for (auto &op : p.ops) {
    if (op.type != Operation::Type::NOP) {
      h = (h * 3) + hash(op);
    }
  }
  return h;
}

size_t ProgramUtil::hash(const Operation &op) {
  auto &meta = Operation::Metadata::get(op.type);
  size_t h = static_cast<size_t>(op.type);
  if (meta.num_operands > 0) {
    h = (5 * h) + hash(op.target);
  }
  if (meta.num_operands > 1) {
    h = (7 * h) + hash(op.source);
  }
  return h;
}

size_t ProgramUtil::hash(const Operand &op) {
  return (11 * static_cast<size_t>(op.type)) + op.value.hash();
}

void throwInvalidLoop(const Program &p) {
  throw std::runtime_error("invalid loop");
}

void ProgramUtil::validate(const Program &p) {
  // validate number of open/closing loops
  int64_t open_loops = 0;
  auto it = p.ops.begin();
  while (it != p.ops.end()) {
    if (it->type == Operation::Type::LPB) {
      open_loops++;
    } else if (it->type == Operation::Type::LPE) {
      if (open_loops == 0) {
        throwInvalidLoop(p);
      }
      open_loops--;
    }
    it++;
  }
  if (open_loops != 0) {
    throwInvalidLoop(p);
  }
}

void swapCells(Operand &o, int64_t old_cell, int64_t new_cell) {
  if (o == Operand(Operand::Type::DIRECT, old_cell)) {
    o.value = new_cell;
  } else if (o == Operand(Operand::Type::DIRECT, new_cell)) {
    o.value = old_cell;
  }
}

void ProgramUtil::migrateOutputCell(Program &p, int64_t old_out,
                                    int64_t new_out) {
  int64_t found_mov_to_old = -1;
  bool can_switch_old_new = false;
  bool can_replace_target = true;
  int64_t open_loops = 0;
  for (size_t i = 0; i < p.ops.size(); i++) {
    auto &op = p.ops[i];
    if (op.type == Operation::Type::MOV && op.target.value == old_out) {
      found_mov_to_old = i;
      can_replace_target = true;
      can_switch_old_new =
          !open_loops && (op.source == Operand(Operand::Type::DIRECT, new_out));
      if (can_switch_old_new) {
        break;
      }
    }
    if (op.type == Operation::Type::LPB) {
      open_loops++;
      can_replace_target = false;
    } else if (op.type == Operation::Type::LPE) {
      open_loops--;
      can_replace_target = false;
    }
    if (op.target.value != old_out ||
        op.source.type != Operand::Type::CONSTANT) {
      can_replace_target = false;
    }
  }
  if (found_mov_to_old >= 0 && can_switch_old_new) {
    for (size_t i = found_mov_to_old + 1; i < p.ops.size(); i++) {
      swapCells(p.ops[i].target, old_out, new_out);
      swapCells(p.ops[i].source, old_out, new_out);
    }
  } else if (found_mov_to_old >= 0 && can_replace_target) {
    if (p.ops[found_mov_to_old].source ==
        Operand(Operand::Type::DIRECT, new_out)) {
      p.ops.erase(p.ops.begin() + found_mov_to_old,
                  p.ops.begin() + found_mov_to_old + 1);
      found_mov_to_old--;
    } else {
      p.ops[found_mov_to_old].target = Operand(Operand::Type::DIRECT, new_out);
    }
    for (size_t i = found_mov_to_old + 1; i < p.ops.size(); i++) {
      if (p.ops[i].target.value == old_out) {
        p.ops[i].target.value = new_out;
      }
    }
  } else {
    p.push_back(Operation::Type::MOV, Operand::Type::DIRECT, new_out,
                Operand::Type::DIRECT, old_out);
  }
}

bool ProgramUtil::isCodedManually(const Program &p) {
  for (const auto &op : p.ops) {
    if (op.type == Operation::Type::NOP &&
        op.comment.find(PREFIX_CODED_MANUALLY) != std::string::npos) {
      return true;
    }
  }
  return false;
}

std::string ProgramUtil::getCommentField(const Program &p,
                                         const std::string &prefix) {
  for (const auto &op : p.ops) {
    if (op.type == Operation::Type::NOP) {
      auto pos = op.comment.find(prefix);
      if (pos != std::string::npos) {
        return op.comment.substr(pos + prefix.size() + 1);
      }
    }
  }
  return std::string();
}

void ProgramUtil::removeCommentField(Program &p, const std::string &prefix) {
  auto it = p.ops.begin();
  while (it != p.ops.end()) {
    if (it->type == Operation::Type::NOP &&
        it->comment.find(prefix) != std::string::npos) {
      it = p.ops.erase(it);
    } else {
      it++;
    }
  }
}

std::string ProgramUtil::getSequenceIdFromProgram(const Program &p) {
  std::string id_str;
  if (p.ops.empty()) {
    return id_str;  // not found
  }
  auto &c = p.ops[0].comment;
  if (c.length() > 1 && c[0] == 'A' && std::isdigit(c[1])) {
    id_str = c.substr(0, 2);
    for (size_t i = 2; i < c.length() && std::isdigit(c[i]); i++) {
      id_str += c[i];
    }
  }
  return id_str;
}

const std::string ProgramUtil::PREFIX_SUBMITTED_BY = "Submitted by";
const std::string ProgramUtil::PREFIX_CODED_MANUALLY = "Coded manually";
const std::string ProgramUtil::PREFIX_MINER_PROFILE =
    "Miner Profile:";  // colon!

void ProgramUtil::avoidNopOrOverflow(Operation &op) {
  if (op.source.type == Operand::Type::CONSTANT) {
    if (op.source.value == 0 &&
        (op.type == Operation::Type::ADD || op.type == Operation::Type::SUB ||
         op.type == Operation::Type::LPB)) {
      op.source.value = 1;
    }
    if ((op.source.value == 0 || op.source.value == 1) &&
        (op.type == Operation::Type::MUL || op.type == Operation::Type::DIV ||
         op.type == Operation::Type::DIF || op.type == Operation::Type::MOD ||
         op.type == Operation::Type::POW || op.type == Operation::Type::GCD ||
         op.type == Operation::Type::BIN)) {
      op.source.value = 2;
    }
  } else if (op.source.type == Operand::Type::DIRECT) {
    if ((op.source.value == op.target.value) &&
        (op.type == Operation::Type::MOV || op.type == Operation::Type::DIV ||
         op.type == Operation::Type::DIF || op.type == Operation::Type::MOD ||
         op.type == Operation::Type::GCD || op.type == Operation::Type::BIN)) {
      op.target.value += Number::ONE;
    }
  }
}
