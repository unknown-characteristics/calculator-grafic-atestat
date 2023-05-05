#include <iostream>
#include <stack>
#include "predef_functii.h"
#include "mate.h"

int isnum(wchar_t c)
{
    static std::wstring numstr = L".0123456789";
    if(numstr.find(c)==numstr.npos) return 0;
    else return 1;
}

int isspecial(wchar_t c)
{
    if(c==0) return 1;
    static std::wstring specialstr = L"()+-/^*=!";
    if(specialstr.find(c)==specialstr.npos) return 0;
    else return 1;
}

int isillegal(wchar_t c)
{
    static std::wstring illegalstr = L"[]{}\\';_@#$%&";
    if(illegalstr.find(c)==illegalstr.npos) return 0;
    else return 1;
}

int isletter(wchar_t c)
{
    static std::wstring letterstr = L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZαβγδεζηθικλμνξοπρστυφχψωăîâșț";
    if (letterstr.find(c) == letterstr.npos) return 0;
    else return 1;
}

std::wstring functie::getErrorString()
{
    auto& err = state.eroareFunctie;
    switch (err.errorType)
    {
        using enum ErrorType;
    case ParseError:
    {
        switch (err.errorValue)
        {
        case SyntaxError::ApelAmbiguu: return L"Apel ambiguu al funcției predefinite";
        case SyntaxError::TextLipsa: return L"Text lipsă";
        case SyntaxError::VariabilaLipsa: return L"Variabilă lipsă";
        case SyntaxError::LiteraFunctieLipsa: return L"Litera funcției lipsa";
        case SyntaxError::ParantezeIncorecte: return L"Paranteze incorecte";
        case SyntaxError::ParantezeGoale: return L"Paranteze goale";
        case SyntaxError::VariabilePreaMulte: return L"Variabile prea multe";
        case SyntaxError::FunctieCuPreaMulteVariabile: return L"Funcția are prea multe variabile";
        case SyntaxError::VariabilaRepetata: return std::wstring(L"Variabila se repetă (") + (wchar_t)err.errorInfo + L")";
        case SyntaxError::SintaxaIncorecta: return L"Sintaxă incorectă";
        case SyntaxError::OperandLipsa: return L"Operand lipsă";
        case SyntaxError::ArgumentePutine: return L"Argumente prea puține";
        case SyntaxError::ArgumenteMulte: return L"Argumente prea multe";
        case SyntaxError::VariabileNepermise: return L"O funcție cu litera 'x' nu poate avea variabile";
        case SyntaxError::ArgumentLipsa: return L"Argument lipsă";
        case SyntaxError::ArgumentGol: return L"Argument gol";
        case SyntaxError::CaracterInterzis: return std::wstring(L"Caracter interzis (") + funcstr[err.errorInfo] + L")";
        case SyntaxError::Unexpected: return L"Eroare neașteptată";
        default: return L"";
        }
    }
    case ReferenceError:
    {
        switch (err.errorValue)
        {
        case ReferenceError::DependentaLipsa: return std::wstring(L"Dependența nu este definită (") + (wchar_t)err.errorInfo + L")";
        case ReferenceError::DependentaRecursiva: return L"Funcția este definită recursiv";
        case ReferenceError::DependentaAreEroare: return std::wstring(L"Dependența are eroare (") + (wchar_t)err.errorInfo + L")";
        case ReferenceError::SelfReference: return L"Funcția este definită in funcție de sine";
        case ReferenceError::FunctiaDejaExista: return L"Funcția cu numele acesta este deja definită";
        case ReferenceError::VariabilaExista: return std::wstring(L"Variabila este deja definita (")  + (wchar_t)err.errorInfo + L")";
        case ReferenceError::Unexpected: return L"Eroare neașteptată";
        default: return L"";
        }
    }
    default: return L"";
    }
}

bool errorIsPositional(eroare err)
{
    switch (err.errorType)
    {
        using enum ErrorType;
    case ParseError:
    {
        switch (err.errorValue)
        {
        case SyntaxError::ApelAmbiguu:
        case SyntaxError::VariabilaLipsa:
        case SyntaxError::LiteraFunctieLipsa:
        case SyntaxError::ParantezeIncorecte:
        case SyntaxError::ParantezeGoale:
        case SyntaxError::SintaxaIncorecta:
        case SyntaxError::OperandLipsa:
        case SyntaxError::ArgumentePutine:
        case SyntaxError::ArgumenteMulte:
        case SyntaxError::ArgumentLipsa:
        case SyntaxError::ArgumentGol:
        case SyntaxError::CaracterInterzis:
        case SyntaxError::Unexpected:
            return true;
        case SyntaxError::VariabilePreaMulte:
        case SyntaxError::VariabilaRepetata:
        case SyntaxError::VariabileNepermise:
        case SyntaxError::FunctieCuPreaMulteVariabile:
        case SyntaxError::TextLipsa:
        default: 
            return false;
        }
    }
    case ReferenceError:
    {
        switch (err.errorValue)
        {
        case ReferenceError::SelfReference:
            return true;
        case ReferenceError::DependentaLipsa:
        case ReferenceError::DependentaAreEroare:
        case ReferenceError::VariabilaExista:
        case ReferenceError::DependentaRecursiva:
        case ReferenceError::FunctiaDejaExista:
        case ReferenceError::Unexpected:
        default: 
            return false;
        }
    }
    default: return false;
    }
}

std::map<wchar_t, functie*> litSpFct;
std::map<wchar_t, std::set<functie*>> dependedOn;

functie* getLiteraSpreFunctie(wchar_t c)
{
    return litSpFct[c];
}
auto setLiteraSpreFunctie(wchar_t c, functie* ptr)
{
    return litSpFct.insert_or_assign(c,ptr);
}
void removeLiteraSpreFunctie(wchar_t c)
{
    litSpFct.erase(c);
}


int findElem(std::vector<wchar_t> vec, wchar_t c)
{
    for(wchar_t let : vec)
        if(let==c)
            return 1;
    return 0;
}


