#include "mate.h"

double FunctionNode::evalNode(const varAssignment* vars)
{
    functie* funcptr = getLiteraSpreFunctie(funcLetter);
    for(int i = 0; i<argc; i++)
    {
        values[i] = argv[i]->evalNode(vars);
    }
    return funcptr->evalFunc(values);
}



double LetterNode::evalNode(const varAssignment* vars)
{
    for(auto i = 0; i < 100; i++)
        if(vars[i].varLetter == letter)
            return vars[i].value;

    //should never get here
    return NAN;
}


double NumberNode::evalNode(const varAssignment* vars)
{
    return value;
}


double PredefFuncNode::evalNode(const varAssignment* vars)
{
    for(auto i = 0; i<argc; i++)
    {
        values[i] = argv[i]->evalNode(vars);
    }

    return funcptr(values);
}


double OperatorNode::evalNode(const varAssignment* vars)
{
    for(auto i = 0ull; i<argc; i++)
    {
        values[i] = argv[i]->evalNode(vars);
    }

    return funcptr(values);
}





FunctionToken::~FunctionToken()
{
    for(auto& l : args.argv)
        clearList(l);
    args.argv.clear();
}

PredefFuncToken::~PredefFuncToken()
{
    for(auto& l : args.argv)
        clearList(l);
    args.argv.clear();
}

ParansToken::~ParansToken()
{
    clearList(innerTokenList);
}

AssembledToken::~AssembledToken()
{
    if(assembledNode)
        delete assembledNode;
}

