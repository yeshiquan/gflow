
#include "expr_processor.h"
#include "vertex.h"

namespace gflow {

void ExpressionProcessor::init(std::string expr_string_,
                               std::string result_name) {
    expr_string = std::move(expr_string_);
    LOG(TRACE) << "trace 1 " << result_name;
    _result_data = get_data(result_name)->make<int>();
    LOG(TRACE) << "trace 2 _result_data:" << _result_data;
}

int ExpressionProcessor::setup() {
    if (!expr.parse(expr_string, ast)) {
        LOG(WARNING) << "expression parse failed expr:" << expr_string;
        return -1;
    }
    if (!expr.var_analyze(ast, &varnames)) {
        LOG(WARNING) << "expression var analyze failed expr:" << expr_string;
        return -1;
    }
    LOG(TRACE) << "ExpressionProcessor expr use variables:[" << noflush;
    for (const std::string& var : varnames) {
        LOG(TRACE) << var << "," << noflush;
        _vertex->depend(var);
    }
    LOG(TRACE) << "]";
    return 0;
}

int ExpressionProcessor::process() {
    LOG(TRACE) << "ExpressionProcessor process()";
    int& result = _result_data->raw<int>();
    std::unordered_map<std::string, int> variables;
    for (const std::string& var : varnames) {
        int var_value = get_data(var)->raw<int>();
        variables.emplace(var, var_value);
    }
    if (!expr.evaluate(expr_string, &variables, ast, result)) {
        LOG(WARNING) << "expression evaluate failed expr:" << expr_string;
        return -1;
    }
    LOG(TRACE) << "ExpressionProcessor process result["
                << _result_data->get_name() << "] -> " << result;
    _result_data->release();
    return 0;
}

}  // namespace;