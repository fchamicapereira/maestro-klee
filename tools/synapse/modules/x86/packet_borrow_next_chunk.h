#pragma once

#include "../../execution_plan/context.h"
#include "../../log.h"
#include "../module.h"
#include "call-paths-to-bdd.h"

namespace synapse {
namespace targets {
namespace x86 {

class PacketBorrowNextChunk : public Module {
private:
  klee::ref<klee::Expr> p_addr;
  klee::ref<klee::Expr> chunk_addr;
  klee::ref<klee::Expr> chunk;
  klee::ref<klee::Expr> length;

public:
  PacketBorrowNextChunk()
      : Module(ModuleType::x86_PacketBorrowNextChunk, Target::x86,
               "PacketBorrowNextChunk") {}

  PacketBorrowNextChunk(const BDD::Node *node, klee::ref<klee::Expr> _p_addr,
                        klee::ref<klee::Expr> _chunk_addr,
                        klee::ref<klee::Expr> _chunk,
                        klee::ref<klee::Expr> _length)
      : Module(ModuleType::x86_PacketBorrowNextChunk, Target::x86,
               "PacketBorrowNextChunk", node),
        p_addr(_p_addr), chunk_addr(_chunk_addr), chunk(_chunk),
        length(_length) {}

private:
  BDD::BDDVisitor::Action visitBranch(const BDD::Branch *node) override {
    return BDD::BDDVisitor::Action::STOP;
  }

  BDD::BDDVisitor::Action visitCall(const BDD::Call *node) override {
    auto call = node->get_call();

    if (call.function_name == "packet_borrow_next_chunk") {
      assert(!call.args["p"].expr.isNull());
      assert(!call.args["chunk"].out.isNull());
      assert(!call.extra_vars["the_chunk"].second.isNull());
      assert(!call.args["length"].expr.isNull());

      auto _p_addr = call.args["p"].expr;
      auto _chunk_addr = call.args["chunk"].out;
      auto _chunk = call.extra_vars["the_chunk"].second;
      auto _length = call.args["length"].expr;

      auto new_module = std::make_shared<PacketBorrowNextChunk>(
          node, _p_addr, _chunk_addr, _chunk, _length);
      auto ep_node = ExecutionPlanNode::build(new_module);
      auto ep = context->get_current();
      auto new_leaf = ExecutionPlan::leaf_t(ep_node, node->get_next());
      auto new_ep = ExecutionPlan(ep, new_leaf);

      context->add(new_ep, new_module);
    }

    return BDD::BDDVisitor::Action::STOP;
  }

  BDD::BDDVisitor::Action
  visitReturnInit(const BDD::ReturnInit *node) override {
    return BDD::BDDVisitor::Action::STOP;
  }

  BDD::BDDVisitor::Action
  visitReturnProcess(const BDD::ReturnProcess *node) override {
    return BDD::BDDVisitor::Action::STOP;
  }

public:
  virtual void visit(ExecutionPlanVisitor &visitor) const override {
    visitor.visit(this);
  }

  virtual Module_ptr clone() const override {
    auto cloned =
        new PacketBorrowNextChunk(node, p_addr, chunk_addr, chunk, length);
    return std::shared_ptr<Module>(cloned);
  }

  virtual bool equals(const Module *other) const override {
    if (other->get_type() != type) {
      return false;
    }

    auto other_cast = static_cast<const PacketBorrowNextChunk *>(other);

    if (!BDD::solver_toolbox.are_exprs_always_equal(p_addr,
                                                    other_cast->get_p_addr())) {
      return false;
    }

    if (!BDD::solver_toolbox.are_exprs_always_equal(
             chunk_addr, other_cast->get_chunk_addr())) {
      return false;
    }

    if (!BDD::solver_toolbox.are_exprs_always_equal(chunk,
                                                    other_cast->get_chunk())) {
      return false;
    }

    if (!BDD::solver_toolbox.are_exprs_always_equal(length,
                                                    other_cast->get_length())) {
      return false;
    }

    return true;
  }

  const klee::ref<klee::Expr> &get_p_addr() const { return p_addr; }
  const klee::ref<klee::Expr> &get_chunk_addr() const { return chunk_addr; }
  const klee::ref<klee::Expr> &get_chunk() const { return chunk; }
  const klee::ref<klee::Expr> &get_length() const { return length; }
};
} // namespace x86
} // namespace targets
} // namespace synapse