eroare functie::stage0()    //initial tokenization, handles numbers and predefined function names, illegal characters, registers function, gets dependencies
{
    clearFunc();
    eroare tmp = {ErrorType::NoError, 0, 0};
    
    if (funcstr == L"") return { ErrorType::ParseError, SyntaxError::TextLipsa, 0 };

    int buildingNumber = 0, mode = 0;
    double number = 0, p = 1;
    for (long long unsigned i = 0; i < funcstr.length(); i++)
    {
        if (isillegal(funcstr[i])) return { ErrorType::ParseError, SyntaxError::CaracterInterzis, (long long int)i };
        
        if (isnum(funcstr[i]))
        {
            buildingNumber = 1;
            if (funcstr[i] == L'.')
            {
                mode++;
                if (mode > 1) return { ErrorType::ParseError, SyntaxError::SintaxaIncorecta, (long long int)i };
            }
            else
            {
                if (mode == 0)
                {
                    number = number * 10 + (funcstr[i] - L'0');
                }
                else
                {
                    p /= 10;
                    number += p * (funcstr[i] - L'0');
                }
            }
            continue;
        }
        else if(buildingNumber)
        { 
            if (mode && p == 1) return { ErrorType::ParseError, SyntaxError::SintaxaIncorecta, (long long int)i };

            NumberToken* newTok = new NumberToken(number, (int)i);
            rhsTokenList.push_back(static_cast<Token*>(newTok));

            mode = buildingNumber = 0; number = 0; p = 1;
        }

        if (funcstr[i] == L'=')
        {
            if (i == 0) return { ErrorType::ParseError, SyntaxError::SintaxaIncorecta, 0 };
            if (lhsTokenList.size() != 0) return { ErrorType::ParseError, SyntaxError::SintaxaIncorecta, (long long int)i };
            lhsTokenList.splice(lhsTokenList.end(), rhsTokenList);
            continue;
        }
        
        if (funcstr[i] == L'(' || funcstr[i] == L')')
        {
            ParanToken* newTok = new ParanToken(funcstr[i] == L'(' ? 1 : 0, (int)i);
            rhsTokenList.push_back(static_cast<Token*>(newTok));
            continue;
        }

        CodOperator attempt = isOperator(funcstr[i]);
        if (attempt != CodOperator::noop)
        {
            OperatorToken* newTok = new OperatorToken(attempt, 0, (int)i);
            rhsTokenList.push_back(static_cast<Token*>(newTok));
            continue;
        }

        CodFunctiePredefinita attempt2 = isPredefined(funcstr, i);
        if (attempt2 != CodFunctiePredefinita::notPredefined)
        {
            PredefFuncToken* newTok = new PredefFuncToken(attempt2, (int)i);
            rhsTokenList.push_back(static_cast<Token*>(newTok));
            i += lengthOfPredefName(attempt2) - 1;
            continue;
        }

        //if all else fails...this should be a letter.
        if(!isletter(funcstr[i])) return { ErrorType::ParseError, SyntaxError::CaracterInterzis, (long long int)i };
        LetterToken* newTok = new LetterToken(funcstr[i], (int)i);
        rhsTokenList.push_back(static_cast<Token*>(newTok));
    }

    if (buildingNumber)
    {
        if (mode && p == 1) return { ErrorType::ParseError, SyntaxError::SintaxaIncorecta, (long long int)funcstr.length()-1};

        NumberToken* newTok = new NumberToken(number, (int)funcstr.length() - 1);
        rhsTokenList.push_back(static_cast<Token*>(newTok));

        mode = buildingNumber = 0; number = 0; p = 1;
    }



    if (lhsTokenList.size() == 0)
    {
        state.funcRegistered = 1;
        state.type = tipFunctie::DegenerateFunc;
        state.funcLetter = 0;
    }
    else
    {
        Token& first = **lhsTokenList.begin();
        if (first.type != tipToken::LetterToken) return { ErrorType::ParseError, SyntaxError::LiteraFunctieLipsa, 0 };
        
        LetterToken& realFirst = static_cast<LetterToken&>(first);
        state.funcLetter = realFirst.letter;
        
        if (lhsTokenList.size() == 1)
        {
            state.type = tipFunctie::DegenerateFunc;
            tmp = registerFunc();
            //only possible error
            if (tmp.errorType == ErrorType::ReferenceError && tmp.errorValue == ReferenceError::FunctiaDejaExista)
            {
                depindeDe.insert(state.funcLetter);  //get informed when real function disappears
                state.funcLetter = 0;
                registerDependencies();
                return tmp;
            }
            state.funcRegistered = 1;
        }
        else
        {
            tmp = compressParantheses(lhsTokenList);
            if (tmp.errorType != ErrorType::NoError) return tmp;

            Token& nextElem = **std::next(lhsTokenList.begin());
            if (nextElem.type != tipToken::ParansToken) return { ErrorType::ParseError, SyntaxError::SintaxaIncorecta, 0 };

            if (lhsTokenList.size() > 2) return { ErrorType::ParseError, SyntaxError::SintaxaIncorecta, (**std::next(lhsTokenList.end(),-1)).posInString };
            state.type = tipFunctie::NormalFunc;
            if (state.funcLetter == L'x') return { ErrorType::ParseError, SyntaxError::VariabileNepermise, 0 };

            ParansToken& realNextElem = static_cast<ParansToken&>(nextElem);
            for (Token* tok : realNextElem.innerTokenList)
            {
                if (tok->type != tipToken::LetterToken) continue;

                LetterToken* realTok = static_cast<LetterToken*>(tok);
                if (realTok->letter == state.funcLetter) return { ErrorType::ReferenceError, ReferenceError::SelfReference, tok->posInString };
                depindeDe.insert(realTok->letter);
            }

            //promote letter token to function token
            auto firstIt = lhsTokenList.begin();
            delete *firstIt;

            FunctionToken* newTok = new FunctionToken(state.funcLetter, 0);
            *firstIt = newTok;

            FunctionToken& realElem = *newTok;
            eroare tmp = handleArgumentList(static_cast<ParansToken*>(&nextElem), realElem.args.argv);
            if (tmp.errorType != ErrorType::NoError) return tmp;

            tmp = registerFunc();
            //only possible error
            if (tmp.errorType == ErrorType::ReferenceError && tmp.errorValue == ReferenceError::FunctiaDejaExista)
            {
                depindeDe.insert(state.funcLetter);
                state.funcLetter = 0;
                registerDependencies();
                return tmp;
            }
            
            state.funcRegistered = 1;
        }
    }

    for (Token* tok : rhsTokenList)
    {
        if (tok->type != tipToken::LetterToken) continue;

        LetterToken* realTok = static_cast<LetterToken*>(tok);
        if (realTok->letter == state.funcLetter) return { ErrorType::ReferenceError, ReferenceError::SelfReference, tok->posInString };
        depindeDe.insert(realTok->letter);
    }

    registerDependencies();
    return tmp;
}

eroare functie::stage1()   //determine recurrency and if dependencies are ok
{
    state.processStage = 1;
    eroare tmp = {ErrorType::NoError, 0, 0};

    state.recrrChain.clear();
    if(state.funcLetter != 0 && state.funcLetter != L'x' && state.funcLetter != L'y')    //these can not cause recurrencies
    {
        std::set<wchar_t> recrrTmp; recrrTmp.clear();
        recrrTmp.insert(state.funcLetter);
        int rez = checkRecurrency(recrrTmp);
        if(rez==-1)
        {
            state.recrrChain = recrrTmp;
            tmp = {ErrorType::ReferenceError, ReferenceError::DependentaRecursiva, 0};
            state.eroareFunctie = tmp;
            for(wchar_t let : recrrTmp)
            {
                functie* ptr = getLiteraSpreFunctie(let);
                ptr->updateDependantsAfterRecurrCheck(recrrTmp);
            }
            return tmp;
        }
    }


    for(wchar_t let : depindeDe)
    {
        functie* ptr = getLiteraSpreFunctie(let);
        if(ptr==NULL) continue; //possibly the variable, probably inexistent dependency
        if(ptr->state.eroareFunctie.errorType != ErrorType::NoError)
            return {ErrorType::ReferenceError, ReferenceError::DependentaAreEroare, (long long int)ptr->state.funcLetter};
    }

    return tmp;
}

