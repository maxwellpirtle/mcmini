#include "MCThreadJoin.h"

MCTransition*
MCReadThreadJoin(const MCSharedTransition *shmTransition, void *shmData, MCState *state)
{
    // TODO: Potentially add asserts that the thread that just ran exists!
    auto newThreadData = static_cast<MCThreadShadow *>(shmData);

    auto threadThatExists = state->getVisibleObjectWithSystemIdentity<MCThread>((MCSystemID)newThreadData->systemIdentity);
    tid_t newThreadId = threadThatExists != nullptr ? threadThatExists->tid : state->addNewThread(*newThreadData);
    tid_t threadThatRanId = shmTransition->executor;

    auto joinThread = state->getThreadWithId(newThreadId);
    auto threadThatRan = state->getThreadWithId(threadThatRanId);
    return new MCThreadJoin(threadThatRan, joinThread);
}

std::shared_ptr<MCTransition>
MCThreadJoin::staticCopy()
{
    auto threadCpy=
            std::static_pointer_cast<MCThread, MCVisibleObject>(this->thread->copy());
    auto targetThreadCpy =
            std::static_pointer_cast<MCThread, MCVisibleObject>(this->target->copy());
    auto threadStartCpy = new MCThreadJoin(threadCpy, targetThreadCpy);
    return std::shared_ptr<MCTransition>(threadStartCpy);
}

std::shared_ptr<MCTransition>
MCThreadJoin::dynamicCopyInState(const MCState *state)
{
    std::shared_ptr<MCThread> threadInState = state->getThreadWithId(thread->tid);
    std::shared_ptr<MCThread> targetInState = state->getThreadWithId(target->tid);
    auto cpy = new MCThreadJoin(threadInState, targetInState);
    return std::shared_ptr<MCTransition>(cpy);
}

bool
MCThreadJoin::enabledInState(const MCState *) {
    return thread->enabled() && target->getState() == MCThreadShadow::dead;
}

void
MCThreadJoin::applyToState(MCState *state)
{
    if (target->isDead()) {
        thread->awaken();
    }
    else {
        thread->sleep();
    }
}

bool
MCThreadJoin::coenabledWith(std::shared_ptr<MCTransition> transition)
{
    tid_t targetThreadId = transition->getThreadId();
    if (this->thread->tid == targetThreadId || this->target->tid == targetThreadId) {
        return false;
    }
    return true;
}

bool
MCThreadJoin::dependentWith(std::shared_ptr<MCTransition> transition)
{
    tid_t targetThreadId = transition->getThreadId();
    if (this->thread->tid == targetThreadId || this->target->tid == targetThreadId) {
        return true;
    }
    return false;
}

bool
MCThreadJoin::joinsOnThread(tid_t tid) const
{
    return this->target->tid == tid;
}

bool
MCThreadJoin::joinsOnThread(const std::shared_ptr<MCThread>& thread) const
{
    return this->target->tid == thread->tid;
}

void
MCThreadJoin::print()
{
    printf("thread %lu: pthread_join(%lu, _)\n", this->thread->tid, this->target->tid);
}