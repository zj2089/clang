//===--- RefactoringActionRuleRequirements.h - Clang refactoring library --===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLING_REFACTOR_REFACTORING_ACTION_RULE_REQUIREMENTS_H
#define LLVM_CLANG_TOOLING_REFACTOR_REFACTORING_ACTION_RULE_REQUIREMENTS_H

#include "clang/Basic/LLVM.h"
#include "clang/Tooling/Refactoring/RefactoringOption.h"
#include "clang/Tooling/Refactoring/RefactoringRuleContext.h"
#include "llvm/Support/Error.h"
#include <type_traits>

namespace clang {
namespace tooling {

/// A refactoring action rule requirement determines when a refactoring action
/// rule can be invoked. The rule can be invoked only when all of the
/// requirements are satisfied.
///
/// Subclasses must implement the
/// 'Expected<T> evaluate(RefactoringRuleContext &) const' member function.
/// \c T is used to determine the return type that is passed to the
/// refactoring rule's constructor.
/// For example, the \c SourceRangeSelectionRequirement subclass defines
/// 'Expected<SourceRange> evaluate(RefactoringRuleContext &Context) const'
/// function. When this function returns a non-error value, the resulting
/// source range is passed to the specific refactoring action rule
/// constructor (provided all other requirements are satisfied).
class RefactoringActionRuleRequirement {
  // Expected<T> evaluate(RefactoringRuleContext &Context) const;
};

/// A base class for any requirement that expects some part of the source to be
/// selected in an editor (or the refactoring tool with the -selection option).
class SourceSelectionRequirement : public RefactoringActionRuleRequirement {};

/// A selection requirement that is satisfied when any portion of the source
/// text is selected.
class SourceRangeSelectionRequirement : public SourceSelectionRequirement {
public:
  Expected<SourceRange> evaluate(RefactoringRuleContext &Context) const {
    if (Context.getSelectionRange().isValid())
      return Context.getSelectionRange();
    // FIXME: Use a diagnostic.
    return llvm::make_error<llvm::StringError>(
        "refactoring action can't be initiated without a selection",
        llvm::inconvertibleErrorCode());
  }
};

/// A base class for any requirement that requires some refactoring options.
class RefactoringOptionsRequirement : public RefactoringActionRuleRequirement {
public:
  virtual ~RefactoringOptionsRequirement() {}

  /// Returns the set of refactoring options that are used when evaluating this
  /// requirement.
  virtual ArrayRef<std::shared_ptr<RefactoringOption>>
  getRefactoringOptions() const = 0;
};

/// A requirement that evaluates to the value of the given \c OptionType when
/// the \c OptionType is a required option. When the \c OptionType is an
/// optional option, the requirement will evaluate to \c None if the option is
/// not specified or to an appropriate value otherwise.
template <typename OptionType>
class OptionRequirement : public RefactoringOptionsRequirement {
public:
  OptionRequirement() : Opt(createRefactoringOption<OptionType>()) {}

  ArrayRef<std::shared_ptr<RefactoringOption>>
  getRefactoringOptions() const final override {
    return Opt;
  }

  Expected<typename OptionType::ValueType>
  evaluate(RefactoringRuleContext &) const {
    return static_cast<OptionType *>(Opt.get())->getValue();
  }

private:
  /// The partially-owned option.
  ///
  /// The ownership of the option is shared among the different requirements
  /// because the same option can be used by multiple rules in one refactoring
  /// action.
  std::shared_ptr<RefactoringOption> Opt;
};

} // end namespace tooling
} // end namespace clang

#endif // LLVM_CLANG_TOOLING_REFACTOR_REFACTORING_ACTION_RULE_REQUIREMENTS_H