eroare functie::stage2()   //determine variables and variable dependency
{
    state.processStage = 2;
    eroare tmp = {ErrorType::NoError, 0, 0};

    std::set<wchar_t> possibleVariables;

    for(wchar_t let : depindeDe)
    {
        functie* ptr = getLiteraSpreFunctie(let);
        if(ptr==NULL)
        {
            possibleVariables.insert(let);
        }
        else if(ptr->state.type==tipFunctie::DegenerateFunc)
        {
            if(ptr->state.variables.size()>0)
                possibleVariables.insert(ptr->state.variables[0]);
        }
    }

    state.variables.clear();
    if(state.type == tipFunctie::NormalFunc)
    {
        FunctionToken& elem = *static_cast<FunctionToken*>(*lhsTokenList.begin());
        for(auto& tokList : elem.args.argv)
        {
            if(tokList.size()!=1)
                return {ErrorType::ParseError, SyntaxError::SintaxaIncorecta, (**tokList.begin()).posInString};

            Token& elem = **tokList.begin();
            if(elem.type!=tipToken::LetterToken)
                return {ErrorType::ParseError, SyntaxError::SintaxaIncorecta, elem.posInString};

            LetterToken& realElem = static_cast<LetterToken&>(elem);
            if(realElem.letter == state.funcLetter)
                return {ErrorType::ReferenceError, ReferenceError::SelfReference, realElem.posInString};
            functie* ptr = getLiteraSpreFunctie(realElem.letter);
            if(ptr!=NULL)
                return {ErrorType::ReferenceError, ReferenceError::VariabilaExista, realElem.letter};

            if(findElem(state.variables, realElem.letter))
                return {ErrorType::ParseError, SyntaxError::VariabilaRepetata, realElem.letter};

            state.variables.push_back(realElem.letter);
        }

        for(wchar_t let : possibleVariables)
            if(!findElem(state.variables, let))
                return {ErrorType::ReferenceError, ReferenceError::DependentaLipsa, let};

        if (state.variables.size() > 100) return { ErrorType::ParseError, SyntaxError::FunctieCuPreaMulteVariabile, 100 };
    }
    else
    {
        if(possibleVariables.size()>1)
            return {ErrorType::ParseError, SyntaxError::VariabilePreaMulte, 0}; //should maybe indicate the variables

        if(possibleVariables.size()==1)
            state.variables.push_back(*possibleVariables.begin());

        if (state.variables.size() != 0 && state.funcLetter == L'x') return { ErrorType::ParseError, SyntaxError::VariabileNepermise, 0 };
    }
    return tmp;
}


std::list<Token*> makeNewList(std::list<Token*>& original)
{
    std::list<Token*> output;
    for (Token* tok : original)
    {
        Token* newTok = nullptr;
        switch (tok->type)
        {
        case tipToken::NumberToken:
        {
            NumberToken* realTok = static_cast<NumberToken*>(tok);
            NumberToken* realNewTok = new NumberToken();
            *realNewTok = *realTok;
            newTok = static_cast<Token*>(realNewTok);
            break;
        }
        case tipToken::LetterToken:
        {
            LetterToken* realTok = static_cast<LetterToken*>(tok);
            LetterToken* realNewTok = new LetterToken();
            *realNewTok = *realTok;
            newTok = static_cast<Token*>(realNewTok);
            break;
        }
        case tipToken::ParanToken:
        {
            ParanToken* realTok = static_cast<ParanToken*>(tok);
            ParanToken* realNewTok = new ParanToken();
            *realNewTok = *realTok;
            newTok = static_cast<Token*>(realNewTok);
            break;
        }
        case tipToken::OperatorToken:
        {
            OperatorToken* realTok = static_cast<OperatorToken*>(tok);
            OperatorToken* realNewTok = new OperatorToken();
            *realNewTok = *realTok;
            newTok = static_cast<Token*>(realNewTok);
            break;
        }
        case tipToken::PredefFuncToken: //no arguments to copy yet, this is very early stage
        {
            PredefFuncToken* realTok = static_cast<PredefFuncToken*>(tok);
            PredefFuncToken* realNewTok = new PredefFuncToken();
            *realNewTok = *realTok;
            newTok = static_cast<Token*>(realNewTok);
            break;
        }
        default: break; //should never get here
        }
        output.push_back(newTok);
    }
    return output;
}


eroare functie::stage3()  //normalization stage
{
    state.processStage = 3;
    eroare tmp = {ErrorType::NoError, 0, 0};


    clearList(tempTokenList);
    tempTokenList = makeNewList(rhsTokenList);

    if (rhsTokenList.size() == 0) return { ErrorType::ParseError, SyntaxError::TextLipsa, -1 };

    tmp = compressParantheses(tempTokenList);
    if(tmp.errorType!=ErrorType::NoError)
        return tmp;

    tmp = convertNormalFunctions(tempTokenList);
    if(tmp.errorType!=ErrorType::NoError)
        return tmp;

    tmp = handleFunctionCalls(tempTokenList);
    if(tmp.errorType!=ErrorType::NoError)
        return tmp;

    tmp = handleImplicitMultiplication(tempTokenList);
    if(tmp.errorType!=ErrorType::NoError)
        return tmp;

    tmp = handleDegenerateFunctions(tempTokenList);
    if(tmp.errorType!=ErrorType::NoError)
        return tmp;

    return tmp;
}

eroare functie::stage4()   //assembly stage
{
    state.processStage = 4;
    clearNode();
    eroare tmp = assemble(tempTokenList);
    if(tmp.errorType!=ErrorType::NoError)
        return tmp;

    AssembledToken& elem = *static_cast<AssembledToken*>(*tempTokenList.begin());
    headNode = elem.assembledNode;
    elem.assembledNode = nullptr; 
    return tmp;
}

eroare functie::registerAll()
{
    eroare tmp = registerFunc();
    if (tmp.errorType != ErrorType::NoError) return tmp;
    registerDependencies();
    return { ErrorType::NoError, 0, 0 };
}

eroare functie::registerFunc()
{
    if(state.funcLetter == L'x' || state.funcLetter == L'y' || state.funcLetter == 0) return {ErrorType::NoError, 0, 0};
    if(getLiteraSpreFunctie(state.funcLetter)!=NULL) return {ErrorType::ReferenceError, ReferenceError::FunctiaDejaExista, 0};
    setLiteraSpreFunctie(state.funcLetter, this);
    return { ErrorType::NoError, 0, 0 };
}

void functie::registerDependencies()
{
    for (wchar_t let : depindeDe)
        dependedOn[let].insert(this);
}

void functie::deregisterFunc()
{
    if (!state.funcRegistered) return;
    removeLiteraSpreFunctie(state.funcLetter);
}

void functie::deregisterDependencies()
{
    for (wchar_t let : depindeDe)
        dependedOn[let].erase(this);
}

void functie::deregisterAll()
{
    deregisterFunc();
    deregisterDependencies();
}

void functie::clearNode()
{
    if(headNode)
        delete headNode;
    headNode = nullptr;
}

double functie::evalFunc(double* values)
{
    if(!headNode || state.eroareFunctie.errorType!=ErrorType::NoError) return NAN;
    for(int i = 0; i < state.variables.size(); i++)
        vars[i] = { state.variables[i], values[i] };
    if(state.variables.size()>0)
        return headNode->evalNode(vars);
    if (state.wasCached) return state.cachedValue;
    state.wasCached = 1; state.cachedValue = headNode->evalNode(vars);
    return state.cachedValue;
}



void functie::initMembers()
{
    lhsTokenList.clear(); rhsTokenList.clear(); tempTokenList.clear();
    clearFunc();
}

eroare functie::process()
{
    state.wasCached = 0;
    functionState oldState = state;
    eroare tmp = { ErrorType::NoError, 0, 0 };
    switch(state.processStage)
    {
    case 0:
        tmp = stage0();
        if (tmp.errorType != ErrorType::NoError)
            break;
    case 1:
        tmp = stage1();
        if(tmp.errorType != ErrorType::NoError) 
            break;
    case 2:
        tmp = stage2();
        if(tmp.errorType != ErrorType::NoError)
            break;
    case 3:
        tmp = stage3();
        if(tmp.errorType != ErrorType::NoError)
            break;
    case 4:
        tmp = stage4();
        if(tmp.errorType != ErrorType::NoError)
            break;
    default:
        state.processStage = 5;
        break;
    }
    state.eroareFunctie = tmp;
    state.justUpdated = 1;
    updateDependants(oldState);
    return state.eroareFunctie;
}

