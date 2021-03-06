
#include "pch.h"

#include "AST.h"
#include "TypeSystem.h"
#include "Runtime.h"
#include "ByteCode.h"

ASTFunction::ASTFunction(StmtNodePtr stmt):
    m_stmt(stmt)
{
}
void ASTFunction::call(RuntimeEnv *env)
{
    env->reserveFrame(m_codeSeq->getFrameSize());
    m_codeSeq->execute(env);
}
void ASTFunction::emitCode()
{
    m_codeSeq.reset(new ByteCodeSeq(m_stmt));
}

CodeManager::CodeManager()
{
    auto ts = TypeSystem::instance();
    auto p = new ASTFunction(StmtNodePtr(new StmtNode_Block()));
    m_funcPreMain.reset(p);
}

void CodeManager::emitAll()
{
    getFuncPreMain()->emitCode();
    for (auto p : m_funcs) {
        if (auto f = dynamic_cast<ASTFunction*>(p.second.get())) {
            f->emitCode();
        }
    }
}
