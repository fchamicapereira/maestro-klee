#pragma once

#include "../../execution_plan/context.h"
#include "../../log.h"
#include "../module.h"
#include "call-paths-to-bdd.h"

namespace synapse {
namespace targets {
namespace x86 {

class DchainRejuvenateIndex : public Module {
private:
  klee::ref<klee::Expr> dchain_addr;
  klee::ref<klee::Expr> index;
  klee::ref<klee::Expr> time;

public:
  DchainRejuvenateIndex()
      : Module(ModuleType::x86_DchainRejuvenateIndex, Target::x86,
               "DchainRejuvenate") {}

  DchainRejuvenateIndex(const BDD::Node *node,
                        klee::ref<klee::Expr> _dchain_addr,
                        klee::ref<klee::Expr> _index,
                        klee::ref<klee::Expr> _time)
      : Module(ModuleType::x86_DchainRejuvenateIndex, Target::x86,
               "DchainRejuvenate", node),
        dchain_addr(_dchain_addr), index(_index), time(_time) {}

private:
  BDD::BDDVisitor::Action visitBranch(const BDD::Branch *node) override {
    return BDD::BDDVisitor::Action::STOP;
  }

  BDD::BDDVisitor::Action visitCall(const BDD::Call *node) override {
    auto call = node->get_call();

    if (call.function_name == "dchain_rejuvenate_index") {
      assert(!call.args["chain"].expr.isNull());
      assert(!call.args["index"].expr.isNull());
      assert(!call.args["time"].expr.isNull());

      auto _dchain_addr = call.args["chain"].expr;
      auto _index = call.args["index"].expr;
      auto _time = call.args["time"].expr;

      auto new_module = std::make_shared<DchainRejuvenateIndex>(
          node, _dchain_addr, _index, _time);
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
    auto cloned = new DchainRejuvenateIndex(node, dchain_addr, index, time);
    return std::shared_ptr<Module>(cloned);
  }

  virtual bool equals(const Module *other) const override {
    if (other->get_type() != type) {
      return false;
    }

    auto other_cast = static_cast<const DchainRejuvenateIndex *>(other);

    if (!BDD::solver_toolbox.are_exprs_always_equal(
             dchain_addr, other_cast->get_dchain_addr())) {
      return false;
    }

    if (!BDD::solver_toolbox.are_exprs_always_equal(index,
                                                    other_cast->get_index())) {
      return false;
    }

    if (!BDD::solver_toolbox.are_exprs_always_equal(time,
                                                    other_cast->get_time())) {
      return false;
    }

    return true;
  }

  const klee::ref<klee::Expr> &get_dchain_addr() const { return dchain_addr; }
  const klee::ref<klee::Expr> &get_index() const { return index; }
  const klee::ref<klee::Expr> &get_time() const { return time; }
};
} // namespace x86
} // namespace targets
} // namespace synapse