void functie::clearFunc()
{
    deregisterAll();
    clearList(lhsTokenList);
    clearList(rhsTokenList);
    clearList(tempTokenList);
    clearNode();
    depindeDe.clear();
    state.eroareFunctie = {ErrorType::NoError, 0, 0};
    state.funcLetter = 0;
    state.processStage = state.funcRegistered = state.justUpdated = state.wasCached = 0;
    state.cachedValue = NAN;
    state.type = tipFunctie::NullFunc;
    state.variables.clear();
    state.recrrChain.clear();
}



//associativity = 0 => left  associative: a+b+c+d = ((a+b)+c)+d
//associativity = 1 => right associative: a^b^c^d = a^(b^(c^d))
eroare functie::handleBinaryOperator(std::list<Token*>& tokenList, CodOperator cod, int associativity)
{
    if (associativity)
    {
        for (auto curr = tokenList.rbegin(); curr != tokenList.rend(); ++curr)
        {
            Token& elem = **curr;
            if (elem.type != tipToken::OperatorToken) continue;
            
            OperatorToken& realElem = static_cast<OperatorToken&>(elem);
            if (realElem.cod != cod) continue;

            if (curr == tokenList.rbegin() || std::next(curr) == tokenList.rend()) return { ErrorType::ParseError, SyntaxError::OperandLipsa, elem.posInString };

            auto prevIt = std::next(curr);
            Token& prevElem = **prevIt;
            if (prevElem.type != tipToken::AssembledToken) return { ErrorType::ParseError, SyntaxError::OperandLipsa, elem.posInString };

            auto nextIt = std::next(curr, -1);
            Token& nextElem = **nextIt;
            if (nextElem.type != tipToken::AssembledToken) return { ErrorType::ParseError, SyntaxError::OperandLipsa, elem.posInString };

            AssembledToken& realPrevElem = static_cast<AssembledToken&>(prevElem);
            AssembledToken& realNextElem = static_cast<AssembledToken&>(nextElem);

            OperatorNode* newNode = new OperatorNode(cod, 2);
            newNode->funcptr = getFunctionFromCode(cod);
            newNode->argv[0] = realPrevElem.assembledNode;
            newNode->argv[1] = realNextElem.assembledNode;
            realPrevElem.assembledNode = realNextElem.assembledNode = nullptr;

            delete *prevIt; 
            delete *nextIt;
            tokenList.erase(std::next(prevIt).base()); 
            tokenList.erase(std::next(nextIt).base());  //oof...https://stackoverflow.com/questions/1830158/how-to-call-erase-with-a-reverse-iterator
            curr = nextIt;
            AssembledToken* newToken = new AssembledToken(newNode, elem.posInString);

            delete *curr;
            *curr = static_cast<Token*>(newToken);
        }
    }
    else
    { 
        for (auto curr = tokenList.begin(); curr != tokenList.end(); ++curr)
        {
            Token& elem = **curr;
            if (elem.type != tipToken::OperatorToken) continue;

            OperatorToken& realElem = static_cast<OperatorToken&>(elem);
            if (realElem.cod != cod) continue;

            if (curr == tokenList.begin() || std::next(curr) == tokenList.end()) return { ErrorType::ParseError, SyntaxError::OperandLipsa, elem.posInString };

            auto prevIt = std::next(curr, -1);
            Token& prevElem = **prevIt;
            if (prevElem.type != tipToken::AssembledToken) return { ErrorType::ParseError, SyntaxError::OperandLipsa, elem.posInString };

            auto nextIt = std::next(curr);
            Token& nextElem = **nextIt;
            if (nextElem.type != tipToken::AssembledToken) return { ErrorType::ParseError, SyntaxError::OperandLipsa, elem.posInString };

            AssembledToken& realPrevElem = static_cast<AssembledToken&>(prevElem);
            AssembledToken& realNextElem = static_cast<AssembledToken&>(nextElem);

            OperatorNode* newNode = new OperatorNode(cod, 2);
            newNode->funcptr = getFunctionFromCode(cod);
            newNode->argv[0] = realPrevElem.assembledNode;
            newNode->argv[1] = realNextElem.assembledNode;
            realPrevElem.assembledNode = realNextElem.assembledNode = nullptr;

            delete* prevIt; delete* nextIt;
            tokenList.erase(prevIt); tokenList.erase(nextIt); 

            AssembledToken* newToken = new AssembledToken(newNode, elem.posInString);

            delete* curr;
            *curr = static_cast<Token*>(newToken);
        }
    }
    return { ErrorType::NoError, 0, 0 };
}

//Special case: these two have the same priority, with right - left associativity
//a^-b = a^(-b)
//-a^b = -(a^b)
//-a^-b = -(a^(-b))
//a^-b^-c = a^(-(b^(-c)))
eroare functie::handleExpAndUnaryMinus(std::list<Token*>& tokenList)
{
    for (auto curr = tokenList.rbegin(); curr != tokenList.rend(); ++curr)
    {
        Token& elem = **curr;
        if (elem.type != tipToken::OperatorToken) continue;

        OperatorToken& realElem = static_cast<OperatorToken&>(elem);
        if (realElem.cod != CodOperator::subtract && realElem.cod != CodOperator::pow) continue;

        if (curr == tokenList.rbegin()) return { ErrorType::ParseError, SyntaxError::OperandLipsa, elem.posInString };

        auto nextIt = std::next(curr, -1);
        Token& nextElem = **nextIt;
        if (nextElem.type != tipToken::AssembledToken) return { ErrorType::ParseError, SyntaxError::OperandLipsa, elem.posInString };

        if (realElem.cod == CodOperator::subtract)
        {
            auto prevIt = std::next(curr);
            if (prevIt != tokenList.rend())
            {
                Token& prevElem = **prevIt;
                if (prevElem.type == tipToken::AssembledToken) continue; //probably normal addition/subtraction
            }
            AssembledToken& realNextElem = static_cast<AssembledToken&>(nextElem);
            OperatorNode* newNode = new OperatorNode(CodOperator::unaryMinus, 1);
            newNode->funcptr = getFunctionFromCode(CodOperator::unaryMinus);
            newNode->argv[0] = realNextElem.assembledNode;
            realNextElem.assembledNode = static_cast<Node*>(newNode);

            delete* curr;
            tokenList.erase(std::next(curr).base());
            curr = nextIt;
        }
        else
        {
            if (std::next(curr) == tokenList.rend()) return { ErrorType::ParseError, SyntaxError::OperandLipsa, elem.posInString };

            auto prevIt = std::next(curr);
            Token& prevElem = **prevIt;
            if (prevElem.type != tipToken::AssembledToken) return { ErrorType::ParseError, SyntaxError::OperandLipsa, elem.posInString };

            AssembledToken& realPrevElem = static_cast<AssembledToken&>(prevElem);
            AssembledToken& realNextElem = static_cast<AssembledToken&>(nextElem);

            OperatorNode* newNode = new OperatorNode(CodOperator::pow, 2);
            newNode->funcptr = getFunctionFromCode(CodOperator::pow);
            newNode->argv[0] = realPrevElem.assembledNode;
            newNode->argv[1] = realNextElem.assembledNode;
            realPrevElem.assembledNode = realNextElem.assembledNode = nullptr;

            delete* prevIt;
            delete* nextIt;
            tokenList.erase(std::next(prevIt).base());
            tokenList.erase(std::next(nextIt).base()); 
            curr = nextIt;

            AssembledToken* newToken = new AssembledToken(newNode, elem.posInString);
            delete* curr;
            *curr = static_cast<Token*>(newToken);
        }
    }
    return { ErrorType::NoError, 0, 0 };
}

