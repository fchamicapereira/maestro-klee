#pragma once

#include "../../execution_plan/context.h"
#include "../../log.h"
#include "../module.h"
#include "call-paths-to-bdd.h"

#include "else.h"
#include "then.h"

namespace synapse {
namespace targets {
namespace p4BMv2SimpleSwitchgRPC {

class If : public Module {
private:
  klee::ref<klee::Expr> condition;

public:
  If()
      : Module(ModuleType::p4BMv2SimpleSwitchgRPC_If,
               Target::p4BMv2SimpleSwitchgRPC, "If") {}

  If(const BDD::Node *node, klee::ref<klee::Expr> _condition)
      : Module(ModuleType::p4BMv2SimpleSwitchgRPC_If,
               Target::p4BMv2SimpleSwitchgRPC, "If", node),
        condition(_condition) {}

private:
  BDD::BDDVisitor::Action visitBranch(const BDD::Branch *node) override {
    if (query_contains_map_has_key(node)) {
      return BDD::BDDVisitor::Action::STOP;
    }

    assert(!node->get_condition().isNull());
    auto _condition = node->get_condition();

    auto new_if_module = std::make_shared<If>(node, _condition);
    auto new_then_module = std::make_shared<Then>(node);
    auto new_else_module = std::make_shared<Else>(node);

    auto if_ep_node = ExecutionPlanNode::build(new_if_module);
    auto then_ep_node = ExecutionPlanNode::build(new_then_module);
    auto else_ep_node = ExecutionPlanNode::build(new_else_module);

    auto if_leaf = ExecutionPlan::leaf_t(if_ep_node, nullptr);
    auto then_leaf = ExecutionPlan::leaf_t(then_ep_node, node->get_on_true());
    auto else_leaf = ExecutionPlan::leaf_t(else_ep_node, node->get_on_false());

    std::vector<ExecutionPlan::leaf_t> if_leaves{ if_leaf };
    std::vector<ExecutionPlan::leaf_t> then_else_leaves{ then_leaf, else_leaf };

    auto ep = context->get_current();
    auto ep_if = ExecutionPlan(ep, if_leaves, bdd);
    auto ep_if_then_else = ExecutionPlan(ep_if, then_else_leaves, bdd);

    context->add(ep_if_then_else, new_if_module);

    return BDD::BDDVisitor::Action::STOP;
  }

  BDD::BDDVisitor::Action visitCall(const BDD::Call *node) override {
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
    auto cloned = new If(node, condition);
    return std::shared_ptr<Module>(cloned);
  }

  const klee::ref<klee::Expr> &get_condition() const { return condition; }
};
} // namespace p4BMv2SimpleSwitchgRPC
} // namespace targets
} // namespace synapse
