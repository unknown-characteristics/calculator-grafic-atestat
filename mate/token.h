#ifndef __token_header
#define __token_header

#include <math.h>
#include <list>
#include <vector>
#include "predef_functii.h"
typedef double (*math_func_ptr)(double*);

enum class tipToken
{
    NullToken = 0,
    NumberToken,
    LetterToken,
    FunctionToken,
    ParanToken,
    ParansToken,
    OperatorToken,
    PredefFuncToken,
    AssembledToken
};

enum class tipNode
{
    NullNode = 0,
    NumberNode,
    FunctionNode,
    LetterNode,
    PredefFuncNode,
    OperatorNode
};



template<class T>
class ArgumentList
{
public:
    std::vector<T> argv;

    ArgumentList() {}
    ArgumentList(const std::vector<T>& vec) : argv(vec) {}
};


struct varAssignment
{
    wchar_t varLetter;
    double value;

    varAssignment() : varAssignment(0, NAN) {}
    varAssignment(wchar_t let, double val) : varLetter(let), value(val) {}
};

///////////////////////////////////////////////////////////////////////////
//////////NODES
///////////////////////////////////////////////////////////////////////////


class Node
{
public:
    tipNode tip;

    virtual double evalNode(const varAssignment* vars) = 0;

    Node() : Node(tipNode::NullNode) {}
    Node(tipNode tip) : tip(tip) {}

    virtual ~Node() {}
};
typedef Node* nodeptr;
class OperatorNode : public Node
{
public:
    CodOperator cod;
    nodeptr* argv = NULL;
    double* values = NULL;
    int argc;
    math_func_ptr funcptr = nullptr;

    double evalNode(const varAssignment* vars) override;

    OperatorNode() : OperatorNode(CodOperator::noop, 0) {}
    OperatorNode(CodOperator cod, int argc) : Node(tipNode::OperatorNode), cod(cod), argc(argc)
    {
        argv = new nodeptr[argc];
        values = new double[argc];
    }

    ~OperatorNode()
    {
        delete[argc] argv;
        delete[argc] values;
    }
};

class FunctionNode : public Node
{
public:
    wchar_t funcLetter;
    nodeptr* argv = NULL;
    double* values = NULL;
    int argc;

    double evalNode(const varAssignment* vars) override;

    FunctionNode() : FunctionNode(0, 0) {}
    FunctionNode(wchar_t let, int argc) : Node(tipNode::FunctionNode), funcLetter(let), argc(argc)
    {
        argv = new nodeptr[argc];
        values = new double[argc];
    }

    ~FunctionNode()
    {
        delete[argc] argv;
        delete[argc] values;
    }
};

class PredefFuncNode : public Node
{
public:
    CodFunctiePredefinita cod;
    nodeptr* argv = NULL;
    double* values = NULL;
    int argc;
    math_func_ptr funcptr = nullptr;

    double evalNode(const varAssignment* vars) override;

    PredefFuncNode() : PredefFuncNode(CodFunctiePredefinita::notPredefined, 0) {}
    PredefFuncNode(CodFunctiePredefinita cod, int argc) : Node(tipNode::PredefFuncNode), cod(cod), argc(argc)
    {
        argv = new nodeptr[argc];
        values = new double[argc];
    }

    ~PredefFuncNode()
    {
        delete[argc] argv;
        delete[argc] values;
    }
};

class LetterNode : public Node
{
public:
    wchar_t letter;

    double evalNode(const varAssignment* vars) override;

    LetterNode() : LetterNode(0) {}
    LetterNode(wchar_t let) : Node(tipNode::LetterNode), letter(let) {}
};

class NumberNode : public Node
{
public:
    double value;

    double evalNode(const varAssignment* vars) override;

    NumberNode() : NumberNode(NAN) {}
    NumberNode(double val) : Node(tipNode::NumberNode), value(val) {}
};


///////////////////////////////////////////////////////////////////////////
//////////TOKENS
///////////////////////////////////////////////////////////////////////////


class Token
{
public:
    tipToken type;
    int posInString;

    Token(): Token(tipToken::NullToken) {}
    Token(tipToken tip, int pos = -1): type(tip), posInString(pos) {}

    virtual ~Token() {}
};

class NumberToken : public Token
{
public:
    double value;

    NumberToken() : NumberToken(NAN) {}
    NumberToken(double val, int pos = -1): Token(tipToken::NumberToken, pos), value(val) {}
};

class LetterToken : public Token
{
public:
    wchar_t letter;

    LetterToken() : LetterToken(0) {}
    LetterToken(wchar_t let, int pos = -1) : Token(tipToken::LetterToken, pos), letter(let) {}
};


class ParanToken : public Token
{
public:
    bool open;

    ParanToken() : ParanToken(1) {}
    ParanToken(bool open, int pos = -1) : open(open) {type = tipToken::ParanToken; posInString = pos;}
};

class ParansToken : public Token
{
public:
    std::list<Token*> innerTokenList;

    ParansToken() : Token(tipToken::ParansToken, -1) {}//{type = tipToken::ParansToken; posInString = -1;}
    ParansToken(std::list<Token*>::iterator beg, std::list<Token*>::iterator end, int pos = -1) : Token(tipToken::ParansToken, pos), innerTokenList(beg, end) {}

    ~ParansToken();
};

class OperatorToken : public Token
{
public:
    CodOperator cod;
    bool implicit;

    OperatorToken() : OperatorToken(CodOperator::noop, 0) {}
    OperatorToken(CodOperator cod, bool impl, int pos = -1) : Token(tipToken::OperatorToken, pos), cod(cod), implicit(impl) {}
};

class PredefFuncToken : public Token
{
public:
    CodFunctiePredefinita codFunc;
    ArgumentList<std::list<Token*>> args;

    PredefFuncToken() : PredefFuncToken(CodFunctiePredefinita::notPredefined) {}
    PredefFuncToken(CodFunctiePredefinita cod, int pos = -1) : Token(tipToken::PredefFuncToken, pos), codFunc(cod) {}

    ~PredefFuncToken();
};


class FunctionToken : public Token
{
public:
    wchar_t letter;
    ArgumentList<std::list<Token*>> args;

    FunctionToken() : FunctionToken(0) {}
    FunctionToken(wchar_t let, int pos = -1) : Token(tipToken::FunctionToken, pos), letter(let) {}

    ~FunctionToken();
};



class AssembledToken : public Token
{
public:
    Node* assembledNode;
    

    AssembledToken() : AssembledToken(nullptr) {}
    AssembledToken(Node* ptr, int pos = -1) : Token(tipToken::AssembledToken, pos), assembledNode(ptr) {}

    ~AssembledToken();
};

template<class T>
void clearList(std::list<T*> &contentList)
{
    for(T* t : contentList)
        if(t!=NULL)
            delete t;
    contentList.clear();
}

template<class T>
void clearList(std::vector<T*> &contentList)
{
    for(T* t : contentList)
        if(t!=NULL)
            delete t;
    contentList.clear();
}

#endif // __token_header