eroare functie::handleSameTierBinaryOperators(std::list<Token*>& tokenList, CodOperator cod1, CodOperator cod2)
{
    for (auto curr = tokenList.begin(); curr != tokenList.end(); ++curr)
    {
        Token& elem = **curr;
        if (elem.type != tipToken::OperatorToken) continue;

        OperatorToken& realElem = static_cast<OperatorToken&>(elem);
        if (realElem.cod != cod1 && realElem.cod != cod2) continue;

        if (curr == tokenList.begin() || std::next(curr) == tokenList.end()) return { ErrorType::ParseError, SyntaxError::OperandLipsa, elem.posInString };

        auto prevIt = std::next(curr, -1);
        Token& prevElem = **prevIt;
        if (prevElem.type != tipToken::AssembledToken) return { ErrorType::ParseError, SyntaxError::OperandLipsa, elem.posInString };

        auto nextIt = std::next(curr);
        Token& nextElem = **nextIt;
        if (nextElem.type != tipToken::AssembledToken) return { ErrorType::ParseError, SyntaxError::OperandLipsa, elem.posInString };

        AssembledToken& realPrevElem = static_cast<AssembledToken&>(prevElem);
        AssembledToken& realNextElem = static_cast<AssembledToken&>(nextElem);

        OperatorNode* newNode = new OperatorNode(realElem.cod, 2);
        newNode->funcptr = getFunctionFromCode(realElem.cod);
        newNode->argv[0] = realPrevElem.assembledNode;
        newNode->argv[1] = realNextElem.assembledNode;
        realPrevElem.assembledNode = realNextElem.assembledNode = nullptr;

        delete* prevIt; delete* nextIt;
        tokenList.erase(prevIt); tokenList.erase(nextIt);

        AssembledToken* newToken = new AssembledToken(newNode, elem.posInString);

        delete* curr;
        *curr = static_cast<Token*>(newToken);
    }
    return { ErrorType::NoError, 0, 0 };
}


eroare functie::assemble(std::list<Token*>& tokenList)
{
    eroare tmp;
    //assemble parantheses
    //assemble letters and numbers
    //assemble (predefined functions)
    //test for commas
    for (auto curr = tokenList.begin(); curr != tokenList.end(); ++curr)
    {
        Token& elem = **curr;
        if (elem.type == tipToken::OperatorToken)
        {
            OperatorToken& opTok = static_cast<OperatorToken&>(elem);

            if (opTok.cod == CodOperator::comma) return { ErrorType::ParseError, SyntaxError::SintaxaIncorecta, elem.posInString };
            continue;
        }

        //auto nextIt = std::next(curr);
        Node* newNode = nullptr;

        switch (elem.type)
        {
        case tipToken::LetterToken:
        {
            LetterToken& realElem = static_cast<LetterToken&>(elem);

            LetterNode* letNode = new LetterNode(realElem.letter);

            newNode = static_cast<Node*>(letNode);
            break;
        }
        case tipToken::NumberToken:
        {
            NumberToken& realElem = static_cast<NumberToken&>(elem);

            NumberNode* numNode = new NumberNode(realElem.value);

            newNode = static_cast<Node*>(numNode);
            break;
        }
        case tipToken::ParansToken:
        {
            ParansToken& realElem = static_cast<ParansToken&>(elem);
            tmp = assemble(realElem.innerTokenList);
            if (tmp.errorType != ErrorType::NoError) return tmp;
            AssembledToken& innerAssToken = *static_cast<AssembledToken*>(*realElem.innerTokenList.begin());

            newNode = static_cast<Node*>(innerAssToken.assembledNode);
            innerAssToken.assembledNode = nullptr;
            break;
        }
        case tipToken::FunctionToken:
        {
            FunctionToken& realElem = static_cast<FunctionToken&>(elem);

            FunctionNode* funcNode = new FunctionNode(realElem.letter, realElem.args.argv.size());
            int i = 0;
            for(auto& l : realElem.args.argv)
            {
                tmp = assemble(l);
                if (tmp.errorType != ErrorType::NoError) return tmp;
                AssembledToken& innerAssToken = *static_cast<AssembledToken*>(*l.begin());

                funcNode->argv[i] = static_cast<Node*>(innerAssToken.assembledNode);
                i++;
                innerAssToken.assembledNode = nullptr;
            }

            newNode = static_cast<Node*>(funcNode);
            break;

        }
        case tipToken::PredefFuncToken:
        {
            PredefFuncToken& realElem = static_cast<PredefFuncToken&>(elem);

            PredefFuncNode* predefFuncNode = new PredefFuncNode(realElem.codFunc, realElem.args.argv.size());
            int i = 0;

            predefFuncNode->funcptr = getFunctionFromCode(realElem.codFunc);

            for (auto& l : realElem.args.argv)
            {
                tmp = assemble(l);
                if (tmp.errorType != ErrorType::NoError) return tmp;
                AssembledToken& innerAssToken = *static_cast<AssembledToken*>(*l.begin());

                predefFuncNode->argv[i] = static_cast<Node*>(innerAssToken.assembledNode);
                i++;
                innerAssToken.assembledNode = nullptr;
            }

            newNode = static_cast<Node*>(predefFuncNode);
            break;
        }
        default:
            return { ErrorType::ParseError, SyntaxError::Unexpected, elem.posInString }; // should never get here
        }

        AssembledToken* assTok = new AssembledToken(newNode, elem.posInString);

        delete (*curr);
        *curr = static_cast<Token*>(assTok);
    }

    if (tokenList.size() == 1 && (**tokenList.begin()).type == tipToken::AssembledToken)
        return { ErrorType::NoError, 0, 0 };

    //handle operators:
    
    //handle implicit multiplication (with caveats as mentioned in handleImplicitMultiplication)
    tmp = handleBinaryOperator(tokenList, CodOperator::implicitMult, 0);
    if (tmp.errorType != ErrorType::NoError) return tmp;

    if (tokenList.size() == 1 && (**tokenList.begin()).type == tipToken::AssembledToken)
        return { ErrorType::NoError, 0, 0 };

    //handle factorial
    for (auto curr = tokenList.begin(); curr!=tokenList.end(); ++curr)
    {
        Token& elem = **curr;
        if (elem.type != tipToken::OperatorToken) continue;

        OperatorToken& opTok = static_cast<OperatorToken&>(elem);

        if (opTok.cod != CodOperator::factorial) continue;

        if(curr==tokenList.begin()) return { ErrorType::ParseError, SyntaxError::OperandLipsa, elem.posInString };

        auto prevIt = std::next(curr, -1);
        Token& prevElem = **prevIt;

        if (prevElem.type != tipToken::AssembledToken) return { ErrorType::ParseError, SyntaxError::OperandLipsa, elem.posInString };

        AssembledToken& realPrevElem = static_cast<AssembledToken&>(prevElem);

        OperatorNode* opNode = new OperatorNode(CodOperator::factorial, 1);
        opNode->funcptr = getFunctionFromCode(CodOperator::factorial);
        opNode->argv[0] = realPrevElem.assembledNode;
        realPrevElem.assembledNode = nullptr;

        AssembledToken* newTok = new AssembledToken(static_cast<Node*>(opNode), elem.posInString);
        //auto nextIt = std::next(curr);

        delete *prevIt;
        tokenList.erase(prevIt);
        
        //tokenList.insert(curr, static_cast<Token*>(newTok));
        delete *curr;
        *curr = static_cast<Token*>(newTok);
    }

    if (tokenList.size() == 1 && (**tokenList.begin()).type == tipToken::AssembledToken)
        return { ErrorType::NoError, 0, 0 };

    //handle unary + and compress unary +- but unary- handling takes place later
    for (auto curr = tokenList.rbegin(); curr != tokenList.rend(); ++curr)
    {
        Token& elem = **curr;
        if (elem.type != tipToken::OperatorToken) continue;
        
        OperatorToken& opTok = static_cast<OperatorToken&>(elem);
        if (opTok.cod != CodOperator::add && opTok.cod != CodOperator::subtract) continue;
        if (curr == tokenList.rbegin()) return { ErrorType::ParseError, SyntaxError::OperandLipsa, elem.posInString };

        auto prevIt = std::next(curr);
        auto nextIt = std::next(curr, -1);
        if (prevIt != tokenList.rend())
        {
            Token& prevElem = **prevIt;
            if (prevElem.type == tipToken::AssembledToken) continue; //probably normal addition/subtraction

            
            if(opTok.cod==CodOperator::add)
            {
                //always remove myself
                delete *curr;
                tokenList.erase(std::next(curr).base());
                curr = nextIt;
                continue;
            }

            OperatorToken& prevOpTok = static_cast<OperatorToken&>(prevElem);
            switch (prevOpTok.cod)
            {
            case CodOperator::add:
            {
                //always remove it
                delete *prevIt;
                tokenList.erase(std::next(prevIt).base());
                curr = nextIt;
                continue;
            }
            case CodOperator::subtract:
            {
                
                //remove it
                delete *prevIt;
                tokenList.erase(std::next(prevIt).base());
                //change myself to unaryPlus (but with the add code because tokenisation doesn't recognise unary plus)
                opTok.cod = CodOperator::add;
                curr = nextIt;
                continue;
            }
            default:; //fall through to normal behaviour
            }
        }

        if (opTok.cod == CodOperator::add)
        {
            //remove myself
            delete* curr;
            tokenList.erase(std::next(curr).base());
            curr = nextIt;
            continue; //this will exit anyways
        }
       
        //finish handling in next function...
        
    }

    if (tokenList.size() == 1 && (**tokenList.begin()).type == tipToken::AssembledToken)
        return { ErrorType::NoError, 0, 0 };

    tmp = handleExpAndUnaryMinus(tokenList);
    if (tmp.errorType != ErrorType::NoError) return tmp;

    if (tokenList.size() == 1 && (**tokenList.begin()).type == tipToken::AssembledToken)
        return { ErrorType::NoError, 0, 0 };

    
    //handle multiplication and division
    tmp = handleSameTierBinaryOperators(tokenList, CodOperator::mult, CodOperator::div);
    if (tmp.errorType != ErrorType::NoError) return tmp;

    if (tokenList.size() == 1 && (**tokenList.begin()).type == tipToken::AssembledToken)
        return { ErrorType::NoError, 0, 0 };

    //handle addition and subtraction
    tmp = handleSameTierBinaryOperators(tokenList, CodOperator::add, CodOperator::subtract);
    if (tmp.errorType != ErrorType::NoError) return tmp;

    return {ErrorType::NoError, 0, 0}; 
}

