#include <cmath>
#include <numbers>
#include "predef_functii.h"

double op_plus(double* a)
{
    return a[0]+a[1];
}
double op_minus(double* a)
{
    return a[0]-a[1];
}
double op_inmult(double* a)
{
    return a[0]*a[1];
}
double op_impart(double* a)
{
    if(a[1]==0) return NAN;
    return a[0]/a[1];
}
double op_putere(double* a)
{
    return pow(a[0],a[1]);
}
double op_unaryMinus(double* a)
{
    return -a[0];
}
double op_unaryPlus(double* a)
{
    return a[0];
}
double op_factorial(double* a)
{
    return tgamma(a[0]+1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


double fct_sin(double* a)
{
    return sin(a[0]);
}
double fct_cos(double* a)
{
    return cos(a[0]);
}
double fct_tan(double* a)
{
    return tan(a[0]);
}
double fct_cot(double* a)
{
    static double b[2] = { 1, 0 };
    b[1] = fct_tan(a);
    return op_impart(b);
}
double fct_arcsin(double* a)
{
    return asin(a[0]);
}
double fct_arccos(double* a)
{
    return acos(a[0]);
}
double fct_arctan(double* a)
{
    return atan(a[0]);
}
double fct_pow(double* a)
{
    return pow(a[0], a[1]);
}

double PI = std::numbers::pi;
double EXP1 = std::numbers::e;


double fct_arccot(double* a)
{
    return PI/2 - fct_arctan(a);
}
double fct_ln(double* a)
{
    return log(a[0]);
}
double fct_log(double* a)
{
    static double b[2];
    b[0] = log(a[0]);
    b[1] = log(a[1]);
    return op_impart(b);
}
double fct_floor(double* a)
{
    return floor(a[0]);
}
double fct_ceil(double* a)
{
    return ceil(a[0]);
}
double fct_modul(double* a)
{
    return abs(a[0]);
}
double fct_nthroot(double* a)
{
    static double b[2] = { 1, 0 };
    b[0] = 1;
    b[1] = a[1];
    b[1] = op_impart(b);
    b[0] = a[0];
    return op_putere(b);
}
double fct_sqrt(double* a)
{
    return sqrt(a[0]);
}
double fct_cbrt(double* a)
{
    return cbrt(a[0]);
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CodFunctiePredefinita isPredefined(std::wstring s, long long int i)
{
    //if (!isletter(s[i + 1])) return CodFunctiePredefinita::notPredefined;
    switch (s[i])
    {
        using enum CodFunctiePredefinita;
    case L'l':
    {
        if (s.compare(i, std::wstring::size_type(2), L"ln") == 0) return ln;
        if (s.compare(i, std::wstring::size_type(3), L"log") == 0) return log;
        break;
    }
    case L's':
    {
        if (s.compare(i, std::wstring::size_type(3), L"sin") == 0) return sin;
        else if (s.compare(i, std::wstring::size_type(4), L"sqrt") == 0) return sqrt;
        break;
    }
    case L'a':
    {
        if (s.compare(i, std::wstring::size_type(6), L"arcsin") == 0) return arcsin;
        else if (s.compare(i, std::wstring::size_type(6), L"arccos") == 0) return arccos;
        else if (s.compare(i, std::wstring::size_type(6), L"arctan") == 0) return arctan;
        else if (s.compare(i, std::wstring::size_type(6), L"arccot") == 0) return arccot;
        else if (s.compare(i, std::wstring::size_type(3), L"abs") == 0) return abs;
        break;
    }
    case L't':
    {
        if (s.compare(i, std::wstring::size_type(3), L"tan") == 0) return tan;
        break;
    }
    case L'c':
    {
        if (s.compare(i, std::wstring::size_type(3), L"cos") == 0) return cos;
        else if (s.compare(i, std::wstring::size_type(3), L"cot") == 0) return cot;
        else if (s.compare(i, std::wstring::size_type(4), L"ceil") == 0) return ceil;
        else if (s.compare(i, std::wstring::size_type(4), L"cbrt") == 0) return cbrt;
        break;
    }
    case L'n':
    {
        if (s.compare(i, std::wstring::size_type(7), L"nthroot") == 0) return nthroot;
        break;
    }
    case L'f':
    {
        if (s.compare(i, std::wstring::size_type(5), L"floor") == 0) return floor;
        break;
    }
    case L'p':
    {
        if (s.compare(i, std::wstring::size_type(3), L"pow") == 0) return pow;
    }
    default: return notPredefined;
    }
    return CodFunctiePredefinita::notPredefined;
}

long long unsigned lengthOfPredefName(CodFunctiePredefinita cod)
{
    switch (cod)
    {
        using enum CodFunctiePredefinita;
    case ln:
        return 2;
    case sin:
    case cos:
    case tan:
    case cot:
    case log:
    case pow:
    case abs:
        return 3;
    case sqrt:
    case cbrt:
    case ceil:
        return 4;
    case floor:
        return 5;
    case arccos:
    case arcsin:
    case arctan:
    case arccot:
        return 6;
    case nthroot:
        return 7;
    default:
        return 0;
    }
}

CodOperator isOperator(wchar_t let)
{
    switch (let)
    {
        using enum CodOperator;
    case L'+': return add;
    case L'-': return subtract;
    case L'/': return div;
    case L'*': return mult;
    case L'!': return factorial;
    case L'^': return pow;
    case L',': return comma;
    default: return noop;
    }
}

long long unsigned getNumberOfArgs(CodFunctiePredefinita cod)
{
    switch (cod)
    {
        using enum CodFunctiePredefinita;
    case sin:
    case cos:
    case tan:
    case cot:
    case floor:
    case ceil:
    case arcsin:
    case arccos:
    case arccot:
    case arctan:
    case ln:
    case sqrt:
    case cbrt:
    case abs:
        return 1;
    case log:
    case pow:
    case nthroot:
        return 2;
    default:
        return 0; //should never get here
    }
}

math_func_ptr getFunctionFromCode(CodFunctiePredefinita cod)
{
    switch (cod)
    {
        using enum CodFunctiePredefinita;
    case sin: return fct_sin;
    case cos: return fct_cos;
    case tan: return fct_tan;
    case cot: return fct_cot;
    case floor: return fct_floor;
    case ceil: return fct_ceil;
    case arcsin: return fct_arcsin;
    case arccos: return fct_arccos;
    case arccot: return fct_arccot;
    case arctan: return fct_arctan;
    case ln: return fct_ln;
    case sqrt: return fct_sqrt;
    case cbrt: return fct_cbrt;
    case abs: return fct_modul;
    case log: return fct_log;
    case pow: return fct_pow;
    case nthroot: return fct_nthroot;
    default: return nullptr; //should never get here
    }
}

math_func_ptr getFunctionFromCode(CodOperator cod)
{
    switch (cod)
    {
        using enum CodOperator;
    case implicitMult: return op_inmult;
    case unaryPlus: return op_unaryPlus;
    case unaryMinus: return op_unaryMinus;
    case factorial: return op_factorial;
    case pow: return op_putere;
    case mult: return op_inmult;
    case div: return op_impart;
    case add: return op_plus;
    case subtract: return op_minus;
    default: return nullptr;
    }
}