#ifndef __NATIVE_ASYNC_ACTIONCALLBACK_I__
#define __NATIVE_ASYNC_ACTIONCALLBACK_I__

/*-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
 * Propose :
 * -.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.*/

namespace native {

template<typename P>
ActionCallbackBaseDetached<P>::ActionCallbackBaseDetached(std::shared_ptr<ActionCallbackBase<P>> iInstance, P&& p) :
        AsyncBase(iInstance->getLoop()),
        _instance(iInstance),
        _args(p) {
}

template<typename P>
ActionCallbackBaseDetachedError<P>::ActionCallbackBaseDetachedError(std::shared_ptr<ActionCallbackBase<P>> iInstance, const FutureError &iError) :
        AsyncBase(iInstance->getLoop()),
        _instance(iInstance),
        _error(iError) {
}

template<typename P>
void ActionCallbackBaseDetached<P>::executeAsync(std::shared_ptr<AsyncBase>) {
    NNATIVE_FCALL();
    _instance->setValueCb(std::forward<P>(std::get<0>(_args)));
}


template<typename T>
void ActionCallbackBaseDetached<void>::executeAsyncT(std::shared_ptr<AsyncBase>) {
    NNATIVE_FCALL();
    _instance->setValueCb();
}

template<typename P>
void ActionCallbackBaseDetachedError<P>::executeAsync(std::shared_ptr<AsyncBase>) {
    NNATIVE_FCALL();
    _instance->setErrorCb(_error);
}

template<typename P>
void ActionCallbackBaseDetached<P>::Enqueue(std::shared_ptr<ActionCallbackBase<P>> iInstance, P&& p) {
    NNATIVE_FCALL();
    NNATIVE_ASSERT(iInstance);
    std::unique_ptr<ActionCallbackBaseDetached<P>> detachedInst(new ActionCallbackBaseDetached<P>(iInstance, std::forward<P>(p)));
    detachedInst->enqueue();
    detachedInst.release();
}

template<typename T>
void ActionCallbackBaseDetached<void>::EnqueueT(std::shared_ptr<ActionCallbackBase<void>> iInstance) {
    NNATIVE_FCALL();
    NNATIVE_ASSERT(iInstance);
    std::unique_ptr<ActionCallbackBaseDetached<void>> detachedInst(new ActionCallbackBaseDetached<void>(iInstance));
    detachedInst->enqueue();
    detachedInst.release();
}

template<typename P>
void ActionCallbackBaseDetachedError<P>::Enqueue(std::shared_ptr<ActionCallbackBase<P>> iInstance, const FutureError &iError) {
    NNATIVE_FCALL();
    NNATIVE_ASSERT(iInstance);
    std::unique_ptr<ActionCallbackBaseDetachedError<P>> detachedInst(new ActionCallbackBaseDetachedError<P>(iInstance, iError));
    detachedInst->enqueue();
    detachedInst.release();
}

template<typename P>
void ActionCallbackBase<P>::SetValue(std::shared_ptr<ActionCallbackBase<P>> iInstance, P&& p) {
    ActionCallbackBaseDetached<P>::Enqueue(iInstance, std::forward<P>(p));
}

template<typename T>
void ActionCallbackBase<void>::SetValueT(std::shared_ptr<ActionCallbackBase<void>> iInstance) {
    ActionCallbackBaseDetached<void>::Enqueue(iInstance);
}

template<typename P>
void ActionCallbackBase<P>::SetError(std::shared_ptr<ActionCallbackBase<P>> iInstance, const FutureError &iError) {
    ActionCallbackBaseDetachedError<P>::Enqueue(iInstance, iError);
}

template<typename T>
void ActionCallbackBase<void>::SetErrorT(std::shared_ptr<ActionCallbackBase<void>> iInstance, const FutureError &iError) {
    ActionCallbackBaseDetachedError<void>::Enqueue(iInstance, iError);
}

template<typename R, typename... Args>
ActionCallback<R, Args...>::ActionCallback(std::shared_ptr<uv_loop_t> iLoop, std::function<R(Args...)> f, Args&&... args) :
         _f(f),
         _args(args...),
         _future(std::make_shared<FutureShared<R>>(iLoop))
{
}

template<typename... Args>
ActionCallback<void, Args...>::ActionCallback(std::shared_ptr<uv_loop_t> iLoop, std::function<void(Args...)> f, Args&&... args) :
         _f(f),
         _args(args...),
         _future(std::make_shared<FutureShared<void>>(iLoop))
{
}

template<typename R, typename P, typename... Args>
ActionCallbackP1<R, P, Args...>::ActionCallbackP1(std::shared_ptr<uv_loop_t> iLoop, std::function<R(P, Args...)> f, Args&&... args) :
         _f(f),
         _args(args...),
         _future(std::make_shared<FutureShared<R>>(iLoop))
{
}

template<typename P, typename... Args>
ActionCallbackP1<void, P, Args...>::ActionCallbackP1(std::shared_ptr<uv_loop_t> iLoop, std::function<void(P, Args...)> f, Args&&... args) :
         _f(f),
         _args(args...),
         _future(std::make_shared<FutureShared<void>>(iLoop))
{
}

template<typename R, typename... Args>
std::shared_ptr<FutureShared<R>> ActionCallback<R, Args...>::getFuture() {
    NNATIVE_ASSERT(this->_future);
    return this->_future;
}

template<typename... Args>
std::shared_ptr<FutureShared<void>> ActionCallback<void, Args...>::getFuture() {
    NNATIVE_ASSERT(this->_future);
    return this->_future;
}

template<typename R, typename P, typename... Args>
std::shared_ptr<FutureShared<R>> ActionCallbackP1<R, P, Args...>::getFuture() {
    NNATIVE_ASSERT(this->_future);
    return this->_future;
}

template<typename P, typename... Args>
std::shared_ptr<FutureShared<void>> ActionCallbackP1<void, P, Args...>::getFuture() {
    NNATIVE_ASSERT(this->_future);
    return this->_future;
}

template<typename R, typename... Args>
std::shared_ptr<uv_loop_t> ActionCallback<R, Args...>::getLoop() {
    return this->getFuture()->getLoop();
}

template<typename... Args>
std::shared_ptr<uv_loop_t> ActionCallback<void, Args...>::getLoop() {
    return this->getFuture()->getLoop();
}

template<typename R, typename P, typename... Args>
std::shared_ptr<uv_loop_t> ActionCallbackP1<R, P, Args...>::getLoop() {
    return this->getFuture()->getLoop();
}

template<typename P, typename... Args>
std::shared_ptr<uv_loop_t> ActionCallbackP1<void, P, Args...>::getLoop() {
    return this->getFuture()->getLoop();
}

template<typename R, typename P, typename... Args>
template<std::size_t... Is>
void ActionCallbackP1<R, P, Args...>::callFn(P&& p, helper::TemplateSeqInd<Is...>) {
    if(!this->_future) {
        return;
    }

    try {
        this->_future->setValue(this->_f(std::forward<P>(p), std::get<Is>(this->_args)...));
    } catch (const FutureError &e) {
        this->_future->setError(e);
    }
}

template<typename P, typename... Args>
template<std::size_t... Is>
void ActionCallbackP1<void, P, Args...>::callFn(P&& p, helper::TemplateSeqInd<Is...>) {
    if(!this->_future) {
        return;
    }

    try {
        this->_f(std::forward<P>(p), std::get<Is>(_args)...);
        this->_future->setValue();
    } catch (const FutureError &e) {
        this->_future->setError(e);
    }
}

template<typename R, typename... Args>
template<std::size_t... Is>
void ActionCallback<R, Args...>::callFn(helper::TemplateSeqInd<Is...>) {
    if(!this->_future) {
        return;
    }

    try {
        this->_future->setValue(this->_f(std::get<Is>(_args)...));
    } catch (const FutureError &e) {
        this->_future->setError(e);
    }
}

template<typename... Args>
template<std::size_t... Is>
void ActionCallback<void, Args...>::callFn(helper::TemplateSeqInd<Is...>) {
    if(!this->_future) {
        return;
    }

    try {
        this->_f(std::get<Is>(_args)...);
        this->_future->setValue();
    } catch (const FutureError &e) {
        this->_future->setError(e);
    }
}

template<typename R, typename... Args>
void ActionCallback<R, Args...>::setValueCb() {
    this->template callFn(helper::TemplateSeqIndGen<sizeof...(Args)>());
}

template<typename... Args>
void ActionCallback<void, Args...>::setValueCb() {
    this->template callFn(helper::TemplateSeqIndGen<sizeof...(Args)>());
}

template<typename R, typename P, typename... Args>
void ActionCallbackP1<R, P, Args...>::setValueCb(P&& p) {
    this->template callFn(std::forward<P>(p), helper::TemplateSeqIndGen<sizeof...(Args)>());
}

template<typename P, typename... Args>
void ActionCallbackP1<void, P, Args...>::setValueCb(P&& p) {
    this->template callFn(std::forward<P>(p), helper::TemplateSeqIndGen<sizeof...(Args)>());
}

template<typename R, typename... Args>
void ActionCallback<R, Args...>::setErrorCb(const FutureError &iError) {
    if(this->_future) {
        this->_future->setError(iError);
    }
}

template<typename... Args>
void ActionCallback<void, Args...>::setErrorCb(const FutureError &iError) {
    if(this->_future) {
        this->_future->setError(iError);
    }
}

template<typename R, typename P, typename... Args>
void ActionCallbackP1<R, P, Args...>::setErrorCb(const FutureError &iError) {
    if(_future) {
        _future->setError(iError);
    }
}

template<typename P, typename... Args>
void ActionCallbackP1<void, P, Args...>::setErrorCb(const FutureError &iError) {
    if(this->_future) {
        this->_future->setError(iError);
    }
}

// Error

template<typename... Args>
ActionCallbackError<Args...>::ActionCallbackError(std::shared_ptr<uv_loop_t> iLoop, std::function<void(const FutureError&, Args...)> f, Args&&... args) :
         _f(f),
         _args(args...),
         _future(std::make_shared<FutureShared<void>>(iLoop))
{
}

template<typename R, typename... Args>
ActionCallbackErrorP1<R, Args...>::ActionCallbackErrorP1(std::shared_ptr<uv_loop_t> iLoop, std::function<R(const FutureError&, Args...)> f, Args&&... args) :
         _f(f),
         _args(args...),
         _future(std::make_shared<FutureShared<R>>(iLoop))
{
}

template<typename... Args>
std::shared_ptr<FutureShared<void>> ActionCallbackError<Args...>::getFuture() {
    NNATIVE_ASSERT(this->_future);
    return this->_future;
}

template<typename R, typename... Args>
std::shared_ptr<FutureShared<R>> ActionCallbackErrorP1<R, Args...>::getFuture() {
    NNATIVE_ASSERT(this->_future);
    return this->_future;
}

template<typename... Args>
std::shared_ptr<uv_loop_t> ActionCallbackError<Args...>::getLoop() {
    return this->getFuture()->getLoop();
}

template<typename R, typename... Args>
std::shared_ptr<uv_loop_t> ActionCallbackErrorP1<R, Args...>::getLoop() {
    return this->getFuture()->getLoop();
}

template<typename R, typename... Args>
template<std::size_t... Is>
void ActionCallbackErrorP1<R, Args...>::callFn(const FutureError& iError, helper::TemplateSeqInd<Is...>) {
    if(!this->_future) {
        return;
    }

    try {
        this->_future->setValue(this->_f(iError, std::get<Is>(this->_args)...));
    } catch (const FutureError& e) {
        this->_future->setError(e);
    }
}

template<typename... Args>
template<std::size_t... Is>
void ActionCallbackError<Args...>::callFn(const FutureError& iError, helper::TemplateSeqInd<Is...>) {
    if(!this->_future) {
        return;
    }

    try {
        this->_f(iError, std::get<Is>(_args)...);
        this->_future->setValue();
    } catch (const FutureError &e) {
        this->_future->setError(e);
    }
}

template<typename... Args>
void ActionCallbackError<Args...>::setValueCb() {
    if(this->_future) {
        this->_future->setValue();
    }
}

template<typename R, typename... Args>
void ActionCallbackErrorP1<R, Args...>::setValueCb(R&& r) {
    if(_future) {
        _future->setValue(std::forward<R>(r));
    }
}

template<typename... Args>
void ActionCallbackError<Args...>::setErrorCb(const FutureError &iError) {
    this->template callFn(iError, helper::TemplateSeqIndGen<sizeof...(Args)>());
}

template<typename P, typename... Args>
void ActionCallbackErrorP1<P, Args...>::setErrorCb(const FutureError &iError) {
    this->template callFn(iError, helper::TemplateSeqIndGen<sizeof...(Args)>());
}

} /* namespace native */

#endif  /* __NATIVE_ASYNC_ACTIONCALLBACK_I__ */