void functie::updateDependants(const functionState& oldState)
{
    if(state.eroareFunctie.errorValue == ReferenceError::DependentaRecursiva&&state.eroareFunctie.errorType==ErrorType::ReferenceError)
    {
        for(functie* ptr : dependedOn[state.funcLetter])
            if(!state.recrrChain.contains(ptr->state.funcLetter))
                ptr->updateFromDependency(oldState, state);
        return;
    }
    if(state.funcLetter!=L'x'&&state.funcLetter!=L'y'&&state.funcLetter!=0)
        for(functie* ptr : dependedOn[state.funcLetter])
            ptr->updateFromDependency(oldState, state);


    if (oldState.funcLetter != state.funcLetter && oldState.funcLetter != 0 && oldState.funcLetter != L'x' && oldState.funcLetter != L'y')
    {
        auto dependedCopy = dependedOn[oldState.funcLetter]; //This is because one of these may be a function with the same name, waiting for me to get deleted, at which point it deletes itself from my dependencies which breaks the iteration.
        for (functie* ptr : dependedCopy)
            if(ptr!=this)
                ptr->updateFromDependency(oldState, state);
    }
        
}



void functie::updateFromDependency(const functionState& oldState, const functionState& newState){
    //this means my name was taken and it possibly disappeared so try again
    //maybe should separate tokenization into actual stage 0 and make registering its own stage
    if (state.eroareFunctie.errorType == ErrorType::ReferenceError && state.eroareFunctie.errorValue == ReferenceError::FunctiaDejaExista)
    {
        state.processStage = 0;
        process();
        return;
    }

    if((oldState.eroareFunctie.errorType!=ErrorType::NoError&&newState.eroareFunctie.errorType==ErrorType::NoError) || (oldState.type==tipFunctie::NullFunc))
    {
        state.processStage = 1;
        process();
        return;
    }

    if (newState.eroareFunctie.errorType != ErrorType::NoError)
    {
        state.processStage = 1;
        process();
        return;
    }

    if(newState.funcLetter != oldState.funcLetter)
    {
        state.processStage = 1;
        process();
        return;
    }
    

    if(newState.type != oldState.type)
    {
        state.processStage = 2;
        process();
        return;
    }

    //even though the variable lists could differ in order for more variables which would normally not necessitate an update, a degenerate func only has 1 variable so it always means the variable has changed which needs updating
    //also it actually would necessitate an update but to stage 3 not 2
    if(newState.type == tipFunctie::DegenerateFunc && oldState.variables != newState.variables)
    {
        state.processStage = 2;
        process();
        return;
    }
    state.wasCached = 0;
    state.justUpdated = 1;
    updateDependants(state);
}

eroare functie::updateString(const std::wstring& str)
{
    funcstr = str;
    state.processStage = 0;
    return process();
}



eroare functie::compressParantheses(std::list<Token*>& tokenList)
{
    std::list<Token*>::iterator leftPar, rightPar;
    auto curr = tokenList.begin();

    std::stack<std::list<Token*>::iterator> leftParStack;

    while(curr != tokenList.end())
    {
        Token& elem = **curr;
        if (elem.type != tipToken::ParanToken) 
        {
            ++curr; continue;
        }
        ParanToken& realElem = static_cast<ParanToken&>(elem);
        auto nextIt = std::next(curr);
        if(realElem.open) //leftPar
        {
            leftParStack.push(curr);
        }
        else //rightPar
        {
            rightPar = curr;
            if(leftParStack.size()==0)
                return {ErrorType::ParseError, SyntaxError::ParantezeIncorecte, realElem.posInString};
            leftPar = leftParStack.top();
            leftParStack.pop();
            if(std::next(leftPar)==rightPar)
                return {ErrorType::ParseError, SyntaxError::ParantezeGoale, realElem.posInString};
            rightPar = curr;
            ParansToken* tok = new ParansToken();
            tok->posInString = (*leftPar)->posInString;
            tok->innerTokenList.splice(tok->innerTokenList.end(), tokenList, std::next(leftPar), rightPar);

            tokenList.insert(leftPar, static_cast<Token*>(tok));
            delete *leftPar; delete *rightPar;
            tokenList.erase(leftPar);
            tokenList.erase(rightPar);

            eroare tmp = compressParantheses(tok->innerTokenList);
            if(tmp.errorType!=ErrorType::NoError) return tmp;
        }
        curr = nextIt;
    }

    if(leftParStack.size()>0)
    {
        return {ErrorType::ParseError, SyntaxError::ParantezeIncorecte, (*leftParStack.top())->posInString};
    }

    return {ErrorType::NoError, 0, 0};
}


