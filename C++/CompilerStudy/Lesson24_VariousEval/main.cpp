#include "pch.h" 

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#include <vector>
#include <string>
#include <iostream>
using namespace std;
//============================== lexical analysiser
enum TokenType{
    TT_End,
    TT_Int,
    TT_Operator,
};
struct Token {
    TokenType type;
    const char *begin, *end;
    Token(): type(TT_End), begin(NULL), end(NULL){}
    Token(TokenType _type, const char *_begin, const char *_end): type(_type), begin(_begin), end(_end){}
};
class Scanner {
public:
    Scanner(const char *str): m_str(str) {}
    Token next() {
        if (m_fetch.type != TT_End) {
            Token r = m_fetch;
            m_fetch.type = TT_End;
            return r;
        }
        for (; isspace(*m_str); ++m_str);
        if (!*m_str) return Token(TT_End, NULL, NULL);
        if (isdigit(*m_str)) {
            const char *begin = m_str;
            while (isdigit(*++m_str));
            return Token(TT_Int, begin, m_str);
        } else {
            const char *begin = m_str++;
            return Token(TT_Operator, begin, begin + 1);
        }
    }
    Token fetch() {
        if (m_fetch.type == TT_End) m_fetch = next();
        return m_fetch;
    }
private:
    const char *m_str;
    Token m_fetch;
};
//============================== syntax analysiser
struct ASTNode{
    virtual ~ASTNode(){}
    virtual void destroy(){}
};
struct ASTNode_Int: public ASTNode {
    int num;
    explicit ASTNode_Int(int _num): num(_num){}
    virtual void destroy(){ delete this; }
};
struct ASTNode_BinaryOp: public ASTNode {
    char op;
    ASTNode *left, *right;
    ASTNode_BinaryOp(char _op, ASTNode *_left, ASTNode *_right): op(_op), left(_left), right(_right){}
    virtual void destroy(){ 
        if (left != NULL) left->destroy();
        if (right != NULL) right->destroy();
        delete this;
    }
};
class Parser {
public:
    Parser(Scanner *scaner): m_scaner(scaner){}
    ASTNode* parse() {
        return _expr(0);
    }
private:
    ASTNode* _expr(int rbp) {
        ASTNode *n = _atom();
        while (m_scaner->fetch().type != TT_End && rbp < getOperatorLBP(m_scaner->fetch())) {
            n = _infix(n);
        }
        return n;
    }
    ASTNode* _infix(ASTNode *left) {
        Token t = m_scaner->next();
        assert(t.type == TT_Operator);
        switch (t.begin[0]) {
            case '+': return new ASTNode_BinaryOp('+', left, _expr(getOperatorRBP(t)));
            case '-': return new ASTNode_BinaryOp('-', left, _expr(getOperatorRBP(t)));
            case '*': return new ASTNode_BinaryOp('*', left, _expr(getOperatorRBP(t)));
            case '/': return new ASTNode_BinaryOp('/', left, _expr(getOperatorRBP(t)));
            case '%': return new ASTNode_BinaryOp('%', left, _expr(getOperatorRBP(t)));
            default: assert(0); return NULL;
        }
    } 
    ASTNode* _atom() {
        Token t = m_scaner->next();
        ASTNode *r = NULL;
        if (t.type == TT_Int) {
            r = new ASTNode_Int(atoi(string(t.begin, t.end).c_str()));
        } else {
            assert(t.type == TT_Operator && t.begin[0] == '(');
            r = _expr(0);
            consumeOperatorToken(')');
        }
        return r;
    }
private:
    int getOperatorLBP(const Token &t) {
        assert(t.type == TT_Operator);
        switch (t.begin[0]) {
            case '+': return 1;
            case '-': return 1;
            case '*': return 2;
            case '/': return 2;
            case '%': return 2;
            case ')': return 0;
            default: assert(0); return 0;
        }
    }
    int getOperatorRBP(const Token &t) {
        return getOperatorLBP(t);
    }
private:
    void consumeOperatorToken(char c) {
        Token t = m_scaner->next();
        assert(t.type == TT_Operator && t.begin[0] == c);
    }
private:
    Scanner *m_scaner;
};
//============================== tree-walking interpreter
int _evalByWalkAST(ASTNode *node) {
    if (ASTNode_Int* p = dynamic_cast<ASTNode_Int*>(node)) {
        return p->num;
    } else if (ASTNode_BinaryOp *p = dynamic_cast<ASTNode_BinaryOp*>(node)) {
        switch (p->op) {
            case '+': return _evalByWalkAST(p->left) + _evalByWalkAST(p->right);
            case '-': return _evalByWalkAST(p->left) - _evalByWalkAST(p->right);
            case '*': return _evalByWalkAST(p->left) * _evalByWalkAST(p->right);
            case '/': return _evalByWalkAST(p->left) / _evalByWalkAST(p->right);
            case '%': return _evalByWalkAST(p->left) % _evalByWalkAST(p->right);
            default: assert(0); return 0;
        }
    } else {
        assert(0);
        return 0;
    }
}
int evalByWalkAST(ASTNode *n, int loop) {
    int sum = 0;
    for (int i = 0; i < loop; ++i) sum += _evalByWalkAST(n);
    return sum;
}
//============================== bytecode virtual machine
struct VM {
    vector<int> evalStack;
    vector<char> codes;
    int pc;
    VM(): pc(0){}
};
void vmOp_add(VM *vm) {
    vm->evalStack[(int)vm->evalStack.size() - 2] += vm->evalStack[(int)vm->evalStack.size() - 1];
    vm->evalStack.pop_back();
}
void vmOp_sub(VM *vm) {
    vm->evalStack[(int)vm->evalStack.size() - 2] -= vm->evalStack[(int)vm->evalStack.size() - 1];
    vm->evalStack.pop_back();
}
void vmOp_mul(VM *vm) {
    vm->evalStack[(int)vm->evalStack.size() - 2] *= vm->evalStack[(int)vm->evalStack.size() - 1];
    vm->evalStack.pop_back();
}
void vmOp_div(VM *vm) {
    vm->evalStack[(int)vm->evalStack.size() - 2] /= vm->evalStack[(int)vm->evalStack.size() - 1];
    vm->evalStack.pop_back();
}
void vmOp_mod(VM *vm) {
    vm->evalStack[(int)vm->evalStack.size() - 2] %= vm->evalStack[(int)vm->evalStack.size() - 1];
    vm->evalStack.pop_back();
}
void vmOp_push(VM *vm) {
    vm->evalStack.push_back((int&)vm->codes[vm->pc]);
    vm->pc += sizeof(int);
}
template<typename T>
void vmEmit(VM *vm, T param) {
    int off = (int)vm->codes.size();
    vm->codes.resize(vm->codes.size() + sizeof(T));
    (T&)vm->codes[off] = param;
}
void emitVMCodeByWalkAST(VM *vm, ASTNode *node) {
    if (ASTNode_Int *p = dynamic_cast<ASTNode_Int*>(node)) {
        vmEmit(vm, &vmOp_push);
        vmEmit(vm, p->num);
    } else if (ASTNode_BinaryOp *p = dynamic_cast<ASTNode_BinaryOp*>(node)) {
        emitVMCodeByWalkAST(vm, p->left);
        emitVMCodeByWalkAST(vm, p->right);
        switch (p->op) {
            case '+': vmEmit(vm, &vmOp_add); break;
            case '-': vmEmit(vm, &vmOp_sub); break;
            case '*': vmEmit(vm, &vmOp_mul); break;
            case '/': vmEmit(vm, &vmOp_div); break;
            case '%': vmEmit(vm, &vmOp_mod); break;
            default: assert(0);
        }
    } else {
        assert(0);
    }
}
int evalByRuningVM(ASTNode *node, int loop) {
    VM vm;
    emitVMCodeByWalkAST(&vm, node);

    int sum = 0;
    for (int i = 0; i < loop; ++i) {
        vm.pc = 0;
        while (vm.pc < (int)vm.codes.size()) {
            void(*op)(VM*) = (void(*&)(VM*))vm.codes[vm.pc];
            vm.pc += sizeof(&vmOp_push);
            op(&vm);
        }
        sum += vm.evalStack[0];
        vm.evalStack.pop_back();
    }
    return sum;
}
//============================== JIT
#ifdef _MSC_VER
#pragma warning(disable : 4312)
#pragma warning(disable : 4311)
#include <Windows.h>
#pragma warning(default : 4311)
#pragma warning(default : 4312)
void* mallocExec(int size) {
    void *p = ::VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    assert(p != NULL);
    return p;
}
void freeExec(void *p) {
    assert(p != NULL);
	::VirtualFree(p, 0, MEM_RELEASE);
}
#endif
#if defined(__linux__) || defined(__APPLE__)
#include <unistd.h>
#include <sys/mman.h>
static void* mallocExec(int size) {
    void *p = NULL;
    ::posix_memalign(&p, ::getpagesize(), size);
    ASSERT(p != NULL);
    int erro = ::mprotect(p, size, PROT_READ | PROT_WRITE | PROT_EXEC);
    ASSERT(erro == 0);
    return p;
}
static void freeExec(void *p) {
    ASSERT(p != NULL);
    ::free(p);
}
#endif
class JITEngine {
public:
    JITEngine(): m_off(0) { m_codes = (char*)mallocExec(4096); }
    ~JITEngine() { freeExec(m_codes); }
    void emit(int n, ...) {
        va_list arg;
        va_start(arg, n);
        for (int i = 0; i < n; ++i) m_codes[m_off++] = va_arg(arg, char);
        va_end(arg);
    }
    int run() { return ((int(*)())m_codes)();}
private:
    char *m_codes;
    int m_off;
};
void x86Emit_begin(JITEngine *engine) {
    engine->emit(1, 0x53); // push ebx
    engine->emit(1, 0x52); // push edx
}
void x86Emit_push(JITEngine *engine, int i) {
    char *pi = (char*)&i;
    engine->emit(5, 0x68, pi[0], pi[1], pi[2], pi[3]); // push i
}
void x86Emit_op(JITEngine *engine, char c) {
    engine->emit(4, 0x8b, 0x44, 0x24, 0x04); // mov eax, dword [esp+4]
    engine->emit(3, 0x8b, 0x1c, 0x24); // mov ebx, dword [esp]
    switch (c) { // op eax, ebx
        case '+': engine->emit(2, 0x03, 0xc3); break; 
        case '-': engine->emit(2, 0x2b, 0xc3); break;
        case '*': engine->emit(3, 0x0f, 0xaf, 0xc3);break;
        case '/': case '%': 
            engine->emit(2, 0x33, 0xd2); // xor edx, edx
            engine->emit(2, 0xf7, 0xfb);
            if (c == '%') engine->emit(2, 0x8b, 0xc2); // mov eax, edx
            break;
        default: assert(0); break;
    }
    engine->emit(4, 0x89, 0x44, 0x24, 0x04); // mov dword [esp+4], eax
    engine->emit(3, 0x83, 0xc4, 0x04); // add esp, 4
}
void x86Emit_end(JITEngine *engine) {
    engine->emit(3, 0x8b, 0x04, 0x24); // mov eax, dword [esp]
    engine->emit(3, 0x83, 0xc4, 0x04); // add esp, 4
    engine->emit(1, 0x5a); // pop edx
    engine->emit(1, 0x5b); // pop ebx
    engine->emit(1, 0xc3); // ret
}
void emitx86InsByWalkAST(JITEngine *engine, ASTNode *node) {
    if (ASTNode_Int* p = dynamic_cast<ASTNode_Int*>(node)) {
        x86Emit_push(engine, p->num);
    } else if (ASTNode_BinaryOp* p = dynamic_cast<ASTNode_BinaryOp*>(node)) {
        emitx86InsByWalkAST(engine, p->left);
        emitx86InsByWalkAST(engine, p->right);
        x86Emit_op(engine, p->op);
    } else {
        assert(0);
    }
}
int evalByRuningJIT(ASTNode *n, int loop) {
    JITEngine engine;
    x86Emit_begin(&engine);
    emitx86InsByWalkAST(&engine, n);
    x86Emit_end(&engine);

    int sum = 0;
    for (int i = 0; i < loop; ++i) sum += engine.run();
    return sum;
}
//============================== main
#define TIMINE(codes) { clock_t start = clock(); codes; printf("%f sec\n", float(clock() - start) / CLOCKS_PER_SEC);}

int main() {
    Scanner scanner("1+2-3*4+(5-6)-(7+8)%9");
    ASTNode *n = Parser(&scanner).parse();

    const int LOOP = 100000;
    int result = 0, result2 = 0;
    {
        printf("tree-walking interpreter (loop=%d)\n", LOOP);
        TIMINE(result = evalByWalkAST(n, LOOP));
    }
    {
        printf("stack-based virtual machine (loop=%d)\n", LOOP);
        TIMINE(result2 = evalByRuningVM(n, LOOP); assert(result == result2); );
    }
    {
        printf("jit compiler (loop=%d)\n", LOOP);
        TIMINE(result2 = evalByRuningJIT(n, LOOP); assert(result == result2));
    }

    n->destroy();
}
