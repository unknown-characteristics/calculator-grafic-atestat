#pragma once

#include <string>
#include <set>
#include <map>
#include <math.h>
#include <list>
#include <vector>
#include "token.h"
#include "camera.h"


enum class tipFunctie
{
    NullFunc = 0,
    NormalFunc,
    DegenerateFunc
};

namespace SyntaxError
{
    enum
    {
        NoError = 0,
        TextLipsa,
        VariabilaLipsa,
        LiteraFunctieLipsa,
        ParantezeIncorecte,
        VariabilePreaMulte,
        FunctieCuPreaMulteVariabile,
        VariabilaRepetata,
        SintaxaIncorecta,
        OperandLipsa,
        ArgumentePutine,
        ArgumentLipsa,
        ArgumenteMulte,
        ArgumentGol,
        ParantezeGoale,
        CaracterInterzis,
        VariabileNepermise,
        ApelAmbiguu,
        Unexpected
    };

}

namespace ReferenceError
{
    enum
    {
        NoError = 0,
        DependentaLipsa,
        DependentaRecursiva,
        DependentaAreEroare,
        SelfReference,
        FunctiaDejaExista,
        VariabilaExista,
        Unexpected
    };
}

namespace EvaluationError
{
    enum
    {
        NoError = 0,
        VariabileNumarIncorect
    };
}

enum class ErrorType
{
    NoError = 0,
    ParseError,
    ReferenceError,
    EvaluationError
};

struct eroare
{
    ErrorType errorType;
    long long int errorValue, errorInfo;
    eroare(ErrorType a, long long int b, long long int c) : errorType(a), errorValue(b), errorInfo(c) {}
    eroare() : errorType(ErrorType::NoError), errorValue(0), errorInfo(0) {}
};


bool errorIsPositional(eroare err);

struct functionState
{
    eroare eroareFunctie = { ErrorType::NoError,0,0 };
    wchar_t funcLetter = 0;
    std::vector<wchar_t> variables;
    std::set<wchar_t> recrrChain;
    int processStage = 0, funcRegistered = 0, justUpdated = 0, wasCached = 0;
    tipFunctie type = tipFunctie::NullFunc;
    double cachedValue = NAN;
};

class functie
{
private:
    std::wstring funcstr;
    std::list<Token*> rhsTokenList;
    std::list<Token*> lhsTokenList;
    std::list<Token*> tempTokenList;
    Node* headNode = NULL;
    varAssignment vars[100] = { {0,NAN} };

    void clearFunc();
    void clearNode();
    eroare process();
    eroare registerFunc();
    void registerDependencies();
    eroare registerAll();
    void deregisterFunc();
    void deregisterDependencies();
    void deregisterAll();
    void updateDependants(const functionState& oldState);

    static eroare compressParantheses(std::list<Token*>& tokenList);
    eroare convertNormalFunctions(std::list<Token*>& tokenList);
    eroare handleFunctionCalls(std::list<Token*>& tokenList);
    static eroare handleImplicitMultiplication(std::list<Token*>& tokenList);
    eroare handleDegenerateFunctions(std::list<Token*>& tokenList);

    static eroare handleArgumentList(ParansToken* parTok, std::vector<std::list<Token*>>& output);

    static eroare handleExpAndUnaryMinus(std::list<Token*>& tokenList);
    static eroare handleBinaryOperator(std::list<Token*>& tokenList, CodOperator cod, int associativity);
    static eroare handleSameTierBinaryOperators(std::list<Token*>& tokenList, CodOperator cod1, CodOperator cod2);
    static eroare assemble(std::list<Token*>& tokenList);

    eroare stage0();
    eroare stage1();
    eroare stage2();
    eroare stage3();
    eroare stage4();

    void initMembers();
public:
    functionState state;
    std::set<wchar_t> depindeDe;

    double evalFunc(double* values);

    int checkRecurrency(std::set<wchar_t>& recrrStack);
    void updateFromDependency(const functionState& oldState, const functionState& newState);
    void updateDependantsAfterRecurrCheck(const std::set<wchar_t>& recrrStack);
    bool isSpecialFunction();
    eroare updateString(const std::wstring& newString);

    std::wstring getErrorString();

    functie() : functie(L"") {}
    functie(const std::wstring& str) { initMembers(); updateString(str); }

    ~functie() { functionState oldState = state; clearFunc(); updateDependants(oldState); }
};




functie* getLiteraSpreFunctie(wchar_t c);
auto setLiteraSpreFunctie(wchar_t c, functie* ptr);
void removeLiteraSpreFunctie(wchar_t c);

void initConstante();