eroare functie::handleImplicitMultiplication(std::list<Token*>& tokenList)
{
    for(auto curr = tokenList.begin(); curr != tokenList.end(); ++curr)
    {
        Token& elem = **curr;
        if (elem.type == tipToken::OperatorToken)
        {
            OperatorToken& realElem = static_cast<OperatorToken&>(elem);
            if (realElem.cod != CodOperator::factorial) continue;
        }

        if(elem.type==tipToken::ParansToken)
        {
            ParansToken& realElem = static_cast<ParansToken&>(elem);
            handleImplicitMultiplication(realElem.innerTokenList);
        }
        else if (elem.type == tipToken::FunctionToken)
        {
            FunctionToken& realElem = static_cast<FunctionToken&>(elem);
            for (auto& l : realElem.args.argv)
                handleImplicitMultiplication(l);
        }
        else if (elem.type == tipToken::PredefFuncToken)
        {
            PredefFuncToken& realElem = static_cast<PredefFuncToken&>(elem);
            for (auto& l : realElem.args.argv)
                handleImplicitMultiplication(l);
        }

        if (std::next(curr) == tokenList.end()) break;

        Token& nextElem = **std::next(curr);
        if(nextElem.type==tipToken::OperatorToken) continue;
        
        int cond = 0;
        OperatorToken* newTok;
        if (std::next(std::next(curr)) != tokenList.end())
        {
            Token& nextNextElem = **std::next(std::next(curr));
            if (nextNextElem.type == tipToken::OperatorToken)
            {
                OperatorToken& realNextNextElem = static_cast<OperatorToken&>(nextNextElem);
                if (realNextNextElem.cod == CodOperator::pow)
                    cond = 1; //precedence issue with ab^c : you want a * b^c but reordering operations would imply other issues so add normal multiplication
            }
        }
        if (elem.type != tipToken::OperatorToken && !cond)  //precedence issue with ! and implicit mult: abc! is (abc)! and a!b! is a! *  b! but handleBinaryOperator expects only assembledNodes on both sides so order is implicit mult -> factorial -> implicit mult....
            newTok = new OperatorToken(CodOperator::implicitMult, 1, elem.posInString);
        else
            newTok = new OperatorToken(CodOperator::mult, 0, elem.posInString);
        tokenList.insert(std::next(curr), newTok);
        ++curr;
    }
    return {ErrorType::NoError, 0, 0};
}

eroare functie::handleFunctionCalls(std::list<Token*>& tokenList)
{
    //group predef func arguments
    for(auto curr = tokenList.begin(); curr != tokenList.end(); ++curr)
    {
        Token& elem = **curr;
        if(elem.type!=tipToken::PredefFuncToken) continue;

        auto startIt = std::next(curr);
        if(startIt == tokenList.end())
            return {ErrorType::ParseError, SyntaxError::ArgumentLipsa, elem.posInString};
        if ((**startIt).type == tipToken::ParansToken) continue; //normal function call
        auto tempIt = startIt;
        for(; tempIt!=tokenList.end(); ++tempIt)
        {
            Token& elem = **tempIt;
            if(elem.type==tipToken::OperatorToken) break;
            if(elem.type==tipToken::FunctionToken || elem.type==tipToken::PredefFuncToken || elem.type==tipToken::ParansToken)
                return {ErrorType::ParseError, SyntaxError::ApelAmbiguu, elem.posInString};
        }
        if (tempIt == startIt) return { ErrorType::ParseError, SyntaxError::ArgumentLipsa, elem.posInString };
        ParansToken* parTok = new ParansToken();
        parTok->posInString = (*startIt)->posInString;
        parTok->innerTokenList.clear();

        parTok->innerTokenList.splice(parTok->innerTokenList.end(), tokenList, startIt, tempIt);
        tokenList.insert(std::next(curr), parTok);
    }


    //connect func arguments to func
    for(auto curr = tokenList.begin(); curr!=tokenList.end(); ++curr)
    {
        Token& elem = **curr;
        if(elem.type==tipToken::ParansToken)
        {
            ParansToken& realElem = static_cast<ParansToken&>(elem);
            eroare tmp = handleFunctionCalls(realElem.innerTokenList);
            if(tmp.errorType!=ErrorType::NoError)
                return tmp;
            continue;
        }
        if(elem.type!=tipToken::PredefFuncToken && elem.type!=tipToken::FunctionToken) continue;

        auto nextIt = std::next(curr);
        if(nextIt==tokenList.end())
            return {ErrorType::ParseError, SyntaxError::ArgumentLipsa, elem.posInString};

        Token& nextElem = **nextIt;
        if(nextElem.type!=tipToken::ParansToken&&nextElem.type!=tipToken::LetterToken&&nextElem.type!=tipToken::NumberToken)
            return {ErrorType::ParseError, SyntaxError::ArgumentLipsa, elem.posInString};
        if(nextElem.type!=tipToken::ParansToken)
        {
            if(elem.type==tipToken::FunctionToken)
                return {ErrorType::ParseError, SyntaxError::ArgumentLipsa, elem.posInString};

            PredefFuncToken& realElem = static_cast<PredefFuncToken&>(elem);
            realElem.args.argv.push_back(std::list<Token*>());
            realElem.args.argv[0].splice(realElem.args.argv[0].end(), tokenList, nextIt);
        }
        else if(elem.type==tipToken::FunctionToken)
        {
            FunctionToken& realElem = static_cast<FunctionToken&>(elem);
            eroare tmp = handleArgumentList(static_cast<ParansToken*>(&nextElem), realElem.args.argv);
            if(tmp.errorType!=ErrorType::NoError) return tmp;
            delete *nextIt;
            tokenList.erase(nextIt);

            functie* ptr = getLiteraSpreFunctie(realElem.letter);
            if(ptr->state.variables.size()<realElem.args.argv.size())
                return {ErrorType::ParseError, SyntaxError::ArgumenteMulte, realElem.posInString};
            else if(ptr->state.variables.size()>realElem.args.argv.size())
                return {ErrorType::ParseError, SyntaxError::ArgumentePutine, realElem.posInString};

            for(auto& l : realElem.args.argv)
            {
                tmp = handleFunctionCalls(l);
                if(tmp.errorType!=ErrorType::NoError) return tmp;
            }
        }
        else
        {
            PredefFuncToken& realElem = static_cast<PredefFuncToken&>(elem);
            eroare tmp = handleArgumentList(static_cast<ParansToken*>(&nextElem), realElem.args.argv);
            if(tmp.errorType!=ErrorType::NoError) return tmp;
            delete *nextIt;
            tokenList.erase(nextIt);

            long long unsigned int numvar = getNumberOfArgs(realElem.codFunc);
            if (numvar<realElem.args.argv.size())
                return {ErrorType::ParseError, SyntaxError::ArgumenteMulte, realElem.posInString};
            else if(numvar>realElem.args.argv.size())
                return {ErrorType::ParseError, SyntaxError::ArgumentePutine, realElem.posInString};

            for(auto& l : realElem.args.argv)
            {
                tmp = handleFunctionCalls(l);
                if(tmp.errorType!=ErrorType::NoError) return tmp;
            }
        }
    }
    return {ErrorType::NoError, 0, 0};
}

