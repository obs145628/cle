#pragma once

namespace isa {

class Function;

class FunctionAnalysis {
public:
  FunctionAnalysis(const Function &fun) : _fun(fun) {}
  virtual ~FunctionAnalysis() {}

  const Function &fun() const { return _fun; }

private:
  const Function &_fun;
};

} // namespace isa
