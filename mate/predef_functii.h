#ifndef __predef_functii_header
#define __predef_functii_header

#include <vector>
#include <string>
typedef double (*math_func_ptr)(double*);
extern double PI; extern double EXP1;

double op_plus(double* a);
double op_minus(double* a);
double op_inmult(double* a);
double op_impart(double* a);
double op_putere(double* a);
double op_unaryPlus(double* a);
double op_unaryMinus(double* a);
double op_factorial(double* a);

double fct_sin(double* a);
double fct_cos(double* a);
double fct_tan(double* a);
double fct_cot(double* a);
double fct_arcsin(double* a);
double fct_arccos(double* a);
double fct_arctan(double* a);
double fct_arccot(double* a);
double fct_ln(double* a);
double fct_log(double* a);
double fct_floor(double* a);
double fct_ceil(double* a);
double fct_modul(double* a);
double fct_nthroot(double* a);
double fct_sqrt(double* a);
double fct_cbrt(double* a);
double fct_pow(double* a);


enum class CodFunctiePredefinita
{
    notPredefined = 0,
    sin,
    cos,
    tan,
    cot,
    arcsin,
    arccos,
    arctan,
    arccot,
    log,
    ln,
    floor,
    ceil,
    abs,
    nthroot,
    sqrt,
    cbrt,
    pow
};

enum class CodOperator
{
    noop = 0,
    unaryPlus,
    unaryMinus,
    implicitMult,
    mult,
    div,
    pow,
    factorial,
    add,
    subtract,
    differentiate,
    comma
};

CodFunctiePredefinita isPredefined(std::wstring s, long long int i);
long long unsigned lengthOfPredefName(CodFunctiePredefinita cod);
CodOperator isOperator(wchar_t let);
math_func_ptr getFunctionFromCode(CodFunctiePredefinita cod);
long long unsigned getNumberOfArgs(CodFunctiePredefinita cod);
math_func_ptr getFunctionFromCode(CodFunctiePredefinita cod);
math_func_ptr getFunctionFromCode(CodOperator cod);


#endif