eroare functie::handleArgumentList(ParansToken* parTok, std::vector<std::list<Token*>>& output)
{
    int index = 0;
    auto st = parTok->innerTokenList.begin();

    if((*(parTok->innerTokenList.begin()))->type==tipToken::OperatorToken)
    {
        OperatorToken& realElem = *static_cast<OperatorToken*>(*(parTok->innerTokenList.begin()));
        if(realElem.cod==CodOperator::comma)
            return {ErrorType::ParseError, SyntaxError::ArgumentGol, 0};
    }

    int metComma = 0;
    for(auto curr = std::next(st); curr!=parTok->innerTokenList.end(); ++curr)
    {
        if (metComma)
        {
            metComma = 0;
            --curr;
        }
        Token& elem = **curr;
        if(elem.type!=tipToken::OperatorToken) continue;

        OperatorToken& realElem = static_cast<OperatorToken&>(elem);
        if(realElem.cod!=CodOperator::comma) continue;

        output.push_back(std::list<Token*>());

        if(st==curr)
            return {ErrorType::ParseError, SyntaxError::ArgumentGol, realElem.posInString};

        output[index].splice(output[index].end(), parTok->innerTokenList, st, curr);
        index++;
        
        st = std::next(curr);

        delete *curr;
        parTok->innerTokenList.erase(curr);
        if (st == parTok->innerTokenList.end()) break;
        curr = st; metComma = 1;
    }
    output.push_back(std::list<Token*>());
    if(st==parTok->innerTokenList.end())
        return {ErrorType::ParseError, SyntaxError::ArgumentGol, parTok->posInString};
    output[index].splice(output[index].end(), parTok->innerTokenList, st, parTok->innerTokenList.end());

    return {ErrorType::NoError, 0, 0};
}


eroare functie::handleDegenerateFunctions(std::list<Token*>& tokenList)
{
    eroare tmp;
    for(auto curr = tokenList.begin(); curr != tokenList.end(); ++curr)
    {
        Token& elem = **curr;
        if(elem.type==tipToken::ParansToken)
        {
            ParansToken& realElem = static_cast<ParansToken&>(elem);
            tmp = handleDegenerateFunctions(realElem.innerTokenList);
            if(tmp.errorType!=ErrorType::NoError) return tmp;
            continue;
        }
        else if(elem.type==tipToken::FunctionToken)
        {
            FunctionToken& realElem = static_cast<FunctionToken&>(elem);
            for(auto& l : realElem.args.argv)
            {
                tmp = handleDegenerateFunctions(l);
                if(tmp.errorType!=ErrorType::NoError) return tmp;
            }
        }
        else if (elem.type == tipToken::PredefFuncToken)
        {
            PredefFuncToken& realElem = static_cast<PredefFuncToken&>(elem);
            for (auto& l : realElem.args.argv)
            {
                tmp = handleDegenerateFunctions(l);
                if (tmp.errorType != ErrorType::NoError) return tmp;
            }
        }
        if(elem.type!=tipToken::LetterToken) continue;

        LetterToken& realElem = static_cast<LetterToken&>(elem);

        int cond = 0;
        for(wchar_t let : state.variables)
            if(let == realElem.letter) {cond = 1; break;}
        if(cond) continue;

        functie* funcptr = getLiteraSpreFunctie(realElem.letter);

        if(funcptr->state.type!=tipFunctie::DegenerateFunc) continue; //how?

        int pos = elem.posInString;
        FunctionToken* newTok = new FunctionToken(realElem.letter, pos);

        delete *curr;
        *curr = static_cast<Token*>(newTok);

        if (funcptr->state.variables.size() == 0) continue;
        wchar_t var = funcptr->state.variables[0];

        LetterToken* varTok = new LetterToken(var, pos);

        newTok->args.argv.push_back(std::list<Token*>(1, varTok));
    }
    return {ErrorType::NoError, 0, 0};
}

eroare functie::convertNormalFunctions(std::list<Token*>& tokenList)
{
    eroare tmp;
    for(auto curr = tokenList.begin(); curr != tokenList.end(); ++curr)
    {
        Token& elem = **curr;
        if(elem.type==tipToken::ParansToken)
        {
            ParansToken& realElem = static_cast<ParansToken&>(elem);
            tmp = convertNormalFunctions(realElem.innerTokenList);
            if(tmp.errorType!=ErrorType::NoError) return tmp;
            continue;
        }
        if(elem.type!=tipToken::LetterToken) continue;

        LetterToken& realElem = static_cast<LetterToken&>(elem);

        int cond = 0;
        for(wchar_t let : state.variables)
            if(let == realElem.letter) {cond = 1; break;}
        if(cond) continue;

        functie* funcptr = getLiteraSpreFunctie(realElem.letter);

        if(funcptr->state.type!=tipFunctie::NormalFunc) continue;

        int pos = elem.posInString;
        FunctionToken* newTok = new FunctionToken(realElem.letter, pos);

        delete *curr;
        *curr = static_cast<Token*>(newTok);


        //check if it has arguments here, but they will be properly handled in handleFunctionCalls()
        auto nextIt = std::next(curr);
        if(nextIt==tokenList.end())
            return {ErrorType::ParseError, SyntaxError::ArgumentLipsa, pos};
        Token& nextElem = **nextIt;

        if(nextElem.type!=tipToken::ParansToken)
            return {ErrorType::ParseError, SyntaxError::ArgumentLipsa, pos};
    }
    return {ErrorType::NoError, 0, 0};
}

void functie::updateDependantsAfterRecurrCheck(const std::set<wchar_t>& reccrStack)
{
    functionState oldState = state;
    state.eroareFunctie = {ErrorType::ReferenceError, ReferenceError::DependentaRecursiva, 0};
    state.recrrChain = reccrStack;

    updateDependants(oldState);
}

int functie::checkRecurrency(std::set<wchar_t>& reccrStack)
{
    if (state.eroareFunctie.errorType == ErrorType::ReferenceError && state.eroareFunctie.errorValue == ReferenceError::DependentaRecursiva) return 0;
    for(wchar_t let : depindeDe)
    {
        functie* ptr = getLiteraSpreFunctie(let);
        if(ptr==NULL) continue;
        if (reccrStack.contains(let)) return -1;
        reccrStack.insert(let);
        int rez = ptr->checkRecurrency(reccrStack);
        if(rez==-1) return -1;
        reccrStack.erase(let);
    }
    return 0;
}

bool functie::isSpecialFunction()
{
    if (state.variables.size() > 1) return 0;
    if (state.variables.size() == 0)
    {
        if (state.funcLetter == L'x' || state.funcLetter == L'y') return 1;
        else return 0;
    }
    return 0;
}

void initConstante() //looks like a memory leak (except it isn't since the pointer is kept in litSpFct) but these are supposed to last for the entire program so it's fine
{
    new functie(L"e=2.71828182845904523536"); 
    new functie(L"π=3.14159265358979323846");
}
